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

  /**
   * @brief Enable or disable the filter
   * @param enabled True to enable, false to disable
   */
  virtual void setEnabled(bool enabled) { enabled_ = enabled; }
  
  /**
   * @brief Check if the filter is enabled
   * @return True if enabled, false otherwise
   */
  virtual bool isEnabled() const { return enabled_; }

protected:
  bool enabled_ = true; ///< Filter enabled state
};
} // namespace visioncore::filters

#endif
