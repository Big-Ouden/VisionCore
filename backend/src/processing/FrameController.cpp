/**
 * @file FrmeController.cpp
 * @brief This file contains the implementation of the FrameController class,
 * which
 *
 */

#include "processing/FrameController.hpp"

#include <chrono>
#include <string>
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

// dans FrameController.cpp
void FrameController::setEncodedFrameCallback(EncodedFrameCallback callback) {
  encoded_frame_callback_ = std::move(callback);
}

void FrameController::setEncoder(FrameEncoder encoder) {
  encoder_ = std::move(encoder);
}

void FrameController::workerLoop() {
  cv::Mat input;
  cv::Mat output;

  if (target_fps_ <= 0.0) {
    LOG_WARNING("Target FPS <= 0. Using maximum speed");
  }

  const auto frame_duration = std::chrono::duration<double>(1.0 / target_fps_);
  auto next_frame_time = std::chrono::steady_clock::now();

  size_t dropped_frames = 0;
  double total_frame_time = 0.0;

  while (running_) {

    // Lecture frame
    if (!source_->readFrame(input)) {
      LOG_INFO("End of video stream");
      break;
    }

    cv::Mat input_copy = input.clone();

    auto proc_start = std::chrono::steady_clock::now();
    pipeline_->process(input_copy, output);
    auto proc_end = std::chrono::steady_clock::now();

    double proc_time_ms =
        std::chrono::duration<double, std::milli>(proc_end - proc_start)
            .count();
    total_frame_time += proc_time_ms;

    if (frame_callback_) {
      frame_callback_(input_copy, output, frame_id_);
    }

    if (encoded_frame_callback_) {
      std::vector<uint8_t> buffer;
      if (encoder_.encodeJPEG(output, buffer)) {
        encoded_frame_callback_(buffer);
      }
    }

    ++frame_id_;

    // FPS gestion
    next_frame_time +=
        std::chrono::duration_cast<std::chrono::steady_clock::duration>(
            frame_duration);
    auto now = std::chrono::steady_clock::now();

    if (now > next_frame_time) {
      // Retard, count dropped frames
      size_t skipped = static_cast<size_t>(
          std::chrono::duration<double>(now - next_frame_time).count() /
          frame_duration.count());
      dropped_frames += skipped;
      // Ajust next_frame_time not to accumulate

      next_frame_time =
          now + std::chrono::duration_cast<std::chrono::steady_clock::duration>(
                    frame_duration);
    }

    // sleep until next_frame_time
    if (target_fps_ > 0.0) {
      std::this_thread::sleep_until(next_frame_time);
    }
  }

  if (frame_id_ > 0) {
    double avg_frame_ms = total_frame_time / static_cast<double>(frame_id_);
    double actual_fps = 10.0 / avg_frame_ms;
    LOG_INFO("Frames processed:" + std::to_string(frame_id_) +
             ", dropped: " + std::to_string(dropped_frames) +
             ", avg frame time:" + std::to_string(avg_frame_ms) +
             " ms, approx FPS: " + std::to_string(actual_fps));
  }

  running_ = false;
}

} // namespace visioncore::processing
