/**
 * @brief ResizeFilter implementation
 */

#include "ResizeFilter.hpp"
#include "utils/Logger.hpp"
#include <opencv2/opencv.hpp>
#include <string>

namespace visioncore::filters {

ResizeFilter::ResizeFilter(const int width, const int height)
    : enabled_(true), desired_width_(width), desired_height_(height) {}

ResizeFilter::~ResizeFilter() = default;

void ResizeFilter::apply(const cv::Mat &input, cv::Mat &output) {

  if (!enabled_) {
    output = input.clone();
    return;
  }

  if (input.cols == desired_width_ && input.rows == desired_height_) {
    output = input.clone();
    return;
  }

  cv::resize(input, output, cv::Size(desired_width_, desired_height_));
}

void ResizeFilter::setParameter(const std::string &name,
                                const nlohmann::json &value) {

  if (name == "width") {
    int new_value = value.get<int>();
    if (new_value <= 0) {
      LOG_WARNING("Invalid width value: " + std::to_string(new_value) +
                  ", must be positive");
      return;
    }
    int old_value = desired_width_;
    desired_width_ = new_value;
    LOG_DEBUG("Width changed from " + std::to_string(old_value) + " to " +
              std::to_string(desired_width_));
  } else if (name == "height") {
    int new_value = value.get<int>();
    if (new_value <= 0) {
      LOG_WARNING("Invalid height value: " + std::to_string(new_value) +
                  ", must be positive");
      return;
    }
    int old_value = desired_height_;
    desired_height_ = new_value;
    LOG_DEBUG("Height changed from " + std::to_string(old_value) + " to " +
              std::to_string(desired_height_));
  } else {
    LOG_WARNING("Unknown parameter: " + name);
  }
}

nlohmann::json ResizeFilter::getParameters() const {
  nlohmann::json params;
  params["width"] = desired_width_;
  params["height"] = desired_height_;
  params["enabled"] = enabled_;
  return params;
}

std::string ResizeFilter::getName() const { return "resize"; }

} // namespace visioncore::filters
