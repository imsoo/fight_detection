#ifndef __DET_INTERFACE
#define __DET_INTERFACE
#include <string>
#include <opencv2/opencv.hpp>

class DetectorInterface {
public:
  virtual void detect(cv::Mat, float thresh) = 0;
  virtual void draw(cv::Mat) = 0;
  virtual std::string det_to_json(int frame_id) = 0;
};
  
#endif
