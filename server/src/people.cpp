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
#include "people.hpp"

void Person::set_part(int part, float x, float y) {
  history.front().x[part] = x;
  history.front().y[part] = y;

  // bounding box update
  if (x < min_x)
    min_x = x;
  else if (x > max_x)
    max_x = x;

  if (y < min_y)
    min_y = y;
  else if (y > max_y)
    max_y = y;
}

void Person::set_action(int type) {
  if (type < 0)
    type = ACTION_TYPE_NUM;
  action = type;

  if (actions.size() == ACTION_HIS_NUM)
    actions.pop_front();
  actions.push_back(type);
}
  
float Person::get_dist(float x1, float y1, float x2, float y2) {
  if (x1 == 0 || x2 == 0)
    return 0.0;
  else {
    return sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
  }
}

float Person::get_deg(float x1, float y1, float x2, float y2) {
  if (x1 == 0 || x2 == 0)
    return 0.0;
  double dx = x2 - x1;
  double dy = y2 - y1;
  double rad = atan2(dy, dx);
  double degree = (rad * 180) / M_PI;
  if (degree < 0)
    degree += 360;
  return degree;
}

bool Person::has_output(void) {
  if (overlap_count <= 0 && history.size() == HIS_NUM) {
    overlap_count = OVERLAP_NUM;
    return true;
  }
  else 
    return false;
}

void Person::update(Person* n_p) 
{
  static const int change_part[] = {
    RELBOW, RWRIST, LELBOW, LWRIST, RKNEE, RANKLE, LKNEE, LANKLE
  };
  static const int change_pair[] = {
    RSHOULDER, RELBOW, 
    RELBOW, RWRIST, 
    LSHOULDER, LELBOW, 
    LELBOW, LWRIST, 
    RHIP, RKNEE, 
    RKNEE, RANKLE, 
    LHIP, LKNEE, 
    LKNEE, LANKLE
  };

  Change c;
  double deg, n_deg;
  int part, pair_1, pair_2;
  const Joint& j = history.back();
  const Joint& n_j = n_p->history.front();

  // change calc
  for (int i = 0; i < CHANGE_NUM; i++) {
    part = change_part[i];
    c.dist[i] = abs(get_dist(j.x[part], n_j.x[part], j.y[part], n_j.y[part]));

    pair_1 = change_pair[i * 2];
    pair_2 = change_pair[i * 2 + 1];

    deg = get_deg(j.x[pair_1], j.y[pair_1], j.x[pair_2], j.y[pair_2]);
    n_deg = get_deg(n_j.x[pair_1], n_j.y[pair_1], n_j.x[pair_2], n_j.y[pair_2]);
    c.cur_deg[i] = n_deg;
    if (deg == 0 || n_deg == 0)
      c.deg[i] = 0.0;
    else 
      c.deg[i] = abs(deg - n_deg);
  }

  if (history.size() == HIS_NUM) {
    history.pop_front();
    change_history.pop_front();
  }
  history.push_back(n_p->history.front());
  change_history.push_back(c);
  overlap_count--;

  // delete
  delete n_p;
}

/*
std::string get_history(void) const 
{
  std::stringstream ss;
  for (int i = 0; i < history.size(); i++) {
    if (i != 0)
      ss << '\n';
    for (int j = 0; j < JOINT_NUM; j++) {
      if (j != 0)
        ss << ',';
      ss << history[i].x[j] << ',' << history[i].y[j];
    }
  }
  for (int i = history.size(); i < HIS_NUM; i++) {
    if (i != 0)
      ss << '\n';
    for (int j = 0; j < JOINT_NUM; j++) {
      if (j != 0)
        ss << ',';
      ss << 0.0 << ',' << 0.0;
    }
  }
  return ss.str();
}
*/

bool Person::check_crash(const Person& other) const
{
  const static int punch_check_joint[] = {RELBOW, RWRIST, LELBOW, LWRIST};
  const static int kick_check_joint[] = {RKNEE, RANKLE, LKNEE, LANKLE};
  
  const int* check_joint = nullptr;
  int my_action = this->get_action();

  if (my_action == PUNCH)
    check_joint = punch_check_joint; 
  else if (my_action == KICK)
    check_joint = kick_check_joint;
  else
    return false;

  cv::Rect_<float> other_rect = other.get_rect();

  const Joint& j = history.back();
  float x, y;
  for (int i = 0; i < 4; i++) {
    x = j.x[check_joint[i]];
    y = j.y[check_joint[i]];

    if (other_rect.x <= x && x <= other_rect.x + other_rect.width &&
        other_rect.y <= y && y <= other_rect.y + other_rect.height)
      return true;
  }
  return false;
}

cv::Rect_<float> Person::get_crash_rect(const Person& p) const
{
  cv::Rect_<float> a_rect = this->get_rect();
  cv::Rect_<float> b_rect = p.get_rect();

  float min_x, min_y, max_x, max_y;

  min_x = a_rect.x < b_rect.x ? a_rect.x : b_rect.x;
  min_y = a_rect.y < b_rect.y ? a_rect.y : b_rect.y;
  max_x = (a_rect.x + a_rect.width) > (b_rect.x + b_rect.width) ? (a_rect.x + a_rect.width) : (b_rect.x +
      b_rect.width);
  max_y = (a_rect.y + a_rect.height) > (b_rect.y + b_rect.height) ? (a_rect.y + a_rect.height) : (b_rect.y +
      b_rect.height);

  return cv::Rect_<float>(cv::Point_<float>(min_x, min_y),
      cv::Point_<float>(max_x, max_y));
}

std::string Person::get_history(void) const 
{
  std::stringstream ss;
  for (size_t i = 0; i < change_history.size(); i++) {
    if (i != 0)
      ss << '\n';
    for (int j = 0; j < CHANGE_NUM; j++) {
      if (j != 0)
        ss << ',';
      ss << change_history[i].dist[j] << ',' << change_history[i].deg[j] << ',' << change_history[i].cur_deg[j];
    }
  }
  for (int i = change_history.size(); i < HIS_NUM - 1; i++) {
    if (i != 0)
      ss << '\n';
    for (int j = 0; j < CHANGE_NUM; j++) {
      if (j != 0)
        ss << ',';
      ss << 0.0 << ',' << 0.0 << ',' << 0.0;
    }
  }
  return ss.str();
}

cv::Rect_<float> Person::get_rect(void) const
{
  return cv::Rect_<float>(cv::Point_<float>(min_x, min_y),
      cv::Point_<float>(max_x, max_y));
}

void Person::set_rect(cv::Rect_<float>& rect) 
{
  min_x = rect.x;
  min_y = rect.y;
  max_x = rect.x + rect.width;
  max_y = rect.y + rect.height;
}

Person& Person::operator=(const Person& p)
{
  track_id = p.track_id;
  max_x = p.max_x;
  max_y = p.max_y;
  min_x = p.min_x;
  min_y = p.min_y;
  history = p.history;
  change_history = p.change_history;
  overlap_count = p.overlap_count;
  return *this;
}

std::ostream& operator<<(std::ostream &out, const Person &p)
{
  for (size_t i = 0; i < p.change_history.size(); i++) {
    for (int j = 0; j < p.CHANGE_NUM; j++) {
      if (j != 0)
        out << ',';
      out << p.change_history[i].dist[j] << ',' <<  p.change_history[i].deg[j] << ',' << p.change_history[i].cur_deg[j];
    }
    out << '\n';
  }
  return out;
}


std::vector<Person*> People::to_person(void) {
  std::vector<Person*> persons;
  int person_num = keyshape[0];
  int part_num = keyshape[1];
  for (int person = 0; person < person_num; person++) {
    Person *p = new Person();
    for (int part = 0; part < part_num; part++) {
      int index = (person * part_num + part) * keyshape[2];
      if (keypoints[index + 2] >  thresh) {
        p->set_part(part, keypoints[index] * scale, keypoints[index + 1] * scale);
      }
    }
    persons.push_back(p);
  }
  return persons;
}

std::string People::get_output(void) {
	std::string out_str = "\"people\": [\n";
	int person_num = keyshape[0];
	int part_num = keyshape[1];
	for (int person = 0; person < person_num; person++) {
		if (person != 0)
			out_str += ",\n";
		out_str += " {\n";
		for (int part = 0; part < part_num; part++) {
			if (part != 0) 
				out_str += ",\n ";
			int index = (person * part_num + part) * keyshape[2];
			char *buf = (char*)calloc(2048, sizeof(char));

			if (keypoints[index + 2] >  thresh) {
				sprintf(buf, " \"%d\":[%f, %f]", part, keypoints[index] * scale, keypoints[index + 1] * scale);
			}
			else {
				sprintf(buf, " \"%d\":[%f, %f]", part, 0.0, 0.0);
			}
			out_str += buf;
			free(buf);
		}
		out_str += "\n }";
	}
	out_str += "\n ]";
	return out_str;
}

void People::render_pose_keypoints(cv::Mat& frame)
{
  const int num_keypoints = keyshape[1];
  unsigned int pairs[] =
  {
    1, 2, 1, 5, 2, 3, 3, 4, 5, 6, 6, 7, 1, 8, 8, 9, 9, 10,
    1, 11, 11, 12, 12, 13, 1, 0, 0, 14, 14, 16, 0, 15, 15, 17
  };
  float colors[] =
  {
    255.f, 0.f, 85.f, 255.f, 0.f, 0.f, 255.f, 85.f, 0.f, 255.f, 170.f, 0.f,
    255.f, 255.f, 0.f, 170.f, 255.f, 0.f, 85.f, 255.f, 0.f, 0.f, 255.f, 0.f,
    0.f, 255.f, 85.f, 0.f, 255.f, 170.f, 0.f, 255.f, 255.f, 0.f, 170.f, 255.f,
    0.f, 85.f, 255.f, 0.f, 0.f, 255.f, 255.f, 0.f, 170.f, 170.f, 0.f, 255.f,
    255.f, 0.f, 255.f, 85.f, 0.f, 255.f
  };
  const int pairs_size = sizeof(pairs) / sizeof(unsigned int);
  const int number_colors = sizeof(colors) / sizeof(float);

  for (int person = 0; person < keyshape[0]; ++person)
  {
    // Draw lines
    for (int pair = 0u; pair < pairs_size; pair += 2)
    {
      const int index1 = (person * num_keypoints + pairs[pair]) * keyshape[2];
      const int index2 = (person * num_keypoints + pairs[pair + 1]) * keyshape[2];
      if (keypoints[index1 + 2] > thresh && keypoints[index2 + 2] > thresh)
      {
        const int color_index = pairs[pair + 1] * 3;
        cv::Scalar color { colors[(color_index + 2) % number_colors],
          colors[(color_index + 1) % number_colors],
          colors[(color_index + 0) % number_colors]};
        cv::Point keypoint1{ intRoundUp(keypoints[index1] * scale), intRoundUp(keypoints[index1 + 1] * scale) };
        cv::Point keypoint2{ intRoundUp(keypoints[index2] * scale), intRoundUp(keypoints[index2 + 1] * scale) };
        cv::line(frame, keypoint1, keypoint2, color, 2);
      }
    }
    // Draw circles
    for (int part = 0; part < num_keypoints; ++part)
    {
      const int index = (person * num_keypoints + part) * keyshape[2];
      if (keypoints[index + 2] > thresh)
      {
        const int color_index = part * 3;
        cv::Scalar color { colors[(color_index + 2) % number_colors],
          colors[(color_index + 1) % number_colors],
          colors[(color_index + 0) % number_colors]};
        cv::Point center{ intRoundUp(keypoints[index] * scale), intRoundUp(keypoints[index + 1] * scale) };
        cv::circle(frame, center, 3, color, -1);
      }
    }
  }
}
