/**
 * @brief IFilter implementation for grayscale filter
 *
 * Change the frame from a colored one with 3 channel
 * to a 1-channel cv::Mat. The value of each pixel is
 * calculated with the luminosity method :
 * grayscale = 0.3*red + 0.59*green + 0.11*blue
 */

#include "IFilter.hpp"

namespace visioncore::filters {

class GrayscaleFilter : public IFilter {
public:
  /**
   * @brief Construct a filter
   */
  explicit GrayscaleFilter();

  /**
   * @brief Destructor
   */
  ~GrayscaleFilter() override;

  // Grayscalefilter implementation
  void apply(const cv::Mat &input, cv::Mat &output) override;
  void setParameter(const std::string &name,
                    const nlohmann::json &value) override;

  nlohmann::json getParameters() const override;
  std::string getName() const override;

protected:
  bool enabled_ = true;
};

} // namespace visioncore::filters
