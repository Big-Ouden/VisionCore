#include "core/WebcamSource.hpp"
#include "utils/Logger.hpp"
#include <QApplication>
#include <chrono>
#include <opencv2/opencv.hpp>
#include <string>
#include <thread>

using namespace visioncore;

int main(int argc, char *argv[]) {

  QApplication app(argc, argv); // OBLIGATOIRE avec HighGUI+Qt

  bool show_gui = true;
  if (argc > 1 && std::string(argv[1]) == "--no-gui")
    show_gui = false;

  LOG_INFO("=== WebcamSource Test ===");
  LOG_INFO("GUI mode: " + std::string(show_gui ? "enabled" : "disabled"));

  try {
    {
      visioncore::core::WebcamSource webcam(0);

      if (!webcam.open()) {
        LOG_ERROR("Cannot open webcam");
        return 1;
      }

      cv::Mat frame;
      for (int i = 0; i < 30; ++i) {
        if (!webcam.readFrame(frame))
          break;

        if (show_gui) {
          cv::imshow("Webcam Test", frame);
          if (cv::waitKey(33) == 27)
            break;
        }
      }

      LOG_INFO("Webcam test finished");
    } //  WebcamSource + VideoCapture détruits ICI

    if (show_gui) {
      cv::destroyAllWindows();
      cv::waitKey(50);
    }

    LOG_INFO("Test completed");
    return 0;

  } catch (const std::exception &e) {
    LOG_CRITICAL(e.what());
    return 1;
  }
}
