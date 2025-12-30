/**
 * @file main.cpp
 * @brief Entry Point for VisionCore pipeline demo application
 */

#include <QApplication>

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

// Core
#include "core/ImageSource.hpp"
#include "core/VideoFileSource.hpp"
#include "core/VideoSource.hpp"
#include "core/WebcamSource.hpp"

// Filters
#include "filters/GrayscaleFilter.hpp"
#include "filters/LUTFilter.hpp"
#include "filters/ResizeFilter.hpp"

// Pipeline
#include "pipeline/FramePipeline.hpp"
#include "pipeline/PipelineError.hpp"

// Processing
#include "processing/FrameController.hpp"

// Utils
#include "utils/Logger.hpp"

#include <chrono>
#include <thread>

using Clock = std::chrono::steady_clock;

using namespace visioncore;

/* ============================================================
 * Error handling helpers
 * ============================================================ */

template <typename T>
T unwrap_or_exit(pipeline::PipelineResult<T> &&res,
                 const std::string &context) {
  if (res.isErr()) {
    LOG_CRITICAL(context + ": " + res.errorType() +
                 (res.message.empty() ? "" : " — " + res.message));
    std::exit(EXIT_FAILURE);
  }
  return std::move(res.value);
}

inline void unwrap_or_exit(pipeline::PipelineResult<void> &&res,
                           const std::string &context) {
  if (res.isErr()) {
    LOG_CRITICAL(context + ": " + res.errorType() +
                 (res.message.empty() ? "" : " — " + res.message));
    std::exit(EXIT_FAILURE);
  }
}

/* ============================================================
 * Usage
 * ============================================================ */

void printUsage(const std::string &programName) {
  std::cout << "Usage:\n"
            << "  " << programName << " --image <path>\n"
            << "  " << programName << " --video <path>\n"
            << "  " << programName << " --webcam <device_id>\n";
}

/* ============================================================
 * Main
 * ============================================================ */

int main(int argc, char *argv[]) {

  utils::Logger::instance().setLogLevel(utils::LogLevel::INFO);
  QApplication app(argc, argv);

  if (argc < 3) {
    printUsage(argv[0]);
    return EXIT_FAILURE;
  }

  const std::string sourceType = argv[1];
  const std::string sourceParam = argv[2];

  /* ------------------------------------------------------------
   * Source creation
   * ------------------------------------------------------------ */

  std::unique_ptr<core::VideoSource> source;

  if (sourceType == "--image") {
    source = std::make_unique<core::ImageSource>(sourceParam);
  } else if (sourceType == "--webcam") {
    source = std::make_unique<core::WebcamSource>(std::stoi(sourceParam));
  } else if (sourceType == "--video") {
    source = std::make_unique<core::VideoFileSource>(sourceParam, true);
  } else {
    LOG_CRITICAL("Unknown source type");
    printUsage(argv[0]);
    return EXIT_FAILURE;
  }

  if (!source->open()) {
    LOG_CRITICAL("Failed to open source");
    return EXIT_FAILURE;
  }

  LOG_INFO("Source opened: " + source->getName());
  LOG_INFO("Press 'h' (side-by-side), 'o' (original), 'f' (filtered)");
  LOG_INFO("Press 'g' to toggle grayscale filter");
  LOG_INFO("Press 'q' or ESC to quit");

  /* ------------------------------------------------------------
   * Controller setup
   * ------------------------------------------------------------ */
  processing::FrameController controller;

  /* ------------------------------------------------------------
   * Pipeline configuration
   * ------------------------------------------------------------ */

  auto &pipeline = controller.getPipeline();
  auto grayscale = std::make_shared<filters::GrayscaleFilter>();
  unwrap_or_exit(pipeline.addFilter(grayscale), "Add Grayscale Filter");

  auto lut =
      std::make_shared<filters::LUTFilter>(filters::LUTFilter::LUTType::INVERT);
  unwrap_or_exit(pipeline.addFilter(lut), "Add LUT Filter");

  unwrap_or_exit(
      pipeline.addFilter(std::make_shared<filters::ResizeFilter>(1.0)),
      "Add Resize Filter");

  /* ------------------------------------------------------------
   * Frame callback
   * ------------------------------------------------------------ */

  cv::Mat last_original;
  cv::Mat last_processed;
  std::mutex frame_mutex;
  std::atomic<bool> frame_available{false};

  controller.setFrameCallback([&](const cv::Mat &original,
                                  const cv::Mat &processed,
                                  uint64_t /*frame_id*/) {
    std::lock_guard<std::mutex> lock(frame_mutex);
    last_original = original.clone();
    last_processed = processed.clone();
    frame_available.store(true, std::memory_order_release);
  });

  /* ------------------------------------------------------------
   * Start processing engine
   * ------------------------------------------------------------ */

  controller.start(std::move(source), 30.0);

  LOG_INFO("Controls:");
  LOG_INFO("  g : toggle grayscale");
  LOG_INFO("  1 : invert");
  LOG_INFO("  2 : contrast (2.0)");
  LOG_INFO("  3 : brightness (+1.5)");
  LOG_INFO("  4 : gamma (0.5)");
  LOG_INFO("  5 : logarithmic");
  LOG_INFO("  6 : exponential");
  LOG_INFO("  7 : threshold (128)");
  LOG_INFO("  q / ESC : quit");

  /* ------------------------------------------------------------
   * UI loop (main thread only)
   * ------------------------------------------------------------ */

  bool running = true;

  while (running) {

    if (frame_available.load(std::memory_order_acquire)) {

      cv::Mat o, p;

      {
        std::lock_guard<std::mutex> lock(frame_mutex);
        o = last_original;
        p = last_processed;
        frame_available.store(false, std::memory_order_release);
      }

      if (!o.empty() && !p.empty()) {

        if (o.channels() == 1)
          cv::cvtColor(o, o, cv::COLOR_GRAY2BGR);

        if (p.channels() == 1)
          cv::cvtColor(p, p, cv::COLOR_GRAY2BGR);

        cv::Mat display;
        cv::hconcat(o, p, display);

        cv::imshow("VisionCore", display);
      }
    }

    int key = cv::waitKey(10);

    switch (key) {

    case 'g':
    case 'G':
      grayscale->setEnabled(!grayscale->isEnabled());
      LOG_INFO(std::string("Grayscale: ") +
               (grayscale->isEnabled() ? "enabled" : "disabled"));
      break;

    case '1':
      lut->setParameter("lut_type", "invert");
      LOG_INFO("LUT: invert");
      break;

    case '2':
      lut->setParameter("lut_type", "contrast");
      lut->setParameter("param", 2.0);
      LOG_INFO("LUT: contrast (2.0)");
      break;

    case '3':
      lut->setParameter("lut_type", "brightness");
      lut->setParameter("param", 1.5);
      LOG_INFO("LUT: brightness (+1.5)");
      break;

    case '4':
      lut->setParameter("lut_type", "gamma");
      lut->setParameter("param", 0.5);
      LOG_INFO("LUT: gamma (0.5)");
      break;

    case '5':
      lut->setParameter("lut_type", "logarithmic");
      LOG_INFO("LUT: logarithmic");
      break;

    case '6':
      lut->setParameter("lut_type", "exponential");
      LOG_INFO("LUT: exponential");
      break;

    case '7':
      lut->setParameter("lut_type", "threshold");
      lut->setParameter("param", 128);
      LOG_INFO("LUT: threshold (128)");
      break;

    case 'q':
    case 'Q':
    case 27:
      running = false;
      break;
    }
  }

  /* ------------------------------------------------------------
   * Shutdown
   * ------------------------------------------------------------ */

  controller.stop();

  cv::destroyAllWindows();
  cv::waitKey(100);

  LOG_INFO("Application terminated cleanly");

  controller.stop();
  return EXIT_SUCCESS;
}
