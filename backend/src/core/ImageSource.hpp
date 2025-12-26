/**
 * @file ImageSource.hpp
 * @brief VideoSource implementation for static images
 *
 * Loads a single image file and returns it repeatedly on each readFrame() call.
 * Useful for testing pipelines with static input or creating slideshow-like
 * behavior. getFPS() returns 0.0 since this is not a time-based source.
 */
#include "VideoSource.hpp"

namespace visioncore::core {

class ImageSource : public VideoSource {
public:
  /**
   * @brief Constructs an image source from a file path
   *
   * The image is not loaded until open() is called.
   *
   * @param image_path Filesystem path to the image file (jpg, png, bmp, etc.)
   */
  explicit ImageSource(const std::string &image_path);

  /**
   * @brief Destructor
   */
  ~ImageSource() override;

  // VideoSource implementation
  bool open() override;
  bool readFrame(cv::Mat &frame) override; // Always returns the same image
  void close() override;

  // Property getters - return static image properties
  int getWidth() const override;
  int getHeight() const override;
  double getFPS() const override; // Returns 0.0 (static image)
  bool isOpened() const override;
  std::string getName() const override;

private:
  std::string image_path_; ///< Filesystem path to the image file
  cv::Mat image_;          ///< Cached image data (loaded once during open())
  bool is_opened_; ///< True if image was successfully loaded, false otherwise
};

} // namespace visioncore::core
