/**
 * @file ImageSource.cpp
 * @brief ImageSource implementation
 */
#include "ImageSource.hpp"
#include "../utils/Logger.hpp"
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <string>

namespace visioncore::core {

ImageSource::ImageSource(const std::string &image_path)
    : image_path_(image_path), is_opened_(false) {}

ImageSource::~ImageSource() = default;

bool ImageSource::open() {
  LOG_INFO("Opening image: " + image_path_);

  image_ = cv::imread(image_path_);
  if (image_.empty()) {
    LOG_ERROR("Failed to open image: " + image_path_);
    is_opened_ = false;
  } else {
    is_opened_ = true;
  }

  return is_opened_;
}

bool ImageSource::readFrame(cv::Mat &frame) {
  if (!is_opened_)
    return false;
  image_.copyTo(frame);
  return !frame.empty();
}

void ImageSource::close() {
  if (is_opened_) {
    image_.release();
    LOG_INFO(image_path_ + " closed");
  }
}

int ImageSource::getHeight() const { return image_.rows; }
int ImageSource::getWidth() const { return image_.cols; }
double ImageSource::getFPS() const { return 0.0; }
bool ImageSource::isOpened() const { return is_opened_; }
std::string ImageSource::getName() const { return "Image " + image_path_; }

} // namespace visioncore::core
