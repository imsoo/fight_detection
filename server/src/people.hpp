#ifndef __PEOPLE
#define __PEOPLE
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/vector.hpp>
#include <opencv2/opencv.hpp>
#include <cfloat>
#include <iostream>
#include <cstring>
#include <cmath>
#include <string>
#include <sstream>
#include <deque>
#include <queue>

template<typename T>
inline int intRoundUp(const T a)
{
    return int(a+0.5f);
}

class Person
{
private:
  enum { 
    UNTRACK = -1,
    OVERLAP_NUM = 6, 
    HIS_NUM = 33, 
    CHANGE_NUM = 8, 

    // action 
    ACTION_TYPE_NUM = 4, ACTION_HIS_NUM = 10, 
    STAND = 0, WALK = 1, PUNCH = 2, KICK = 3, UNKNOWN = 4,

    // coco joint keypoint
    JOINT_NUM = 18,
    NOSE = 0, NECK = 1, RSHOULDER = 2, RELBOW = 3, RWRIST = 4, LSHOULDER = 5, LELBOW = 6,
    LWRIST = 7, RHIP = 8, RKNEE = 9, RANKLE = 10, LHIP = 11, LKNEE = 12, LANKLE = 13, 
    REYE = 14, LEYE = 15, REAR = 16, LEAR = 17
  };
  struct Joint {
    float x[JOINT_NUM];
    float y[JOINT_NUM];
  };

  struct Change {
    float dist[CHANGE_NUM];
    float deg[CHANGE_NUM];
    float cur_deg[CHANGE_NUM];
  };

  std::deque<Joint> history;
  std::deque<Change> change_history;
  std::deque<int> actions;

  int overlap_count;
  int track_id;
  int action;

  // for bounding box
  float max_x;
  float max_y;
  float min_x;
  float min_y;

  Person* enemy;

public:
  Person() : enemy(nullptr), overlap_count(0), track_id(UNTRACK), action(ACTION_TYPE_NUM), max_x(FLT_MIN), max_y(FLT_MIN), min_x(FLT_MAX), min_y(FLT_MAX) { history.assign(1, {0}); }

  ~Person() { if (enemy != nullptr) enemy->set_enemy(nullptr); }
  // set
  void set_part(int part, float x, float y);
  void set_id(int id) { track_id = id; }
  void set_action(int type);  
  void set_enemy(Person* p) { enemy = p; }
  void set_rect(cv::Rect_<float>& rect);

  // get
  inline int get_id(void) const { return track_id; }
  inline int get_action(void) const { return action; }
  inline const char* get_action_str(void) const { 
    static const char* action_str[] = {"STAND", "WALK", "PUNCH", "KICK", "UNKNOWN"};
    return action_str[action]; 
  }
  inline const Person* get_enemy(void) const { return enemy; }
  cv::Rect_<float> get_rect(void) const;
  cv::Rect_<float> get_crash_rect(const Person& p) const;

  // crash
  inline bool is_danger(void) const { return (action == PUNCH || action == KICK); }
  bool check_crash(const Person& p) const;

  void update(Person* n_p);
  bool has_output(void);
  std::string get_history(void) const;

  // util
  float get_dist(float x1, float y1, float x2, float y2); 
  float get_deg(float x1, float y1, float x2, float y2);

  Person& operator=(const Person& p);
  friend std::ostream& operator<<(std::ostream &out, const Person &p);
};

class People
{
public:
  friend class boost::serialization::access;
  const float thresh = 0.05;
  std::vector<float> keypoints;
  std::vector<int> keyshape;
  float scale;

  People() {}

  People(std::vector<float> _keypoints, std::vector<int> _keyshape, float _scale) :
    keypoints(_keypoints), keyshape(_keyshape), scale(_scale) {}

  inline int get_person_num(void) const { return keyshape[0]; };

  std::vector<Person*> to_person(void);
  std::string get_output(void);
  void render_pose_keypoints(cv::Mat& frame);

  template<class Archive>
  void serialize(Archive & ar, const unsigned int version)
  {
    ar & keypoints;
    ar & keyshape;
    ar & scale;
  }
};

#endif
