#include "core/WebcamSource.hpp"
#include "utils/Logger.hpp"
#include <chrono>
#include <opencv2/opencv.hpp>
#include <string>
#include <thread>

using namespace visioncore;

int main(int argc, char *argv[]) {
  bool show_gui = false;

  // Parse argument
  if (argc > 1 && std::string(argv[1]) == "--gui") {
    show_gui = true;
  }

  LOG_INFO("=== WebcamSource Test ===");
  LOG_INFO("GUI mode: " + std::string(show_gui ? "enabled" : "disabled"));

  try {
    auto webcam = std::make_unique<core::WebcamSource>(0);

    if (!webcam->open()) {
      LOG_ERROR("Cannot open webcam");
      return 1;
    }

    LOG_INFO("Webcam properties:");
    LOG_INFO("  Size: " + std::to_string(webcam->getWidth()) + "x" +
             std::to_string(webcam->getHeight()));
    LOG_INFO("  FPS: " + std::to_string(webcam->getFPS()));

    cv::Mat frame;
    int frames_captured = 0;

    for (int i = 0; i < 30; ++i) {
      if (!webcam->readFrame(frame)) {
        LOG_ERROR("Failed to read frame");
        break;
      }

      if (frame.empty()) {
        continue;
      }

      frames_captured++;

      // Affichage optionnel
      if (show_gui) {
        cv::imshow("Webcam Test", frame);
        int key = cv::waitKey(33);
        if (key == 'q' || key == 27)
          break;
      } else {
        std::this_thread::sleep_for(std::chrono::milliseconds(33));
      }

      if (i % 10 == 0) {
        LOG_DEBUG("Frame " + std::to_string(i) + " captured");
      }
    }

    LOG_INFO("Total frames: " + std::to_string(frames_captured));
    webcam->close();

    if (show_gui) {
      cv::destroyAllWindows();
      for (int i = 0; i < 10; ++i)
        cv::waitKey(10);
      std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    LOG_INFO("Test completed");
    return 0;

  } catch (const std::exception &e) {
    LOG_CRITICAL("Exception: " + std::string(e.what()));
    if (show_gui)
      cv::destroyAllWindows();
    return 1;
  }
}
