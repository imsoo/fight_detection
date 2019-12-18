#include <zmq.h>
#include <stdio.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <assert.h>
#include <map>
#include <csignal>
#include <boost/archive/text_oarchive.hpp>
#include <tbb/concurrent_hash_map.h>
#include <opencv2/opencv.hpp>
#include "share_queue.h"
#include "people.hpp"
#include "Tracker.hpp"
#include "frame.hpp"

// ZMQ
void *sock_pull;
void *sock_pub;
void *sock_rnn;

// ShareQueue
tbb::concurrent_hash_map<int, Frame> frame_map;
SharedQueue<Frame> processed_frame_queue;

// pool
Frame_pool *frame_pool;

// signal
volatile bool exit_flag = false;
void sig_handler(int s)
{
  exit_flag = true;
}

void *recv_in_thread(void *ptr)
{
  int recv_json_len;
  unsigned char json_buf[JSON_BUF_LEN];
  Frame frame;

  while(!exit_flag) {
    recv_json_len = zmq_recv(sock_pull, json_buf, JSON_BUF_LEN, ZMQ_NOBLOCK);

    if (recv_json_len > 0) {
      frame = frame_pool->alloc_frame();
      json_buf[recv_json_len] = '\0';
      json_to_frame(json_buf, frame);

#ifdef DEBUG
      std::cout << "Sink | Recv From Worker | SEQ : " << frame.seq_buf
        << " LEN : " << frame.msg_len << std::endl;
#endif

      tbb::concurrent_hash_map<int, Frame>::accessor a;
      while(1)
      {
        if(frame_map.insert(a, atoi((char *)frame.seq_buf))) {
          a->second = frame;
          break;
        }
      }
    }
  }
}

void *send_in_thread(void *ptr)
{
  int send_json_len;
  unsigned char json_buf[JSON_BUF_LEN];
  Frame frame;

  while(!exit_flag) {
    if (processed_frame_queue.size() > 0) {
      frame = processed_frame_queue.front();
      processed_frame_queue.pop_front();

#ifdef DEBUG
      std::cout << "Sink | Pub To Client | SEQ : " << frame.seq_buf
        << " LEN : " << frame.msg_len << std::endl;
#endif

      send_json_len = frame_to_json(json_buf, frame);
      zmq_send(sock_pub, json_buf, send_json_len, 0);

      frame_pool->free_frame(frame);
    }
  }
}

int main()
{
  // ZMQ
  int ret;
  void *context = zmq_ctx_new();

  sock_pull = zmq_socket(context, ZMQ_PULL);
  ret = zmq_bind(sock_pull, "ipc://processed");
  assert(ret != -1);

  sock_pub = zmq_socket(context, ZMQ_PUB);
  ret = zmq_bind(sock_pub, "tcp://*:5570");
  assert(ret != -1);

  sock_rnn = zmq_socket(context, ZMQ_REQ);
  ret = zmq_connect(sock_rnn, "ipc://action");
  assert(ret != -1);

  // frame_pool
  frame_pool = new Frame_pool(5000);

  // Thread
  pthread_t recv_thread;
  if (pthread_create(&recv_thread, 0, recv_in_thread, 0))
    std::cerr << "Thread creation failed (recv_thread)" << std::endl;

  pthread_t send_thread;
  if (pthread_create(&send_thread, 0, send_in_thread, 0))
    std::cerr << "Thread creation failed (recv_thread)" << std::endl;

  pthread_detach(send_thread);
  pthread_detach(recv_thread);

  // serialize 
  std::stringstream ss;
  std::stringbuf *pbuf = ss.rdbuf();

  // frame
  Frame frame;
  int frame_len;
  unsigned char *frame_buf_ptr;
  char json_tmp_buf[1024];

  // Tracker
  Tracker tracker;
  TrackingBox tb;
  std::vector<TrackingBox> det_data;
  std::vector<TrackingBox> track_data;
  volatile int track_frame = 1;
  int init_flag = 0;

  // person
  std::vector<Person*> person_data;
  std::stringstream p_ss;
  std::ofstream output_file("output.txt");

  // rnn
  unsigned char rnn_buf[100];

  // draw
  std::vector<int> param = {cv::IMWRITE_JPEG_QUALITY, 60 };
  const int CNUM = 20;
  cv::RNG rng(0xFFFFFFFF);
  cv::Scalar_<int> randColor[CNUM];
  for (int i = 0; i < CNUM; i++)
    rng.fill(randColor[i], cv::RNG::UNIFORM, 0, 256);

  while(!exit_flag) {
    if (!frame_map.empty()) {
      tbb::concurrent_hash_map<int, Frame>::accessor c_a;

      if (frame_map.find(c_a, (const int)track_frame))
      {
        frame = (Frame)c_a->second;
        while(1) {
          if (frame_map.erase(c_a))
            break;
        }

        // unsigned char array -> vector
        frame_len = frame.msg_len;
        frame_buf_ptr = frame.msg_buf;
        std::vector<unsigned char> raw_vec(frame_buf_ptr, frame_buf_ptr + frame_len);
        
        // vector -> mat
        cv::Mat raw_mat = cv::imdecode(cv::Mat(raw_vec), 1);

        // get people & unserialize
        ss.str(""); // ss clear
        pbuf->sputn((const char*)frame.det_buf, frame.det_len);
        People people;
        {
          boost::archive::text_iarchive ia(ss);
          ia >> people;
        }   

        // detect people result to json
        std::string det_json;
        sprintf(json_tmp_buf, "{\n \"frame_id\":%d, \n ", track_frame);
        det_json = json_tmp_buf;
        det_json += people.get_output();
        det_json += "\n}";

        frame.det_len = det_json.size();
        memcpy(frame.det_buf, det_json.c_str(), frame.det_len);
        frame.det_buf[frame.det_len] = '\0';

        // draw people skeleton
        people.render_pose_keypoints(raw_mat);

        // people to person
        person_data.clear();
        person_data = people.to_person(); 

        // ready to track
        det_data.clear();
        for (auto it = person_data.begin(); it != person_data.end(); it++) {
          tb.frame = track_frame;
          tb.box = (*it)->get_rect();
          tb.p = (*it);
          det_data.push_back(tb);
        }

        // not detect people
        if (det_data.size() < 1) {
          processed_frame_queue.push_back(frame);
          track_frame++;
          continue;
        }

        // Track
        track_data.clear();
        if (init_flag == 0) {
          track_data = tracker.init(det_data);
          init_flag = 1;
        }
        else {
          track_data = tracker.update(det_data);
        }
  
        p_ss.str("");
        for (unsigned int i = 0; i < track_data.size(); i++) {
          // get person 
          Person* track_person = track_data[i].p;
          
          // get history
          if (i != 0)
            p_ss << '\n';
          p_ss << track_person->get_history();

          // get_output for train
          if (track_person->has_output()) {
            output_file << *(track_person);
          }
        }
        
        
        // RNN action
        double time_begin = getTickCount(); 

        std::string hists = p_ss.str();
        if (hists.size() > 0) {
          zmq_send(sock_rnn, hists.c_str(), hists.size(), 0);
          zmq_recv(sock_rnn, rnn_buf, 100, 0);

          // action update
          for (unsigned int i = 0; i < track_data.size(); i++) {
            track_data[i].p->set_action(rnn_buf[i] - '0');
          }
        }  

        double fee_time = (getTickCount() - time_begin) / getTickFrequency() * 1000;
#ifdef DEBUG
        std::cout << "RNN fee: " << fee_time << "ms | T : " << track_frame << std::endl;
#endif

        // check crash
        for (unsigned int i = 0; i < track_data.size(); i++) {
          Person* me = track_data[i].p;
          if (me->is_danger()) { // punch or kick
            for (unsigned int j = 0; j < track_data.size(); j++) {
              if (j == i)
                continue;
              Person* other = track_data[j].p;

              // if me and other crash
              if (me->check_crash(*other)) {
                //std::cout << "CRASH !!! " << me->get_id() << " : " << other->get_id() << std::endl;
                me->set_enemy(other);
                other->set_enemy(me);
              }
            }
          }

          // draw fight bounding box
          const Person *my_enemy = me->get_enemy();
          if (my_enemy != nullptr) {
            cv::Rect_<float> crash_rect = me->get_crash_rect(*my_enemy);

            cv::putText(raw_mat, "FIGHT", cv::Point(crash_rect.x, crash_rect.y), cv::FONT_HERSHEY_DUPLEX, 0.8, cv::Scalar(0, 255, 0), 2);
            cv::rectangle(raw_mat, crash_rect, cv::Scalar(0, 255, 0), 2, 8, 0);
          }
        }
        
        
        // draw Track data
        for (auto td: track_data) {
          // draw track_id
          cv::putText(raw_mat, to_string(td.id), cv::Point(td.box.x, td.box.y), cv::FONT_HERSHEY_DUPLEX, 0.8, randColor[td.id % CNUM], 2);
          // draw person bounding box
          cv::putText(raw_mat, td.p->get_action_str(), cv::Point(td.box.x, td.box.y + 30), cv::FONT_HERSHEY_DUPLEX, 0.8,
          randColor[td.id % CNUM], 2);
          cv::rectangle(raw_mat, td.box, randColor[td.id % CNUM], 2, 8, 0);
        }

        // draw_mat -> vector
        std::vector<unsigned char> res_vec;
        cv::imencode(".jpg", raw_mat, res_vec, param);

        // vector -> frame array
        frame.msg_len = res_vec.size();
        std::copy(res_vec.begin(), res_vec.end(), frame.msg_buf);

        processed_frame_queue.push_back(frame);
        track_frame++;
      }
    }
  }

  delete frame_pool;

  output_file.close();

  zmq_close(sock_pull);
  zmq_close(sock_pub);
  zmq_close(sock_rnn);

  zmq_ctx_destroy(context);
  return 0;
}
