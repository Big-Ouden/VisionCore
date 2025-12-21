/**
 * @brief Video source class interface
 */

#ifndef VIDEO_SOURCE_HPP
#define VIDEO_SOURCE_HPP

// #include <opencv2/opencv.hpp>
#include <opencv2/opencv.hpp>
#include <string>

namespace visioncore::core {

class VideoSource {
public:
  virtual ~VideoSource() = default;

  virtual bool open() = 0;
  virtual bool readFrame(cv::Mat &frame) = 0;
  virtual void close() = 0;

  virtual int getWidth() const = 0;
  virtual int getHeight() const = 0;
  virtual double getFPS() const = 0;
  virtual bool isOpened() const = 0;
  virtual std::string getName() const = 0;
};

} // namespace visioncore::core

#endif
