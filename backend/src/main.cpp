#include "core/ImageSource.hpp"
#include "core/VideoSource.hpp"
#include "core/WebcamSource.hpp"
#include "filters/GrayscaleFilter.hpp"
#include "filters/IFilter.hpp"
#include "filters/ResizeFilter.hpp"
#include "utils/Logger.hpp"
#include <QApplication>
#include <chrono>
#include <memory>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/opencv.hpp>
#include <string>
#include <thread>

using namespace visioncore;

void printUsage(const std::string &program_name) {
  std::cout << "Usage:" << std::endl;
  std::cout << "\t" + program_name + " --image <path> [--no-gui]" << std::endl;
  std::cout << "\t" + program_name + " --webcam <device_id> [--no-gui]"
            << std::endl;
  std::cout << std::endl;
  std::cout << "Examples: " << std::endl;
  std::cout << "\t" + program_name + " --image ../assets/image.jpg"
            << std::endl;
  std::cout << "\t" + program_name + " --webcam 0" << std::endl;
}

int main(int argc, char *argv[]) {

  utils::Logger::instance().setLogLevel(utils::LogLevel::DEBUG);

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

    // Create filters
    std::unique_ptr<filters::IFilter> resize_filter =
        std::make_unique<filters::ResizeFilter>(640, 480);
    std::unique_ptr<filters::IFilter> grayscale_filter =
        std::make_unique<filters::GrayscaleFilter>();

    LOG_INFO("ResizeFilter: " + resize_filter->getName() +
             ", enabled: " + (resize_filter->isEnabled() ? "true" : "false"));
    LOG_INFO("GrayscaleFilter: " + grayscale_filter->getName() + ", enabled: " +
             (grayscale_filter->isEnabled() ? "true" : "false"));

    // Test setParameter to see DEBUG logs
    LOG_INFO("Testing ResizeFilter parameter changes...");
    resize_filter->setParameter("width", 640);
    resize_filter->setParameter("height", 480);

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
    LOG_INFO("Press 'h' for side-by-side, 'o' for original, 'f' for filtered");

    // Main display loop
    cv::Mat frame, processed;
    char display_mode =
        'h'; // 'h' = horizontal (side-by-side), 'o' = original, 'f' = filtered
    bool running = true;

    while (running) {
      if (!source->readFrame(frame)) {
        LOG_WARNING("Failed to read frame");
        break;
      }

      // Apply filter pipeline: resize -> grayscale
      cv::Mat resized;
      resize_filter->apply(frame, resized);
      grayscale_filter->apply(resized, processed);

      // Use resized as the base for display (both have same dimensions now)
      cv::Mat display_frame = resized;       // Already resized by filter
      cv::Mat display_processed = processed; // Same size as resized

      if (show_gui) {
        // Convert grayscale to BGR for concatenation (if needed)
        cv::Mat processed_bgr;
        if (display_processed.channels() == 1) {
          cv::cvtColor(display_processed, processed_bgr, cv::COLOR_GRAY2BGR);
        } else {
          processed_bgr = display_processed;
        }

        // Choose what to display based on mode
        cv::Mat display;
        switch (display_mode) {
        case 'h': // Side-by-side (horizontal concat)
          cv::hconcat(display_frame, processed_bgr, display);
          break;
        case 'o': // Original only
          display = display_frame;
          break;
        case 'f': // Filtered only
          display = processed_bgr;
          break;
        default:
          cv::hconcat(display_frame, processed_bgr, display);
          break;
        }

        cv::flip(display, display, 1);
        cv::imshow(source->getName(), display);

        // Wait for key press (1ms for webcam, 30ms for image)
        int wait_time = (source->getFPS() > 0) ? 30 : 1;
        int key = cv::waitKey(wait_time);

        // Handle key presses
        if (key != -1) { // -1 means no key pressed
          switch (key) {
          case 'h':
          case 'H':
            display_mode = 'h';
            LOG_INFO("Display mode: Side-by-side");
            break;
          case 'o':
          case 'O':
            display_mode = 'o';
            LOG_INFO("Display mode: Original only");
            break;
          case 'f':
          case 'F':
            display_mode = 'f';
            LOG_INFO("Display mode: Filtered only");
            break;
          case 'q':
          case 'Q':
          case 27: // ESC
            LOG_INFO("User requested quit");
            running = false;
            break;
          }
        }
      } else {
        // If no GUI, just process one frame then exit
        running = false;
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
