/**
 * @file main.cpp
 * @brief Entry Point for VisionCore pipeline demo application with WebSocket
 * streaming
 */

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <memory>
#include <mutex>
#include <string>

// Core
#include "core/ImageSource.hpp"
#include "core/VideoFileSource.hpp"
#include "core/VideoSource.hpp"
#include "core/WebcamSource.hpp"

// Filters
#include "filters/GrayscaleFilter.hpp"
#include "filters/LUTFilter.hpp"
#include "filters/ResizeFilter.hpp"

// Network
#include "network/WSFrameServer.hpp"

// Pipeline
#include "pipeline/FramePipeline.hpp"
#include "pipeline/PipelineError.hpp"

// Processing
#include "processing/FrameController.hpp"
#include "processing/FrameEncoder.hpp"

// Utils
#include "utils/Logger.hpp"

using namespace visioncore;

/* ============================================================
 * Global state for signal handling
 * ============================================================ */

std::atomic<bool> g_running{true};

void signalHandler(int signal) {
  LOG_INFO("Caught signal " + std::to_string(signal) + ", shutting down...");
  g_running = false;
}

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
            << "  " << programName
            << " --image <path> [--no-display] [--ws-port PORT]\n"
            << "  " << programName
            << " --video <path> [--no-display] [--ws-port PORT]\n"
            << "  " << programName
            << " --webcam <device_id> [--no-display] [--ws-port PORT]\n"
            << "\nOptions:\n"
            << "  --no-display    Disable local OpenCV display window\n"
            << "  --ws-port PORT  WebSocket server port (default: 9001)\n";
}

/* ============================================================
 * Main
 * ============================================================ */

int main(int argc, char *argv[]) {
  // Setup signal handlers
  std::signal(SIGINT, signalHandler);
  std::signal(SIGTERM, signalHandler);

  utils::Logger::instance().setLogLevel(utils::LogLevel::INFO);

  if (argc < 3) {
    printUsage(argv[0]);
    return EXIT_FAILURE;
  }

  // Parse arguments
  const std::string sourceType = argv[1];
  const std::string sourceParam = argv[2];
  bool showDisplay = true;
  int wsPort = 9001;

  // Parse optional flags
  for (int i = 3; i < argc; i++) {
    std::string arg = argv[i];
    if (arg == "--no-display") {
      showDisplay = false;
    } else if (arg == "--ws-port" && i + 1 < argc) {
      wsPort = std::stoi(argv[++i]);
    }
  }

  LOG_INFO("=== VisionCore WebSocket Streaming ===");

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

  /* ------------------------------------------------------------
   * Controller setup
   * ------------------------------------------------------------ */
  processing::FrameController controller;

  /* ------------------------------------------------------------
   * Pipeline configuration
   * ------------------------------------------------------------ */

  auto &pipeline = controller.getPipeline();

  auto resize = std::make_shared<filters::ResizeFilter>(0.5); // 50% scale
  unwrap_or_exit(pipeline.addFilter(resize), "Add Resize Filter");

  auto grayscale = std::make_shared<filters::GrayscaleFilter>();
  unwrap_or_exit(pipeline.addFilter(grayscale), "Add Grayscale Filter");

  auto lut = std::make_shared<filters::LUTFilter>(
      filters::LUTFilter::LUTType::IDENTITY);
  unwrap_or_exit(pipeline.addFilter(lut), "Add LUT Filter");

  /* ------------------------------------------------------------
   * WebSocket server setup
   * ------------------------------------------------------------ */

  network::WSFrameServer wsServer;

  if (!wsServer.start(wsPort)) {
    LOG_ERROR("Failed to start WebSocket server on port " +
              std::to_string(wsPort));
    return EXIT_FAILURE;
  }

  LOG_INFO("WebSocket server started on port " + std::to_string(wsPort));
  LOG_INFO("Connect with: ws://localhost:" + std::to_string(wsPort));

  /* ------------------------------------------------------------
   * Frame encoder setup
   * ------------------------------------------------------------ */

  processing::FrameEncoder encoder(85); // JPEG quality 85

  /* ------------------------------------------------------------
   * Frame callback with WebSocket streaming
   * ------------------------------------------------------------ */

  cv::Mat last_original;
  cv::Mat last_processed;
  std::mutex frame_mutex;
  std::atomic<bool> frame_available{false};

  controller.setFrameCallback([&](const cv::Mat &original,
                                  const cv::Mat &processed, uint64_t frame_id) {
    // Store for local display
    {
      std::lock_guard<std::mutex> lock(frame_mutex);
      last_original = original.clone();
      last_processed = processed.clone();
      frame_available.store(true, std::memory_order_release);
    }

    // Stream via WebSocket if clients connected
    if (wsServer.getClientCount() > 0) {
      std::vector<uint8_t> jpegBuffer;

      if (encoder.encodeJPEG(processed, jpegBuffer)) {
        wsServer.sendFrame(jpegBuffer);
      }
    }
  });

  /* ------------------------------------------------------------
   * Start processing engine
   * ------------------------------------------------------------ */

  controller.start(std::move(source), 30.0);

  LOG_INFO("\nControls:");
  LOG_INFO("  g : toggle grayscale");
  LOG_INFO("  r : change resize factor");
  LOG_INFO("  1 : LUT invert");
  LOG_INFO("  2 : LUT contrast (2.0)");
  LOG_INFO("  3 : LUT brightness (+50)");
  LOG_INFO("  4 : LUT gamma (0.5)");
  LOG_INFO("  5 : LUT logarithmic");
  LOG_INFO("  6 : LUT exponential");
  LOG_INFO("  7 : LUT threshold (128)");
  LOG_INFO("  0 : LUT identity (reset)");
  LOG_INFO("  q / ESC : quit");

  /* ------------------------------------------------------------
   * UI loop (main thread only)
   * ------------------------------------------------------------ */

  int frameDisplayCount = 0;
  auto lastStatsTime = std::chrono::steady_clock::now();

  while (g_running) {

    // Display frame locally if enabled
    if (showDisplay && frame_available.load(std::memory_order_acquire)) {
      cv::Mat o, p;

      {
        std::lock_guard<std::mutex> lock(frame_mutex);
        o = last_original.clone();
        p = last_processed.clone();
        frame_available.store(false, std::memory_order_release);
      }

      if (!o.empty() && !p.empty()) {
        // Convert both to BGR for consistent display
        cv::Mat o_bgr, p_bgr;

        if (o.channels() == 1) {
          cv::cvtColor(o, o_bgr, cv::COLOR_GRAY2BGR);
        } else {
          o_bgr = o.clone();
        }

        if (p.channels() == 1) {
          cv::cvtColor(p, p_bgr, cv::COLOR_GRAY2BGR);
        } else {
          p_bgr = p.clone();
        }

        // IMPORTANT: Resize o_bgr to match p_bgr dimensions (pipeline may have
        // resized)
        if (o_bgr.size() != p_bgr.size()) {
          cv::resize(o_bgr, o_bgr, p_bgr.size());
        }

        cv::Mat display;
        cv::hconcat(o_bgr, p_bgr, display);

        // Add text overlay with stats
        std::string info =
            "Clients: " + std::to_string(wsServer.getClientCount());
        cv::putText(display, info, cv::Point(10, 30), cv::FONT_HERSHEY_SIMPLEX,
                    0.7, cv::Scalar(0, 255, 0), 2);

        cv::imshow("VisionCore - Original | Processed", display);

        frameDisplayCount++;
      }
    }

    // Handle keyboard input
    int key = showDisplay ? cv::waitKey(1) : -1;

    // If no display, sleep to reduce CPU
    if (!showDisplay) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));

      // Check for Ctrl+C
      if (!g_running) {
        break;
      }
    }

    switch (key) {
    case 'g':
    case 'G':
      grayscale->setEnabled(!grayscale->isEnabled());
      LOG_INFO(std::string("Grayscale: ") +
               (grayscale->isEnabled() ? "enabled" : "disabled"));
      break;

    case 'r':
    case 'R': {
      auto params = resize->getParameters();
      double current = params["scale_factor"].get<double>();
      double newScale = (current == 0.5) ? 1.0 : 0.5;
      resize->setParameter("scale_factor", newScale);
      LOG_INFO("Resize scale: " + std::to_string(newScale));
      break;
    }

    case '0':
      lut->setParameter("lut_type", "identity");
      LOG_INFO("LUT: identity (reset)");
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
      lut->setParameter("param", 50.0);
      LOG_INFO("LUT: brightness (+50)");
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
      lut->setParameter("lut_type", "threshold_binary");
      lut->setParameter("param", 128.0);
      LOG_INFO("LUT: threshold (128)");
      break;

    case 'q':
    case 'Q':
    case 27: // ESC
      g_running = false;
      break;
    }

    // Print stats every 5 seconds
    auto now = std::chrono::steady_clock::now();
    auto elapsed =
        std::chrono::duration_cast<std::chrono::seconds>(now - lastStatsTime);
    if (elapsed.count() >= 5) {
      LOG_INFO("Stats - Clients: " + std::to_string(wsServer.getClientCount()) +
               " | Frames displayed: " + std::to_string(frameDisplayCount));
      frameDisplayCount = 0;
      lastStatsTime = now;
    }
  }

  /* ------------------------------------------------------------
   * Shutdown
   * ------------------------------------------------------------ */

  LOG_INFO("Shutting down...");

  controller.stop();
  wsServer.stop();

  if (showDisplay) {
    cv::destroyAllWindows();
    cv::waitKey(100);
  }

  LOG_INFO("Application terminated cleanly");
  return EXIT_SUCCESS;
}
