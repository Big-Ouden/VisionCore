// tests/test_framecontroller.cpp
#include "core/VideoFileSource.hpp"
#include "filters/GrayscaleFilter.hpp"
#include "processing/FrameController.hpp"
#include "gtest/gtest.h"
#include <chrono>
#include <memory>
#include <opencv2/opencv.hpp>
#include <thread>

using namespace visioncore::processing;
using namespace visioncore::core;
using namespace visioncore::filters;

// -------------------- FrameController Tests with VideoFileSource
// --------------------

TEST(FrameControllerTest, StartStopController) {
  FrameController controller;
  auto source = std::make_unique<VideoFileSource>("../assets/video.mp4");

  EXPECT_NO_THROW(controller.start(std::move(source), 0.0));
  EXPECT_EQ(controller.getPipeline().size(), 0u);
  EXPECT_NO_THROW(controller.stop());
}

TEST(FrameControllerTest, FrameCallbackCalled) {
  FrameController controller;
  auto source = std::make_unique<VideoFileSource>("../assets/video.mp4");

  controller.getPipeline().addFilter(std::make_shared<GrayscaleFilter>());

  int callback_count = 0;
  controller.setFrameCallback(
      [&callback_count](const cv::Mat &orig, const cv::Mat &proc, uint64_t id) {
        EXPECT_FALSE(orig.empty());
        EXPECT_FALSE(proc.empty());
        ++callback_count;
      });

  controller.start(std::move(source), 0.0);
  std::this_thread::sleep_for(std::chrono::milliseconds(300));
  controller.stop();

  EXPECT_GT(callback_count, 0);
}

TEST(FrameControllerTest, PipelineProcessing) {
  FrameController controller;
  auto source = std::make_unique<VideoFileSource>("../assets/video.mp4");

  controller.getPipeline().addFilter(std::make_shared<GrayscaleFilter>());

  int callback_count = 0;
  controller.setFrameCallback(
      [&callback_count](const cv::Mat &orig, const cv::Mat &proc, uint64_t) {
        EXPECT_EQ(proc.channels(), 1); // Grayscale
        ++callback_count;
      });

  controller.start(std::move(source), 0.0);
  std::this_thread::sleep_for(
      std::chrono::milliseconds(200)); // laisse le thread lire 1-2 frames
  controller.stop();

  EXPECT_GT(callback_count, 0);
}

TEST(FrameControllerTest, FPSLimiting) {
  FrameController controller;
  auto source = std::make_unique<VideoFileSource>("../assets/video.mp4");

  int callback_count = 0;
  controller.setFrameCallback(
      [&callback_count](const cv::Mat &, const cv::Mat &, uint64_t) {
        ++callback_count;
      });

  auto start_time = std::chrono::steady_clock::now();

  controller.start(std::move(source), 2.0); // 2 FPS
  std::this_thread::sleep_for(
      std::chrono::milliseconds(1600)); // laisse passer ~3 frames
  controller.stop();

  auto end_time = std::chrono::steady_clock::now();

  EXPECT_GT(callback_count, 0);

  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                      end_time - start_time)
                      .count();
  EXPECT_GE(duration, 500); // at least ~0.5s for 1 frame at 2 FPS
}
