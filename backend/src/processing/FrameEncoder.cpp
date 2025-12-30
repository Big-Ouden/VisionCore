/**
 * @file FrameEncoder.cpp
 * @brief Implementation of FrameEncoder class that encodes video frames.
 */

// FrameEncoder.cpp
#include "processing/FrameEncoder.hpp"

namespace visioncore::processing {

FrameEncoder::FrameEncoder(int jpeg_quality) {
  params_ = {cv::IMWRITE_JPEG_QUALITY, jpeg_quality};
}

FrameEncoder::~FrameEncoder() = default;

bool FrameEncoder::encodeJPEG(const cv::Mat &frame,
                              std::vector<uint8_t> &out_buffer) const {
  if (frame.empty())
    return false;
  return cv::imencode(".jpg", frame, out_buffer, params_);
}

} // namespace visioncore::processing
