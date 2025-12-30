/**
 * @file FrameEncoder.hpp
 * @brief Header file for FrameEncoder class that encodes video frames.
 */

#include <opencv2/opencv.hpp>
#include <string>
#include <vector>

#ifndef FRAME_ENCODER_HPP
#define FRAME_ENCODER_HPP

namespace visioncore::processing {

class FrameEncoder {
public:
  /**
   * @brief Constructs a FrameEncoder with specified quality.
   *
   * @param quality Compression quality (0-100).
   */
  FrameEncoder(int quality = 95);

  /**
   * @brief destructor
   */
  ~FrameEncoder();

  /**
   * @brief Encodes a video frame to a compressed format.
   *
   * @param frame Input video frame as cv::Mat.
   * @param out_buffer Output buffer to hold the encoded data.
   */
  bool encodeJPEG(const cv::Mat &frame, std::vector<uint8_t> &out_buffer) const;

private:
  std::vector<int> params_;
};

} // namespace visioncore::processing

#endif // FRAME_ENCODER_HPP
