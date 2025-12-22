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
   * @Brief Destructor
   */
  ~ResizeFilter() override;

  // ResizeFilter implementation
  void apply(const cv::Mat &input, cv::Mat &output) override;
  void setParameter(const std::string &name,
                    const nlohmann::json &value) override;
  nlohmann::json getParameters() const override;
  std::string getName() const override;

protected:
  bool enabled_ = true;

private:
  int desired_width_;
  int desired_height_;
};

} // namespace visioncore::filters
