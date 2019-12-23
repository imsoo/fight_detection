#pragma once
#include "KalmanTracker.h"
#include <set>
#include "people.hpp"

typedef struct TrackingBox
{
  int frame;
  int id;
  Rect_<float> box;
  Person* p;
}TrackingBox;

class Tracker
{

private:
  int frame_count;
  int max_age;
  int min_hits;
  double iouThreshold;

  unsigned int trkNum;
  unsigned int detNum;

  vector<KalmanTracker> trackers;

  // variables used in the for-loop
  vector<Rect_<float> > predictedBoxes;
  vector<vector<double> > iouMatrix;
  vector<int> assignment;
  set<int> unmatchedDetections;
  set<int> unmatchedTrajectories;
  set<int> allItems;
  set<int> matchedItems;
  vector<cv::Point> matchedPairs;
  vector<TrackingBox> frameTrackingResult;

public:
  Tracker();
  // Computes IOU between two bounding boxes
  double GetIOU(Rect_<float> bb_test, Rect_<float> bb_gt);
  vector<TrackingBox> init(vector<TrackingBox> &detections);
  vector<TrackingBox> update(vector<TrackingBox> &detections);
  ~Tracker();
};
