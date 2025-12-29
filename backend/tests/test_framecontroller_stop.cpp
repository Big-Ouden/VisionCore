
// tests/test_framecontroller_stop_file.cpp
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

TEST(FrameControllerTest, StopDuringProcessingWithVideoFile) {
  FrameController controller;

  // Crée un VideoFileSource à partir d'une courte vidéo de test
  auto source = std::make_unique<VideoFileSource>("../assets/video.mp4");

  int callback_count = 0;
  controller.setFrameCallback(
      [&callback_count](const cv::Mat &, const cv::Mat &, uint64_t) {
        ++callback_count;
        std::this_thread::sleep_for(
            std::chrono::milliseconds(100)); // simule traitement long
      });

  controller.start(std::move(source), 0.0);

  // Attends un peu puis stoppe le contrôleur en plein traitement
  std::this_thread::sleep_for(std::chrono::milliseconds(250));
  controller.stop();

  // Vérifie que le worker thread s'est arrêté et quelques frames ont été
  // traitées
  EXPECT_GE(callback_count, 2); // au moins 2 callbacks
  EXPECT_LE(callback_count, 3); // au plus 3 selon timing
}
