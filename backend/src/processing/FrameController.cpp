/**
 * @file FrmeController.cpp
 * @brief This file contains the implementation of the FrameController class,
 * which
 *
 */

#include "processing/FrameController.hpp"

#include <chrono>
#include <thread>

#include "utils/Logger.hpp"

namespace visioncore::processing {

FrameController::FrameController() {
  pipeline_ = std::make_unique<pipeline::FramePipeline>("main");
}

FrameController::~FrameController() = default;

pipeline::FramePipeline &FrameController::getPipeline() { return *pipeline_; }

void FrameController::setFrameCallback(FrameCallback cb) {
  frame_callback_ = std::move(cb);
}

void FrameController::setErrorCallback(ErrorCallback cb) {
  error_callback_ = std::move(cb);
}

void FrameController::start(std::unique_ptr<core::VideoSource> source,
                            double target_fps) {
  if (running_) {
    throw std::runtime_error("FrameController already running");
  }

  source_ = std::move(source);
  target_fps_ = target_fps;

  if (!source_ || !source_->open()) {
    throw std::runtime_error("Failed to open video source");
  }

  running_ = true;
  worker_ = std::thread(&FrameController::workerLoop, this);
}

void FrameController::stop() {
  if (!running_)
    return;

  running_ = false;

  if (worker_.joinable())
    worker_.join();

  if (source_)
    source_->close();
}

void FrameController::workerLoop() {
  cv::Mat input;
  cv::Mat output;

  const auto frame_duration = std::chrono::duration<double>(1.0 / target_fps_);

  auto next_frame_time = std::chrono::steady_clock::now();

  while (running_) {

    next_frame_time +=
        std::chrono::duration_cast<std::chrono::steady_clock::duration>(
            frame_duration);

    if (!source_->readFrame(input)) {
      LOG_INFO("End of video stream");
      break;
    }

    // Ensure the original frame remains stable for callbacks
    cv::Mat input_copy = input.clone();

    pipeline_->process(input_copy, output);

    if (frame_callback_) {
      frame_callback_(input_copy, output, frame_id_);
    }

    ++frame_id_;

    if (target_fps_ > 0.0) {
      std::this_thread::sleep_until(next_frame_time);
    }
  }

  running_ = false;
}

} // namespace visioncore::processing
