/**
 * @file WebcamSource.cpp
 * @brief WebcamSource implementation
 */

#include "WebcamSource.hpp"
#include "../utils/Logger.hpp"
#include <string>

namespace visioncore::core {

WebcamSource::WebcamSource(int device_id)
    : device_id_(device_id), configured_width_(0), configured_height_(0),
      configured_fps_(0) {}

WebcamSource::~WebcamSource() = default;

bool WebcamSource::open() {
  LOG_INFO("Opening webcam device" + std::to_string(device_id_));

  if (!capture_.open(device_id_)) {
    LOG_ERROR("Failed to open camera " + std::to_string(device_id_));
    return false;
  }

  // Query actual device capabilities (may differ from requested values)
  configured_width_ = static_cast<int>(capture_.get(cv::CAP_PROP_FRAME_WIDTH));
  configured_height_ =
      static_cast<int>(capture_.get(cv::CAP_PROP_FRAME_HEIGHT));
  configured_fps_ = capture_.get(cv::CAP_PROP_FPS);

  LOG_INFO("Webcam opened: " + std::to_string(configured_width_) + "x" +
           std::to_string(configured_height_) + " @ " +
           std::to_string(configured_fps_) + " FPS");

  return true;
}

bool WebcamSource::readFrame(cv::Mat &frame) {
  if (!capture_.isOpened())
    return false;
  return capture_.read(frame);
}

void WebcamSource::close() {
  if (capture_.isOpened()) {
    capture_.release();
    LOG_INFO("Webcam closed");
  }
}

int WebcamSource::getWidth() const { return configured_width_; }
int WebcamSource::getHeight() const { return configured_height_; }
double WebcamSource::getFPS() const { return configured_fps_; }
bool WebcamSource::isOpened() const { return capture_.isOpened(); }
std::string WebcamSource::getName() const {
  return "Webcam " + std::to_string(device_id_);
}

} // namespace visioncore::core
