/**
 * @file main.cpp
 * @brief Entry Point for VisionCore pipeline demo application
 */

#include <QApplication>

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

#include <cstdlib>
#include <memory>
#include <string>

// Core
#include "core/ImageSource.hpp"
#include "core/VideoSource.hpp"
#include "core/WebcamSource.hpp"

// Filters
#include "filters/GrayscaleFilter.hpp"
#include "filters/ResizeFilter.hpp"

// Pipeline
#include "pipeline/FramePipeline.hpp"
#include "pipeline/PipelineError.hpp"

// Utils
#include "utils/Logger.hpp"

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
   * Pipeline configuration
   * ------------------------------------------------------------ */

  pipeline::FramePipeline pipeline("main");

  unwrap_or_exit(
      pipeline.addFilter(std::make_shared<filters::ResizeFilter>(640, 480)),
      "Add ResizeFilter");

  auto grayscaleFilter = std::make_shared<filters::GrayscaleFilter>();

  unwrap_or_exit(pipeline.addFilter(grayscaleFilter), "Add GrayscaleFilter");

  /* ------------------------------------------------------------
   * Main loop
   * ------------------------------------------------------------ */

  cv::Mat frame;
  cv::Mat processed;

  char displayMode = 'h'; // h = concat, o = original, f = filtered
  bool running = true;

  while (running) {

    if (!source->readFrame(frame)) {
      LOG_WARNING("Failed to read frame");
      break;
    }

    pipeline.process(frame, processed);

    /* ----------------------------------------------------------
     * Safe color handling
     * ---------------------------------------------------------- */

    cv::Mat frameBgr;
    if (frame.channels() == 1) {
      cv::cvtColor(frame, frameBgr, cv::COLOR_GRAY2BGR);
    } else {
      frameBgr = frame;
    }

    cv::Mat processedBgr;
    if (processed.channels() == 1) {
      cv::cvtColor(processed, processedBgr, cv::COLOR_GRAY2BGR);
    } else {
      processedBgr = processed;
    }

    /* ----------------------------------------------------------
     * Display selection
     * ---------------------------------------------------------- */

    cv::Mat display;

    switch (displayMode) {
    case 'h':
      cv::hconcat(frameBgr, processedBgr, display);
      break;
    case 'o':
      display = frameBgr;
      break;
    case 'f':
      display = processedBgr;
      break;
    default:
      display = frameBgr;
      break;
    }

    cv::imshow(source->getName(), display);

    int key = cv::waitKey(1);
    if (key == -1)
      continue;

    switch (key) {
    case 'h':
    case 'H':
      displayMode = 'h';
      LOG_INFO("Display mode: side-by-side");
      break;

    case 'o':
    case 'O':
      displayMode = 'o';
      LOG_INFO("Display mode: original");
      break;

    case 'f':
    case 'F':
      displayMode = 'f';
      LOG_INFO("Display mode: filtered");
      break;

    case 'g':
    case 'G':
      grayscaleFilter->setEnabled(!grayscaleFilter->isEnabled());
      LOG_INFO(std::string("Grayscale filter: ") +
               (grayscaleFilter->isEnabled() ? "enabled" : "disabled"));
      break;

    case 'q':
    case 'Q':
    case 27:
      running = false;
      break;
    }
  }

  /* ------------------------------------------------------------
   * Cleanup
   * ------------------------------------------------------------ */

  source->close();
  cv::destroyAllWindows();
  cv::waitKey(100);

  source.reset();

  LOG_INFO("Application terminated cleanly");
  std::_Exit(EXIT_SUCCESS);
}
