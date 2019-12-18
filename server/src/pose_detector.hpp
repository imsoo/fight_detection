#ifndef __POSE_DETECTOR
#define __POSE_DETECTOR
#include <vector>
#include <string>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include "yolo_v2_class.hpp"
#include "DetectorInterface.hpp"
#include "people.hpp"

class PoseDetector : public Detector, public DetectorInterface
{
  private:
    int net_inw;
    int net_inh;
    int net_outw;
    int net_outh;
    People* det_people;
  public:
    PoseDetector(const char *cfg_path, const char *weight_path, int gpu_id);
    ~PoseDetector();
    inline People* get_people(void) { return det_people; }
    virtual void detect(cv::Mat mat, float thresh);
    virtual void draw(cv::Mat mat);
    virtual std::string det_to_json(int frame_id);
  private:
    void connect_bodyparts
      (
       std::vector<float>& pose_keypoints,
       const float* const map,
       const float* const peaks,
       int mapw,
       int maph,
       const int inter_min_above_th,
       const float inter_th,
       const int min_subset_cnt,
       const float min_subset_score,
       std::vector<int>& keypoint_shape
      );

    void find_heatmap_peaks
      (
       const float *src,
       float *dst,
       const int SRCW,
       const int SRCH,
       const int SRC_CH,
       const float TH
      );

    cv::Mat create_netsize_im
      (
       const cv::Mat &im,
       const int netw,
       const int neth,
       float *scale
      );
};

#endif
