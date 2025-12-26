/**
 * @file WebcamSource.hpp
 * @brief VideoSource implementation for webcam/camera devices
 *
 * Captures live video from a physical camera device using OpenCV's
 * VideoCapture. Supports both auto-configuration (query device capabilities)
 * and manual configuration (request specific resolution/FPS).
 */
#include "VideoSource.hpp"

namespace visioncore::core {

class WebcamSource : public VideoSource {
public:
  /**
   * @brief Constructs a webcam source with auto-configuration
   *
   * The actual resolution and FPS will be determined by querying
   * the device after opening.
   *
   * @param device_id Camera device index (0 = default/first camera)
   */
  explicit WebcamSource(int device_id);

  /**
   * @brief Constructs a webcam source with desired configuration
   *
   * Attempts to configure the camera to the specified parameters.
   * Actual values may differ based on device capabilities.
   *
   * @param device_id Camera device index (0 = default/first camera)
   * @param width Desired frame width in pixels
   * @param height Desired frame height in pixels
   * @param fps Desired frames per second
   */
  WebcamSource(int device_id, int width, int height, double fps);

  /**
   * @brief Destructor
   */
  ~WebcamSource() override;

  // VideoSource implementation
  bool open() override;
  bool readFrame(cv::Mat &frame) override;
  void close() override;

  // Property getters - return actual capture device properties
  int getWidth() const override;
  int getHeight() const override;
  double getFPS() const override;
  bool isOpened() const override;
  std::string getName() const override;

private:
  cv::VideoCapture capture_; ///< OpenCV video capture handle for camera I/O
  int device_id_;            ///< Camera device index (0 = default camera)

  int configured_width_;  ///< Actual frame width provided by device
  int configured_height_; ///< Actual frame height provided by device
  double configured_fps_; ///< Actual FPS provided by device
};

} // namespace visioncore::core
