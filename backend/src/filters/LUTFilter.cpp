/**
 * @file LUTFilter.cpp
 * @brief LUTFiltre implementation
 *
 * Change the pixel values of an image using a Look-Up Table (LUT).
 */
#include "LUTFilter.hpp"
#include "../utils/Logger.hpp"
#include <cmath>
#include <cstdint>
#include <opencv2/core/hal/interface.h>
#include <opencv2/core/saturate.hpp>
#include <opencv2/opencv.hpp>
#include <string>

namespace visioncore::filters {

LUTFilter::LUTFilter() : lut_type_(LUTType::IDENTITY) { createIdentityLUT(); }

LUTFilter::LUTFilter(LUTType type, double param)
    : lut_type_(type), param_(param) {
  updateLUT();
}

LUTFilter::~LUTFilter() = default;

void LUTFilter::apply(const cv::Mat &input, cv::Mat &output) {
  if (!enabled_) {
    output = input.clone();
    return;
  }

  if (input.empty() || lut_.empty()) {
    output = input.clone();
    return;
  }

  cv::LUT(input, lut_, output);
}

void LUTFilter::setParameter(const std::string &name,
                             const nlohmann::json &value) {
  if (name == "lut_type") {
    std::string type_str = value.get<std::string>();

    if (type_str == "identity") {
      lut_type_ = LUTType::IDENTITY;
    } else if (type_str == "invert") {
      lut_type_ = LUTType::INVERT;

    } else if (type_str == "contrast") {
      lut_type_ = LUTType::CONTRAST;

    } else if (type_str == "brightness") {
      lut_type_ = LUTType::BRIGHTNESS;

    } else if (type_str == "gamma") {
      lut_type_ = LUTType::GAMMA;

    } else if (type_str == "logarithmic") {
      lut_type_ = LUTType::LOGARITHMIC;

    } else if (type_str == "exponential") {
      lut_type_ = LUTType::EXPONENTIAL;

    } else if (type_str == "threshold_binary") {
      lut_type_ = LUTType::THRESHOLD_BINARY;

    } else {
      LOG_WARNING("Unknown LUT type : " + type_str);
      return;
    }

    updateLUT();

  } else if (name == "param") {
    param_ = value.get<double>();
    updateLUT();
  } else if (name == "custom_lut") {
    // expect array of 256 value [0-255]
    if (value.is_array() && value.size() == 256) {
      setCustomLUT(value);
    } else {
      LOG_WARNING("Custom LUT must be an array of 256 values");
    }
  } else {
    LOG_WARNING("Unknown parameter : " + name);
  }
}

nlohmann::json LUTFilter::getParameters() const {
  nlohmann::json params;
  params["lut_type"] = lutTypeToString(lut_type_);
  params["param"] = param_;
  params["enabled"] = enabled_;
  return params;
}

std::string LUTFilter::getName() const { return "lut"; }

void LUTFilter::updateLUT() {

  switch (lut_type_) {
  case LUTType::IDENTITY:
    createIdentityLUT();
    break;
  case LUTType::INVERT:
    createInvertLUT();
    break;
  case LUTType::CONTRAST:
    createContrastLUT(param_);
    break;
  case LUTType::BRIGHTNESS:
    createBrightnessLUT(param_);
    break;
  case LUTType::GAMMA:
    createGammaLUT(param_);
    break;
  case LUTType::LOGARITHMIC:
    createLogLUT();
    break;
  case LUTType::EXPONENTIAL:
    createExpLUT();
    break;
  case LUTType::THRESHOLD_BINARY:
    createThresholdLUT(param_);
    break;
  case LUTType::CUSTOM:
    break;
  }
}

void LUTFilter::createIdentityLUT() {
  lut_ = cv::Mat(1, 256, CV_8U);
  for (int i = 0; i < 256; i++) {
    lut_.at<uint8_t>(i) = static_cast<uint8_t>(i);
  }
}

void LUTFilter::createInvertLUT() {
  lut_ = cv::Mat(1, 256, CV_8U);

  for (int i = 0; i < 256; i++) {
    lut_.at<uint8_t>(i) = static_cast<uint8_t>(255 - i);
  }
}

void LUTFilter::createContrastLUT(double factor) {
  // factor > 1.0 increases contrast, < 1.0 decreases
  lut_ = cv::Mat(1, 256, CV_8U);

  for (int i = 0; i < 256; i++) {
    int value = static_cast<int>(factor * (i - 128) + 128);
    lut_.at<uint8_t>(i) = cv::saturate_cast<uint8_t>(value);
  }
}

void LUTFilter::createBrightnessLUT(double offset) {
  // offset in range [-255, 255]
  lut_ = cv::Mat(1, 256, CV_8U);

  for (int i = 0; i < 256; i++) {
    int value = static_cast<int>(i + offset);
    lut_.at<uint8_t>(i) = cv::saturate_cast<uint8_t>(value);
  }
}

void LUTFilter::createGammaLUT(double gamma) {
  // gamma > 1.0 darkens, < 1.0 brightens
  lut_ = cv::Mat(1, 256, CV_8U);

  for (int i = 0; i < 256; i++) {
    double normalized = i / 255.0;
    double corrected = std::pow(normalized, gamma);
    lut_.at<uint8_t>(i) = cv::saturate_cast<uint8_t>(corrected * 255.0);
  }
}

void LUTFilter::createLogLUT() {
  // Logarithmic transform (brightens dark areas)
  lut_ = cv::Mat(1, 256, CV_8U);

  double c = 255.0 / std::log(1 + 255.0);
  for (int i = 0; i < 256; i++) {
    double value = c * std::log(i + 1);
    lut_.at<uint8_t>(i) = cv::saturate_cast<uint8_t>(value);
  }
}

void LUTFilter::createExpLUT() {
  lut_ = cv::Mat(1, 256, CV_8U);

  for (int i = 0; i < 256; i++) {
    double normalized = i / 255.0;
    double value = 255.0 * (std::exp(normalized) - 1) / (std::exp(1.0) - 1);
    lut_.at<uint8_t>(i) = cv::saturate_cast<uint8_t>(value);
  }
}

void LUTFilter::createThresholdLUT(double threshold) {

  lut_ = cv::Mat(1, 256, CV_8U);

  for (int i = 0; i < 256; i++) {
    lut_.at<uint8_t>(i) = (i >= threshold) ? 255 : i;
  }
}

void LUTFilter::setCustomLUT(const nlohmann::json &lut_json) {

  lut_ = cv::Mat(1, 256, CV_8U);

  for (int i = 0; i < 256; i++) {
    int value = lut_json[i].get<int>();
    lut_.at<uint8_t>(i) = cv::saturate_cast<uint8_t>(value);
  }
  lut_type_ = LUTType::CUSTOM;
}

std::string LUTFilter::lutTypeToString(LUTType type) const {
  switch (type) {
  case LUTType::IDENTITY:
    return "identity";
  case LUTType::INVERT:
    return "invert";
  case LUTType::CONTRAST:
    return "contrast";
  case LUTType::BRIGHTNESS:
    return "brightness";
  case LUTType::GAMMA:
    return "gamma";
  case LUTType::LOGARITHMIC:
    return "logarithmic";
  case LUTType::EXPONENTIAL:
    return "exponential";
  case LUTType::THRESHOLD_BINARY:
    return "threshold_binary";
  case LUTType::CUSTOM:
    return "custom";
  default:
    return "unknown";
  }
}

} // namespace visioncore::filters
