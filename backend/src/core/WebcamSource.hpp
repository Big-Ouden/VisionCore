/**
 * @brief Implementation of webcam source class
 */
#include "VideoSource.hpp"

namespace visioncore::core {

class WebcamSource : public VideoSource {
public:
  /**
   * @brief Constructor
   * @param  device_id: peripherical ID (0 = first webcam)
   */
  explicit WebcamSource(int device_id);

  /**
   * @brief Constructor with configuration
   * @param device_id: peripherical ID
   * @param width : aimed width
   * @param height : aimed height
   * @param fps: aimed FPS
   */
  WebcamSource(int device_id, int width, int height, double fps);

  /**
   * @brief Destructor
   */
  ~WebcamSource() override;

  // VideoSource Implementation

  /**
   * @brief Open the source
   * @return true if the source if succesfully opened
   */
  bool open() override;

  /**
   * @brief read a frame
   * @return true if frame succesfully read
   */
  bool readFrame(cv::Mat &frame) override;

  /**
   * @brief close the video source
   * @return nothing
   */
  void close() override;

  /**
   * @brief Width Getter
   * @return frame width
   */
  int getWidth() const override;

  /**
   * @brief Heigth Getter
   * @return frame heigth
   */
  int getHeight() const override;

  /**
   * @brief Source FPS getter
   * @return source fps
   */
  double getFPS() const override;

  /**
   * @brief Check is source is opened
   * @return true if source opened, false else
   */
  bool isOpened() const override;

  /**
   * @brief Get source name
   * @return source name
   */
  std::string getName() const override;

private:
  /**
   * @brief Video capture
   */
  cv::VideoCapture capture_;

  /**
   * @brief device id
   */
  int device_id_;

  /**
   * @brief configured width
   */
  int configured_width_;

  /**
   * @brief configured heigth
   */
  int configured_height_;

  /**
   * @brief configured fps
   */
  double configured_fps_;
};

} // namespace visioncore::core
