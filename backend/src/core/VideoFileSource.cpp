/**
 * @file VideoFileSource.cpp
 * @brief VideofileSource implementation
 */

#include "VideoFileSource.hpp"
#include "../utils/Logger.hpp"
#include <string>

namespace visioncore::core {

VideoFileSource::VideoFileSource(const std::string &video_path, bool loop)
    : video_path_(video_path), configured_width_(0), configured_height_(0),
      configured_fps_(0), loop_(loop) {}

VideoFileSource::~VideoFileSource() = default;

bool VideoFileSource::open() {
  LOG_INFO("Opening video file " + video_path_);

  if (!capture_.open(video_path_)) {
    LOG_ERROR("Failed to open " + video_path_);
    return false;
  }

  // Query actual device capabilities (may differ from requested values)
  configured_width_ = static_cast<int>(capture_.get(cv::CAP_PROP_FRAME_WIDTH));
  configured_height_ =
      static_cast<int>(capture_.get(cv::CAP_PROP_FRAME_HEIGHT));
  configured_fps_ = capture_.get(cv::CAP_PROP_FPS);

  LOG_INFO("Video file opened: " + std::to_string(configured_width_) + "x" +
           std::to_string(configured_height_) + "@" +
           std::to_string(configured_fps_) + "FPS");

  return true;
}

bool VideoFileSource::readFrame(cv::Mat &frame) {
  if (!capture_.isOpened())
    return false;

  if (capture_.read(frame)) {
    return true;
  }

  // EOF atteint
  if (!loop_) {
    return false;
  }

  // Loop activée : retour au début
  LOG_INFO("Looping video file: " + video_path_);

  capture_.set(cv::CAP_PROP_POS_FRAMES, 0);

  return capture_.read(frame);
}

void VideoFileSource::close() {
  if (capture_.isOpened()) {
    capture_.release();
    LOG_INFO(video_path_ + " source closed");
  } else {
    LOG_INFO(video_path_ + " source already closed");
  }
}

int VideoFileSource::getWidth() const { return configured_width_; }
int VideoFileSource::getHeight() const { return configured_height_; }
double VideoFileSource::getFPS() const { return configured_fps_; }
bool VideoFileSource::isOpened() const { return capture_.isOpened(); }
std::string VideoFileSource::getName() const { return video_path_; }

} // namespace visioncore::core
