#include "core/ImageSource.hpp"
#include "core/VideoSource.hpp"
#include "core/WebcamSource.hpp"
#include "utils/Logger.hpp"
#include <QApplication>
#include <chrono>
#include <memory>
#include <opencv2/opencv.hpp>
#include <string>
#include <thread>

using namespace visioncore;

void printUsage(const std::string &program_name) {
  std::cout << "Usage:" << std::endl;
  std::cout << "\t" +  program_name + " --image <path> [--no-gui]" << std::endl;
  std::cout << "\t" +  program_name + " --webcam <device_id> [--no-gui]" << std::endl;
  std::cout << std::endl;
  std::cout << "Examples: " << std::endl;
  std::cout << "\t" + program_name + " --image ../assets/image.jpg" << std::endl;
  std::cout << "\t" + program_name + " --webcam 0" << std::endl;
}

int main(int argc, char *argv[]) {

  QApplication app(argc, argv); // OBLIGATOIRE avec HighGUI+Qt

  if (argc < 3) {
    printUsage(argv[0]);
    return 1;
  }

  std::string source_type = argv[1];
  std::string source_param = argv[2];
  bool show_gui = true;

  if (argc > 3 && std::string(argv[3]) == "--no-gui") {
    show_gui = false;
  }

  LOG_INFO("=== VideoSource Test ===");
  LOG_INFO("GUI mode: " + std::string(show_gui ? "enabled" : "disabled"));

  try {
    // Create the appropriate source using polymorphism
    std::unique_ptr<core::VideoSource> source;

    if (source_type == "--image") {
      LOG_INFO("Creating ImageSource: " + source_param);
      source = std::make_unique<core::ImageSource>(source_param);
    } else if (source_type == "--webcam") {
      int device_id = std::stoi(source_param);
      LOG_INFO("Creating WebcamSource: device " + std::to_string(device_id));
      source = std::make_unique<core::WebcamSource>(device_id);
    } else {
      LOG_ERROR("Unknown source type: " + source_type);
      printUsage(argv[0]);
      return 1;
    }

    // Open the source
    if (!source->open()) {
      LOG_ERROR("Failed to open source: " + source->getName());
      return 1;
    }

    LOG_INFO("Source opened: " + source->getName());
    LOG_INFO("Resolution: " + std::to_string(source->getWidth()) + "x" +
             std::to_string(source->getHeight()));
    LOG_INFO("FPS: " + std::to_string(source->getFPS()));
    LOG_INFO("Press 'q' or ESC to quit");

    // Main display loop
    cv::Mat frame;
    while (true) {
      if (!source->readFrame(frame)) {
        LOG_WARNING("Failed to read frame");
        break;
      }

      // Resize for display (optional)
      cv::Mat display_frame;
      double scale = 1.0;
      if (dynamic_cast<core::ImageSource*>(source.get())){
        scale = 0.5;
      }
      cv::resize(frame, display_frame, cv::Size(), scale, scale);

      if (show_gui) {
        cv::imshow(source->getName(), display_frame);

        // Wait for key press (1ms for webcam, 30ms for image)
        int wait_time = (source->getFPS() > 0) ? 30 : 1;
        int key = cv::waitKey(wait_time);

        if (key == 'q' || key == 'Q' || key == 27) { // 'q' or ESC
          LOG_INFO("User requested quit");
          break;
        }
      } else {
        // If no GUI, just process one frame then exit
        break;
      }
    }

    source->close();
    LOG_INFO("Source closed");

    if (show_gui) {
      cv::destroyAllWindows();
      cv::waitKey(50);
    }

    LOG_INFO("Test completed successfully");
    return 0;

  } catch (const std::exception &e) {
    LOG_CRITICAL("Exception: " + std::string(e.what()));
    return 1;
  }
}
