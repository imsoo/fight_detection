#include <zmq.h>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <csignal>
#include <assert.h>
#include <pthread.h>
#include <boost/archive/text_oarchive.hpp>
#include "share_queue.h"
#include "frame.hpp"
#include "args.hpp"
#include "pose_detector.hpp"
// opencv
#include <opencv2/opencv.hpp>			// C++

// ZMQ
void *sock_pull;
void *sock_push;

// ShareQueue
SharedQueue<Frame> unprocessed_frame_queue;
SharedQueue<Frame> processed_frame_queue;;

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
      std::cout << "Worker | Recv From Ventilator | SEQ : " << frame.seq_buf 
        << " LEN : " << frame.msg_len << std::endl;
#endif
      unprocessed_frame_queue.push_back(frame);
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
      std::cout << "Worker | Send To Sink | SEQ : " << frame.seq_buf
        << " LEN : " << frame.msg_len << std::endl;
#endif
      send_json_len = frame_to_json(json_buf, frame);
      zmq_send(sock_push, json_buf, send_json_len, 0);

      frame_pool->free_frame(frame);
    }
  }
}

int main(int argc, char *argv[])
{
	if (argc < 2) {
    fprintf(stderr, "usage: %s <cfg> <weights> [-gpu GPU_ID] [-thresh THRESH]\n", argv[0]);
    return 0;
  }

  const char *cfg_path = argv[1];
  const char *weights_path = argv[2];
  int gpu_id = find_int_arg(argc, argv, "-gpu", 0);
  float thresh = find_float_arg(argc, argv, "-thresh", 0.2);
  fprintf(stdout, "cfg : %s, weights : %s, gpu-id : %d, thresh : %f\n", 
      cfg_path, weights_path, gpu_id, thresh);

  // opencv
  std::vector<int> param = {cv::IMWRITE_JPEG_QUALITY, 60 };

  // ZMQ
  int ret;

  void *context = zmq_ctx_new();

  sock_pull = zmq_socket(context, ZMQ_PULL);
  ret = zmq_connect(sock_pull, "ipc://unprocessed");
  assert(ret != -1);

  sock_push = zmq_socket(context, ZMQ_PUSH);
  ret = zmq_connect(sock_push, "ipc://processed");
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

  // darkent openpose detector
  PoseDetector detector(cfg_path, weights_path, gpu_id);
  People* det_people = nullptr;
  std::stringstream ss;

  // frame
  Frame frame;
  int frame_len;
  unsigned char *frame_buf_ptr;

  // time
  auto time_begin = std::chrono::steady_clock::now();
  auto time_end = std::chrono::steady_clock::now();
  double det_time;

  while(!exit_flag) {
    // recv from ventilator
    if (unprocessed_frame_queue.size() > 0) {
      frame = unprocessed_frame_queue.front();
      unprocessed_frame_queue.pop_front();

      frame_len = frame.msg_len;
      frame_buf_ptr = frame.msg_buf;

      // unsigned char array -> vector
      std::vector<unsigned char> raw_vec(frame_buf_ptr, frame_buf_ptr + frame_len);

      // vector -> mat
      cv::Mat raw_mat = cv::imdecode(cv::Mat(raw_vec), 1);
      
      // detect pose
      time_begin = std::chrono::steady_clock::now();
      detector.detect(raw_mat, thresh);
      det_people = detector.get_people();
      time_end = std::chrono::steady_clock::now();
      det_time = std::chrono::duration <double, std::milli> (time_end - time_begin).count();
#ifdef DEBUG
      std::cout << "Darknet | Detect | SEQ : " << frame.seq_buf << " Time : " << det_time << "ms" << std::endl;
#endif 
      // det_people & serialize
      ss.str(""); // ss clear
      {
        boost::archive::text_oarchive oa(ss);
        oa << *det_people;
      }
      std::string ser_people = ss.str();

      frame.det_len = ser_people.size();
      memcpy(frame.det_buf, ser_people.c_str(), frame.det_len);
      frame.det_buf[frame.det_len] = '\0';

      // mat -> vector
      std::vector<unsigned char> res_vec;
      cv::imencode(".jpg", raw_mat, res_vec, param);

      // vector -> frame array
      frame.msg_len = res_vec.size();
      std::copy(res_vec.begin(), res_vec.end(), frame.msg_buf);

      // push to processed frame_queue
      processed_frame_queue.push_back(frame);
    }
  }

	delete frame_pool;
  zmq_close(sock_pull);
  zmq_close(sock_push);
  zmq_ctx_destroy(context);

  return 0;
}
