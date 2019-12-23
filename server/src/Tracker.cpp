#include <vector>
#include <set>
#include <iterator>
#include <iostream>
#include "Hungarian.h"
#include "Tracker.hpp"

using namespace std;


Tracker::Tracker() {
  frame_count = 0;
  max_age = 1;
  min_hits = 3;
  iouThreshold = 0.3;
  trkNum = detNum = 0;
  KalmanTracker::kf_count = 0; // tracking id relies on this, so we have to reset it in each seq.
}

// Computes IOU between two bounding boxes
double Tracker::GetIOU(Rect_<float> bb_test, Rect_<float> bb_gt)
{
  float in = (bb_test & bb_gt).area();
  float un = bb_test.area() + bb_gt.area() - in;

  if (un < DBL_EPSILON)
    return 0;

  return (double)(in / un);
}

vector<TrackingBox> Tracker::init(vector<TrackingBox> &detections) {
  frame_count = 1;
  // initialize kalman trackers using first detections.
  for (unsigned int i = 0; i < detections.size(); i++)
  {
    KalmanTracker trk = KalmanTracker(detections[i].box, detections[i].p);
    trackers.push_back(trk);
  }

  // output the first frame detections
  for (unsigned int id = 0; id < detections.size(); id++)
  {
    TrackingBox tb = detections[id];
    tb.id = id + 1;
    tb.p->set_id(tb.id);
    frameTrackingResult.push_back(tb);
    // 
  }
  return frameTrackingResult;
}

vector<TrackingBox> Tracker::update(vector<TrackingBox> &detections) {
  frame_count++;
  // 3.1. get predicted locations from existing trackers.
  predictedBoxes.clear();

  for (auto it = trackers.begin(); it != trackers.end();)
  {
    Rect_<float> pBox = (*it).predict();
    if (pBox.x >= 0 && pBox.y >= 0)
    {
      predictedBoxes.push_back(pBox);
      it++;
    }
    else
    {
      it = trackers.erase(it);
      //cerr << "Box invalid at frame: " << frame_count << endl;
    }
  }

  ///////////////////////////////////////
  // 3.2. associate detections to tracked object (both represented as bounding boxes)
  // dets : detFrameData[fi]
  trkNum = predictedBoxes.size();
  detNum = detections.size();

  iouMatrix.clear();
  iouMatrix.resize(trkNum, vector<double>(detNum, 0));

  for (unsigned int i = 0; i < trkNum; i++) // compute iou matrix as a distance matrix
  {
    for (unsigned int j = 0; j < detNum; j++)
    {
      // use 1-iou because the hungarian algorithm computes a minimum-cost assignment.
      iouMatrix[i][j] = 1 - GetIOU(predictedBoxes[i], detections[j].box);
    }
  }

  // solve the assignment problem using hungarian algorithm.
  // the resulting assignment is [track(prediction) : detection], with len=preNum
  HungarianAlgorithm HungAlgo;
  assignment.clear();
  HungAlgo.Solve(iouMatrix, assignment);

  // find matches, unmatched_detections and unmatched_predictions
  unmatchedTrajectories.clear();
  unmatchedDetections.clear();
  allItems.clear();
  matchedItems.clear();

  if (detNum > trkNum) //	there are unmatched detections
  {
    for (unsigned int n = 0; n < detNum; n++)
      allItems.insert(n);

    for (unsigned int i = 0; i < trkNum; ++i)
      matchedItems.insert(assignment[i]);

    set_difference(allItems.begin(), allItems.end(),
      matchedItems.begin(), matchedItems.end(),
      insert_iterator<set<int>>(unmatchedDetections, unmatchedDetections.begin()));
  }
  else if (detNum < trkNum) // there are unmatched trajectory/predictions
  {
    for (unsigned int i = 0; i < trkNum; ++i)
      if (assignment[i] == -1) // unassigned label will be set as -1 in the assignment algorithm
        unmatchedTrajectories.insert(i);
  }
  else
    ;

  // filter out matched with low IOU
  matchedPairs.clear();
  for (unsigned int i = 0; i < trkNum; ++i)
  {
    if (assignment[i] == -1) // pass over invalid values
      continue;
    if (1 - iouMatrix[i][assignment[i]] < iouThreshold)
    {
      unmatchedTrajectories.insert(i);
      unmatchedDetections.insert(assignment[i]);
    }
    else
      matchedPairs.push_back(cv::Point(i, assignment[i]));
  }

  ///////////////////////////////////////
  // 3.3. updating trackers

  // update matched trackers with assigned detections.
  // each prediction is corresponding to a tracker
  int detIdx, trkIdx;
  for (unsigned int i = 0; i < matchedPairs.size(); i++)
  {
    trkIdx = matchedPairs[i].x;
    detIdx = matchedPairs[i].y;
    trackers[trkIdx].update(detections[detIdx].box);
    trackers[trkIdx].m_p->update(detections[detIdx].p);
  }

  // create and initialise new trackers for unmatched detections
  for (auto umd : unmatchedDetections)
  {
    KalmanTracker tracker = KalmanTracker(detections[umd].box, detections[umd].p);
    trackers.push_back(tracker);
  }

  // get trackers' output
  frameTrackingResult.clear();
  for (auto it = trackers.begin(); it != trackers.end();)
  {
    if (((*it).m_time_since_update < 1) &&
      ((*it).m_hit_streak >= min_hits || frame_count <= min_hits))
    {
      TrackingBox res;
      res.box = (*it).get_state();
      res.id = (*it).m_id + 1;
      res.frame = frame_count;

      // person info update
      res.p = (*it).m_p;
      res.p->set_id(res.id);
      res.p->set_rect(res.box);

      frameTrackingResult.push_back(res);
      it++;
    }
    else
      it++;

    // remove dead tracklet
    if (it != trackers.end() && (*it).m_time_since_update > max_age) {
      // delete person object
      delete (*it).m_p;
      it = trackers.erase(it);
    }
  }

  return frameTrackingResult;
}	// update method end

Tracker::~Tracker() {

}
