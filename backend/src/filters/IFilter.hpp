/**
 * @brief Abstract interface for image filters
 *
 * Provide a unified interface for differents types of image filters
 * (grayscale filter, resize filter, ascii filter, convolution filter)
 */

#ifndef IFILTER_HPP
#define IFILTER_HPP

#include <nlohmann/json.hpp>
#include <opencv2/opencv.hpp>
#include <string>

namespace visioncore::filters {
class IFilter {
public:
  virtual ~IFilter() = default;

  virtual void apply(const cv::Mat &input, cv::Mat &output) = 0;
  virtual void setParameter(const std::string &name,
                            const nlohmann::json &value) = 0;
  virtual nlohmann::json getParameters() const = 0;
  virtual std::string getName() const = 0;

  void setEnabled(bool enabled) { enabled_ = enabled; }
  bool isEnabled() const { return enabled_; }

protected:
  bool enabled_ = true;
};
} // namespace visioncore::filters

#endif
