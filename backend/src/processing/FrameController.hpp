/**
 * @file FrameController.hpp
 * @brief VisionCore - Frame Processing Controller
 */

#include <atomic>
#include <functional>
#include <memory>
#include <thread>

#include <opencv2/core.hpp>

#include "core/VideoSource.hpp"
#include "pipeline/FramePipeline.hpp"

namespace visioncore::processing {

/**
 * @brief Runtime state of the processing engine.
 */
enum class EngineState {
  STOPPED,  ///< Engine is stopped
  RUNNING,  ///< Engine is running
  STOPPING, ///< Engine is stopping
  ERROR     ///< Engine encountered an error
};

/**
 * @brief Main processing controller.
 *
 * The FrameController owns the complete processing loop:
 *  - frame acquisition from a VideoSource
 *  - frame processing via FramePipeline
 *  - optional frame pacing (FPS limiting)
 *  - delivery via callback
 *
 * The controller runs in its own worker thread.
 * The main thread is only responsible for configuration and UI.
 */
class FrameController {
public:
  /**
   * @brief Callback invoked for each processed frame.
   *
   * @param original  Original input frame (unmodified)
   * @param processed Output frame after pipeline processing
   * @param frame_id  Monotonic frame identifier
   */
  using FrameCallback = std::function<void(
      const cv::Mat &original, const cv::Mat &processed, uint64_t frame_id)>;

  /**
   * @brief Callback invoked on fatal processing errors.
   */
  using ErrorCallback = std::function<void(const std::string &)>;

  /**
   * @brief Construct a FrameController.
   */
  FrameController();

  /**
   * @brief Destructor.
   *
   * Automatically stops the engine if still running.
   */
  ~FrameController();

  /**
   * @brief Start the processing engine.
   *
   * This transfers ownership of the video source to the controller
   * and launches the internal worker thread.
   *
   * @param source     Video source (ownership transferred)
   * @param target_fps Target frames per second (0 = unbounded)
   *
   * @throws std::runtime_error if already running or source fails
   */
  void start(std::unique_ptr<core::VideoSource> source,
             double target_fps = 30.0);

  /**
   * @brief Stop the processing engine.
   *
   * This method blocks until the worker thread exits.
   */
  void stop();

  /**
   * @brief Access the processing pipeline.
   *
   * Allows dynamic configuration of filters.
   *
   * @return Reference to the internal FramePipeline
   */
  pipeline::FramePipeline &getPipeline();

  /**
   * @brief Set the frame delivery callback.
   *
   * @param cb Callback invoked on each processed frame
   */
  void setFrameCallback(FrameCallback cb);

  /**
   * @brief Set the error callback.
   *
   * @param cb Callback invoked on fatal errors
   */
  void setErrorCallback(ErrorCallback cb);

private:
  /**
   * @brief Main worker loop executed in a dedicated thread.
   */
  void workerLoop();

private:
  std::unique_ptr<core::VideoSource> source_;         ///< Video input source
  std::unique_ptr<pipeline::FramePipeline> pipeline_; ///< Processing pipeline

  std::thread worker_;               ///< Worker thread
  std::atomic<bool> running_{false}; ///< Engine running flag

  double target_fps_{30.0}; ///< Target FPS limit

  FrameCallback frame_callback_; ///< Frame output callback
  ErrorCallback error_callback_; ///< Error callback

  uint64_t frame_id_{0}; ///< Frame counter
};

} // namespace visioncore::processing
