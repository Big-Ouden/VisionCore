/**
 * @brief Abstract interface for video frame sources
 * 
 * Provides a unified interface for different types of video sources
 * (webcams, image files, video files, network streams, etc.).
 * All sources share the same lifecycle: open -> readFrame (loop) -> close.
 */

#ifndef VIDEO_SOURCE_HPP
#define VIDEO_SOURCE_HPP

// #include <opencv2/opencv.hpp>
#include <opencv2/opencv.hpp>
#include <string>

namespace visioncore::core {

class VideoSource {
public:
  virtual ~VideoSource() = default;

  /**
   * @brief Opens the video source and initializes resources
   * @return true if source was successfully opened, false otherwise
   */
  virtual bool open() = 0;

  /**
   * @brief Reads the next frame from the source
   * @param frame Output parameter where the frame will be stored
   * @return true if frame was successfully read, false on error or end of stream
   */
  virtual bool readFrame(cv::Mat &frame) = 0;

  /**
   * @brief Closes the video source and releases all resources
   */
  virtual void close() = 0;

  /**
   * @brief Gets the frame width in pixels
   * @return Frame width, or 0 if source is not opened
   */
  virtual int getWidth() const = 0;

  /**
   * @brief Gets the frame height in pixels
   * @return Frame height, or 0 if source is not opened
   */
  virtual int getHeight() const = 0;

  /**
   * @brief Gets the frames per second rate
   * @return FPS value, or 0.0 for static sources (images)
   */
  virtual double getFPS() const = 0;

  /**
   * @brief Checks if the source is currently opened
   * @return true if source is opened and ready to read frames, false otherwise
   */
  virtual bool isOpened() const = 0;

  /**
   * @brief Gets a human-readable name for the source
   * @return Source name/description (e.g., "Webcam 0", "image.jpg")
   */
  virtual std::string getName() const = 0;
};

} // namespace visioncore::core

#endif
