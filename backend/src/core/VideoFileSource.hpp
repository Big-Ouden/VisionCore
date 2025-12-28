/**
 * @file VideoFileSource.hpp
 * @brief VideoSource implementation for video files.
 *
 * Takes a video file path and provides frames from the video.
 */

#include "VideoSource.hpp"

namespace visioncore::core {

class VideoFileSource : public VideoSource {
public:
  /**
   * @brief Constructs a video file source from a file path
   *
   * The actual video file is not opened until open() is called.
   *
   * @pram video_path Filesystem path to the video file (mp4, avi, mkv, etc.)
   */
  explicit VideoFileSource(const std::string &video_path, bool loop = false);

  ~VideoFileSource() override;

  // VideoSource implementation
  bool open() override;
  bool readFrame(cv::Mat &frame) override;
  void close() override;

  // Property getters - return actual video properties
  int getWidth() const override;
  int getHeight() const override;
  double getFPS() const override;
  bool isOpened() const override;
  std::string getName() const override;

private:
  cv::VideoCapture capture_; ///< OpenCV video capture handle for file I/O
  std::string video_path_;   ///< Filesystem path to the video file

  int configured_width_;  ///< Actual frame width provided by video
  int configured_height_; ///< Actual frame height provided by video
  double configured_fps_; ///< Actual FPS provided by video
  bool loop_ = false;     ///< Auto-restart video when it ends
};
} // namespace visioncore::core
