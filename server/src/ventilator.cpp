#include <zmq.h>
#include <iostream>
#include <cassert>
#include <pthread.h>
#include <csignal>
#include "share_queue.h"
#include "frame.hpp"

// ZMQ
void *sock_pull;
void *sock_push;

// ShareQueue
SharedQueue<Frame> frame_queue;

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
      std::cout << "Ventilator | Recv From Client | SEQ : " << frame.seq_buf 
        << " LEN : " << frame.msg_len << std::endl;
#endif
      frame_queue.push_back(frame);
    }
  }
}

void *send_in_thread(void *ptr)
{
  int send_json_len;
  unsigned char json_buf[JSON_BUF_LEN];
  Frame frame;
  while(!exit_flag) {
    if (frame_queue.size() > 0) {
      frame = frame_queue.front();
      frame_queue.pop_front();

#ifdef DEBUG
      std::cout << "Ventilator | Send To Worker | SEQ : " << frame.seq_buf 
        << " LEN : " << frame.msg_len << std::endl;
#endif

      send_json_len = frame_to_json(json_buf, frame);
      zmq_send(sock_push, json_buf, send_json_len, 0);

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
  ret = zmq_bind(sock_pull, "tcp://*:5575");
  assert(ret != -1);

  sock_push = zmq_socket(context, ZMQ_PUSH);
  ret = zmq_bind(sock_push, "ipc://unprocessed");
  assert(ret != -1);

  // frame_pool
  frame_pool = new Frame_pool();

  // Thread
  pthread_t recv_thread;
  if (pthread_create(&recv_thread, 0, recv_in_thread, 0))
    std::cerr << "Thread creation failed (recv_thread)" << std::endl;

  pthread_t send_thread;
  if (pthread_create(&send_thread, 0, send_in_thread, 0))
    std::cerr << "Thread creation failed (recv_thread)" << std::endl;

  pthread_detach(send_thread);
  pthread_detach(recv_thread);

  while(!exit_flag);

  delete frame_pool;
  zmq_close(sock_pull);
  zmq_close(sock_push);
  zmq_ctx_destroy(context);
}
