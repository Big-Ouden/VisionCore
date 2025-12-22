/**
 * @brief GrayscaleFilter implementation
 */
#include "GrayscaleFilter.hpp"
#include "utils/Logger.hpp"
#include <opencv2/opencv.hpp>

namespace visioncore::filters {

GrayscaleFilter::GrayscaleFilter() = default;

GrayscaleFilter::~GrayscaleFilter() = default;

void GrayscaleFilter::apply(const cv::Mat &input, cv::Mat &output) {
  if (!enabled_) {
    output = input.clone();
    return;
  }

  if (input.channels() == 1) {
    output = input.clone();
  } else {
    cv::cvtColor(input, output, cv::COLOR_BGR2GRAY);
  }
}

void GrayscaleFilter::setParameter([[maybe_unused]] const std::string &name,
                                   [[maybe_unused]] const nlohmann::json &value) {
  LOG_WARNING("Grayscale Filter has no parameters");
}

nlohmann::json GrayscaleFilter::getParameters() const {
  nlohmann::json params;
  params["enabled"] = enabled_;
  return params;
}

std::string GrayscaleFilter::getName() const { return "grayscale"; }

} // namespace visioncore::filters
