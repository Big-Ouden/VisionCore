/**
 * @file LUTFilter.hpp
 * @brief IFilter implementation for LUT filter
 *
 * Change the pixel values of an image using a Look-Up Table (LUT).
 */
#include "IFilter.hpp"

namespace visioncore::filters {

class LUTFilter : public IFilter {

public:
  enum class LUTType {
    CUSTOM,
    IDENTITY,        ///< No changes
    INVERT,          ///< Negative image
    CONTRAST,        ///< Increase contrast
    BRIGHTNESS,      ///< Increase brightness
    GAMMA,           ///< Gamma correction
    LOGARITHMIC,     ///< Log Transform
    EXPONENTIAL,     ///< Exp Transform
    THRESHOLD_BINARY ///< Binary threshold
  };

  /**
   * @brief Construct the filter with identity
   */
  explicit LUTFilter();

  /**
   * @brief Construct the filter with predefined LUT type
   *
   * @param type Type of LUT to create
   * @param param Generic parameter for LUT creation (e.g., gamma, etc)
   */
  explicit LUTFilter(LUTType type, double param = 1.0);

  /**
   * @brief destructor
   */
  ~LUTFilter() override;

  // LUTFilter implementation
  void apply(const cv::Mat &input, cv::Mat &output) override;
  void setParameter(const std::string &name,
                    const nlohmann::json &value) override;
  nlohmann::json getParameters() const override;
  std::string getName() const override;

private:
  cv::Mat lut_;
  LUTType lut_type_;
  double param_ = 1.0; ///< Generic parameters (gamma, threshold etc)

  // Specific LUTFilter methods

  /**
   * @brief Update the LUT based on current type and parameters
   */
  void updateLUT();

  /**
   * @brief Create identity LUT
   *
   * Identity LUT maps each pixel value to itself.
   */
  void createIdentityLUT();

  /**
   * @brief Create invert LUT
   *
   * Invert LUT maps each pixel value to its inverse (255 - value).
   */
  void createInvertLUT();

  /**
   * @brief Create contrast LUT
   *
   * Contrast LUT increases the contrast of the image based on the given factor.
   * @param factor Contrast adjustment factor. Values > 1 increase contrast,
   * values < 1 decrease contrast.
   */
  void createContrastLUT(double factor);

  /**
   *
   * @brief Create brightness LUT
   *
   * Brightness LUT increases the brightness of the image based on the given
   * offset.
   * @param offset Brightness offset to be added to each pixel value.
   */
  void createBrightnessLUT(double offset);

  /**
   * @brief Create gamma correction LUT
   *
   * Gamma LUT applies gamma correction to the image based on the given gamma
   * value.
   * @param gamma Gamma correction value. Values < 1 brighten the image, values
   * >
   */
  void createGammaLUT(double gamma);

  /**
   * @brief Create logarithmic LUT
   *
   * Logarithmic LUT applies logarithmic transformation to the image.
   */
  void createLogLUT();

  /**
   * @brief Create exponential LUT
   *
   * Exponential LUT applies exponential transformation to the image.
   */
  void createExpLUT();

  /**
   * @brief Create binary threshold LUT
   *
   * threshold Binary LUT applies binary thresholding to the image based on the
   * given threshold value.
   * @param threshold Threshold value for binary segmentation.
   */
  void createThresholdLUT(double threshold);

  /**
   * @brief Set a custom LUT from JSON array
   *
   * Sets a custom LUT based on the provided JSON array.
   * @param lut_json JSON array representing the custom LUT values.
   *
   */
  void setCustomLUT(const nlohmann::json &lut_json);

  /**
   * @brief Return name of the lut type given
   *
   * @param LUTType
   */
  std::string lutTypeToString(LUTType type) const;
};

} // namespace visioncore::filters
