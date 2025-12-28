/**
 * @brief IFilter implementation for resize filter
 *
 * Resize the frame from its size to the specified size
 */

#include "IFilter.hpp"

namespace visioncore::filters {

class ResizeFilter : public IFilter {
public:
  /**
   * @brief Construct the filter
   */
  explicit ResizeFilter(const int width, const int height);

  /**
   * @brief Construct the filter with scale parameters
   */
  explicit ResizeFilter(const double scale);

  /**
   * @Brief Destructor
   */
  ~ResizeFilter() override;

  // ResizeFilter implementation
  void apply(const cv::Mat &input, cv::Mat &output) override;
  void setParameter(const std::string &name,
                    const nlohmann::json &value) override;
  nlohmann::json getParameters() const override;
  std::string getName() const override;

private:
  int desired_width_ = 0;
  int desired_height_ = 0;
  double scale_ = 0.0;
};

} // namespace visioncore::filters
