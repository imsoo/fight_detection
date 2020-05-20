#include <zmq.h>
#include <iostream>
#include <thread>
#include <queue>
#include <chrono>
#include <string>
#include <fstream>
#include <csignal>
#ifdef __linux__
#include <tbb/concurrent_priority_queue.h>
#elif _WIN32
#include <concurrent_priority_queue.h>
#endif
#include <opencv2/opencv.hpp>
#include "share_queue.hpp"
#include "frame.hpp"
#include "util.hpp"
#include "args.hpp"

#ifdef __linux__
#define FD_SETSIZE 4096
using namespace tbb;
#elif _WIN32
using namespace concurrency;
#endif
using namespace cv;
using namespace std;

// thread
void fetch_thread(void);
void capture_thread(void);
void recv_thread(void);
void output_show_thread(void);
void input_show_thread(void);

volatile bool fetch_flag = false;
volatile bool exit_flag = false;
volatile int final_exit_flag = 0;

// ZMQ
void *context;
void *sock_push;
void *sock_sub;

// pair
class ComparePair
{
public:
  bool operator()(pair<long, Frame> n1, pair<long, Frame> n2) {
    return n1.first > n2.first;
  }
};
Frame_pool *frame_pool;
concurrent_priority_queue<pair<long, Frame>, ComparePair> recv_queue;

// Queue
SharedQueue<Mat> cap_queue;
SharedQueue<Mat> fetch_queue;

// opencv
VideoCapture cap;
VideoWriter writer;
static Mat mat_show_output;
static Mat mat_show_input;
static Mat mat_recv;
static Mat mat_cap;
static Mat mat_fetch;

const int cap_width = 640;
const int cap_height = 480;
double delay;
long volatile show_frame = 1;
double end_frame;

// option
bool cam_input_flag;
bool vid_input_flag;
bool dont_show_flag;
bool json_output_flag;
bool vid_output_flag;

// output
string out_json_path;
string out_vid_path;

ofstream out_json_file;

void sig_handler(int s)
{
  exit_flag = true;
}

int main(int argc, char *argv[])
{
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <-addr ADDR> <-cam CAM_NUM | -vid VIDEO_PATH> [-dont_show] [-out_json] [-out_vid] \n" << std::endl;
    return 0;
  }

  // install signal
  std::signal(SIGINT, sig_handler);

  // option init
  int cam_num = find_int_arg(argc, argv, "-cam", -1);
  if (cam_num != -1)
    cam_input_flag = true;

  const char *vid_def_path = "./test.mp4";
  const char *vid_path = find_char_arg(argc, argv, "-vid", vid_def_path);
  if (vid_path != vid_def_path)
    vid_input_flag = true;

  dont_show_flag = find_arg(argc, argv, "-dont_show");
  json_output_flag = find_arg(argc, argv, "-out_json");
  vid_output_flag = find_arg(argc, argv, "-out_vid");

  // frame_pool init
  frame_pool = new Frame_pool(5000);

  // ZMQ
  const char *addr = find_char_arg(argc, argv, "-addr", "127.0.0.1");
  context = zmq_ctx_new();

  sock_push = zmq_socket(context, ZMQ_PUSH);
  zmq_connect(sock_push, ((std::string("tcp://") + addr) + ":5575").c_str());

  sock_sub = zmq_socket(context, ZMQ_SUB);
  zmq_connect(sock_sub, ((std::string("tcp://") + addr) + ":5570").c_str());

  zmq_setsockopt(sock_sub, ZMQ_SUBSCRIBE, "", 0);

  if (vid_input_flag) {
    // VideoCaputre video
    cap = VideoCapture(vid_path);
    out_json_path = string(vid_path, strrchr(vid_path, '.')) + "_output.json";
    out_vid_path = string(vid_path, strrchr(vid_path, '.')) + "_output.mp4";
  }
  else if (cam_input_flag) {
    // VideoCapture cam
    cap = VideoCapture(cam_num);
    cap.set(CAP_PROP_FPS, 20);
    cap.set(CAP_PROP_BUFFERSIZE, 3);
    fetch_flag = true;
    out_json_path = "./cam_output.json";
    out_vid_path = "./cam_output.mp4";
  }
  else {
    // error
    std::cerr << "Usage: " << argv[0] << " <-addr ADDR> <-cam CAM_NUM | -vid VIDEO_PATH> [-dont_show] [-out_json] [-out_vid] \n" << std::endl;
    return 0;
  }

  if (!cap.isOpened()) {
    cerr << "Erro VideoCapture...\n";
    return -1;
  }

  double fps = cap.get(CAP_PROP_FPS);
  end_frame = cap.get(CAP_PROP_FRAME_COUNT);
  delay = cam_input_flag ? 1 : (1000.0 / fps);

  // read frame
  cap.read(mat_fetch);

  if (mat_fetch.empty()) {
    cerr << "Empty Mat Captured...\n";
    return 0;
  }

  mat_show_output = mat_fetch.clone();
  mat_show_input = mat_fetch.clone();

  // output init
  if (json_output_flag) {
    out_json_file = ofstream(out_json_path);
    if (!out_json_file.is_open()) {
      cerr << "output file : " << out_json_path << " open error \n";
      return 0;
    }
    out_json_file << "{\n \"det\": [\n";
  }

  if (vid_output_flag) {
    writer.open(out_vid_path, VideoWriter::fourcc('M', 'P', '4', 'V'), fps, Size(cap_width, cap_height), true);
    if (!writer.isOpened()) {
      cerr << "Erro VideoWriter...\n";
      return -1;
    }
  }

  // thread init
  thread thread_fetch(fetch_thread);
  thread_fetch.detach();

  while (!fetch_flag);

  thread thread_show_input(output_show_thread);
  thread thread_show_output(input_show_thread);
  thread thread_recv(recv_thread);
  thread thread_capture(capture_thread);

  thread_show_input.detach();
  thread_show_output.detach();
  thread_recv.detach();
  thread_capture.detach();

  while (final_exit_flag)
  {
    // for debug
    cout << "R : " << recv_queue.size() << " | C : " << cap_queue.size() << " | F : " << fetch_queue.size() << " | T : " << end_frame << " : " << show_frame << endl;
  }

  cap.release();

  if (json_output_flag) {
    out_json_file << "\n ]\n}";
    out_json_file.close();
  }

  if (vid_output_flag) {
    writer.release();
  }

  delete frame_pool;
  zmq_close(sock_sub);
  zmq_close(sock_push);
  zmq_ctx_destroy(context);

  return 0;
}

#define FETCH_THRESH 100
#define FETCH_WAIT_THRESH 30
#define FETCH_STATE 0
#define FETCH_WAIT 1
void fetch_thread(void) {
  volatile int fetch_state = FETCH_STATE;
  final_exit_flag += 1;
  while (!exit_flag) {

    switch (fetch_state) {
    case FETCH_STATE:
      if (cap.grab()) {

        cap.retrieve(mat_fetch);

        // push fetch queue
        fetch_queue.push_back(mat_fetch.clone());

        // if cam dont wait
        if (!cam_input_flag && (fetch_queue.size() > FETCH_THRESH)) {
          fetch_state = FETCH_WAIT;
        }
      }
      // if fetch end
      else {
        final_exit_flag -= 1;
        return;
      }
      break;
    case FETCH_WAIT:
      fetch_flag = true;
      if (fetch_queue.size() < FETCH_WAIT_THRESH) {
        fetch_state = FETCH_STATE;
      }
      break;
    }
  }
  final_exit_flag -= 1;
}

void capture_thread(void) {
  static vector<int> param = { IMWRITE_JPEG_QUALITY, 50 };
  static vector<uchar> encode_buf(JSON_BUF_LEN);

  volatile int frame_seq_num = 1;
  string frame_seq;

  // for json
  unsigned char json_buf[JSON_BUF_LEN];
  int send_json_len;

  Frame frame = frame_pool->alloc_frame();

  final_exit_flag += 1;
  while (!exit_flag) {
    if (fetch_queue.size() < 1)
      continue;

    // get input mat 
    mat_cap = fetch_queue.front().clone();
    fetch_queue.pop_front();

    if (mat_cap.empty()) {
      cerr << "Empty Mat Captured...\n";
      continue;
    }

    // resize
    resize(mat_cap, mat_cap, Size(cap_width, cap_height));

    // push to cap queue (for display input)
    cap_queue.push_back(mat_cap.clone());

    // mat to jpg
    imencode(".jpg", mat_cap, encode_buf, param);

    // jpg to json (seq + msg)
    frame_seq = to_string(frame_seq_num);
    frame.seq_len = frame_seq.size();
    memcpy(frame.seq_buf, frame_seq.c_str(), frame.seq_len);

    frame.msg_len = encode_buf.size();
    memcpy(frame.msg_buf, &encode_buf[0], frame.msg_len);

    send_json_len = frame_to_json(json_buf, frame);

    // send json to server
    zmq_send(sock_push, json_buf, send_json_len, 0);

    frame_seq_num++;
  }
  frame_pool->free_frame(frame);
  final_exit_flag -= 1;
}

void recv_thread(void) {
  int recv_json_len;
  int frame_seq_num = 1;
  Frame frame;
  unsigned char json_buf[JSON_BUF_LEN];

  final_exit_flag += 1;
  while (!exit_flag) {
    recv_json_len = zmq_recv(sock_sub, json_buf, JSON_BUF_LEN, ZMQ_NOBLOCK);

    if (recv_json_len > 0) {
      frame = frame_pool->alloc_frame();
      json_buf[recv_json_len] = '\0';
      json_to_frame(json_buf, frame);

      frame_seq_num = str_to_int((const char *)frame.seq_buf, frame.seq_len);

      // push to recv_queue (for display output)
      pair<long, Frame> p = make_pair(frame_seq_num, frame);
      recv_queue.push(p);
    }
  }
  final_exit_flag -= 1;
}

#define DONT_SHOW 0
#define SHOW_START 1
#define DONT_SHOW_THRESH 2  // for buffering
#define SHOW_START_THRESH 1 // for buffering

int volatile show_state = DONT_SHOW;
void input_show_thread(void) {

  if (!dont_show_flag) {
    cvNamedWindow("INPUT");
    moveWindow("INPUT", 30, 130);
    cv::imshow("INPUT", mat_show_input);
  }

  final_exit_flag += 1;
  while (!exit_flag) {
    switch (show_state) {
    case DONT_SHOW:
      break;
    case SHOW_START:
      if (cap_queue.size() >= DONT_SHOW_THRESH) {
        mat_show_input = cap_queue.front().clone();
        cap_queue.pop_front();
      }
      break;
    }

    if (!dont_show_flag) {

      // draw text (INPUT) Left Upper corner  
      putText(mat_show_input, "INPUT", Point(10, 25),
        FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 0, 255), 2);

      cv::imshow("INPUT", mat_show_input);

      // wait key for exit
      if (waitKey(delay) >= 0)
        exit_flag = true;
    }
  }
  final_exit_flag -= 1;
}

void output_show_thread(void) {
  Frame frame;

  if (!dont_show_flag) {
    cvNamedWindow("OUTPUT");
    moveWindow("OUTPUT", 670, 130);
    cv::imshow("OUTPUT", mat_show_output);
  }

  final_exit_flag += 1;
  while (!exit_flag) {

    switch (show_state) {
    case DONT_SHOW:
      if (recv_queue.size() >= SHOW_START_THRESH) {
        show_state = SHOW_START;
      }
      break;
    case SHOW_START:
      if (recv_queue.size() >= DONT_SHOW_THRESH || (end_frame - show_frame) == 1) {
        pair<long, Frame> p;
        // try pop success
        while (1) {
          if (recv_queue.try_pop(p)) {
            // if right sequence
            if (p.first == show_frame) {

              frame = ((Frame)p.second);
              vector<uchar> decode_buf((unsigned char*)(frame.msg_buf), (unsigned char*)(frame.msg_buf) + frame.msg_len);

              // jpg to mat
              mat_show_output = imdecode(decode_buf, IMREAD_COLOR);

              // resize
              resize(mat_show_output, mat_recv, Size(cap_width, cap_height));

              // wirte out_json
              if (json_output_flag) {
                if (show_frame != 1)
                  out_json_file << ",\n";
                out_json_file.write((const char*)frame.det_buf, frame.det_len);
              }

              // write out_vid
              if (vid_output_flag)
                writer.write(mat_show_output);

              // free frame
              frame_pool->free_frame(frame);
              show_frame++;
            }
            // wrong sequence
            else {
              recv_queue.push(p);
            }
            break;
          }
        }
      }
      else {
        show_state = DONT_SHOW;
      }
      break;
    }

    if (show_frame == end_frame)
      exit_flag = true;

    if (!dont_show_flag) {
      // draw text (OUTPUT) Left Upper corner  
      putText(mat_show_output, "OUTPUT", Point(10, 25),
        FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 255, 0), 2);

      cv::imshow("OUTPUT", mat_show_output);

      // wait key for exit
      if (waitKey(delay) >= 0)
        exit_flag = true;
    }
  }
  final_exit_flag -= 1;
}
