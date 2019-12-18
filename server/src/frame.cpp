#include <string>
#include <sstream>
#include <cstring>
#include "frame.hpp"
#include "json.h"
#include "base64.h"

using namespace std;

Frame_pool::Frame_pool()
{
  mem_pool_msg = new CMemPool(MEM_POOL_UNIT_NUM, SEQ_BUF_LEN);
  mem_pool_seq = new CMemPool(MEM_POOL_UNIT_NUM, MSG_BUF_LEN);
  mem_pool_det = new CMemPool(MEM_POOL_UNIT_NUM, DET_BUF_LEN);
};

Frame_pool::Frame_pool(int unit_num)
{
  mem_pool_msg = new CMemPool(unit_num, SEQ_BUF_LEN);
  mem_pool_seq = new CMemPool(unit_num, MSG_BUF_LEN);
  mem_pool_det = new CMemPool(unit_num, DET_BUF_LEN);
};

Frame_pool::~Frame_pool()
{

};

Frame Frame_pool::alloc_frame(void) {
  Frame frame;
  frame_init(frame);
  return frame;
};

void Frame_pool::free_frame(Frame& frame) {
  mem_pool_seq->Free((void *)frame.seq_buf);
  mem_pool_msg->Free((void *)frame.msg_buf);
  mem_pool_det->Free((void *)frame.det_buf);
}

void Frame_pool::frame_init(Frame& frame) {
  frame.seq_len = frame.msg_len = frame.det_len = 0;
  frame.seq_buf = (unsigned char *)(mem_pool_seq->Alloc(SEQ_BUF_LEN, true));
  frame.msg_buf = (unsigned char *)(mem_pool_msg->Alloc(MSG_BUF_LEN, true));
  frame.det_buf = (unsigned char *)(mem_pool_det->Alloc(DET_BUF_LEN, true));
};



int frame_to_json(void* buf, const Frame& frame) {
	stringstream ss;
	ss << "{\n\"seq\":\"" << base64_encode((unsigned char *)frame.seq_buf, frame.seq_len) << "\",\n"
		<< "\"msg\": \"" << base64_encode((unsigned char*)(frame.msg_buf), frame.msg_len) << "\",\n" 
		<< "\"det\": \"" << base64_encode((unsigned char*)(frame.det_buf), frame.det_len) 
    << "\"\n}";
    

	memcpy(buf, ss.str().c_str(), ss.str().size());
	((unsigned char*)buf)[ss.str().size()] = '\0';
	return ss.str().size();
};

void json_to_frame(void* buf, Frame& frame) {
	json_object *raw_obj;
	raw_obj = json_tokener_parse((const char*)buf);

	json_object *seq_obj = json_object_object_get(raw_obj, "seq");
	json_object *msg_obj = json_object_object_get(raw_obj, "msg");
	json_object *det_obj = json_object_object_get(raw_obj, "det");

	string seq(base64_decode(json_object_get_string(seq_obj)));
	string msg(base64_decode(json_object_get_string(msg_obj)));
	string det(base64_decode(json_object_get_string(det_obj)));

	frame.seq_len = seq.size();
	frame.msg_len = msg.size();
	frame.det_len = det.size();

	memcpy(frame.seq_buf, seq.c_str(), frame.seq_len);
	((unsigned char*)frame.seq_buf)[frame.seq_len] = '\0';

	memcpy(frame.msg_buf, msg.c_str(), frame.msg_len);
	((unsigned char*)frame.msg_buf)[frame.msg_len] = '\0';

  if (frame.det_len > 0) {
    memcpy(frame.det_buf, det.c_str(), frame.det_len);
    ((unsigned char*)frame.det_buf)[frame.det_len] = '\0';
  }

  // free
  json_object_put(seq_obj);
  json_object_put(msg_obj);
  json_object_put(det_obj);
};
