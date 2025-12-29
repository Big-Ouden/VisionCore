/**
 * @brief Logger utility for debugging
 */

#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>

namespace visioncore::utils {
enum class LogLevel { DEBUG, INFO, WARNING, ERROR, CRITICAL };

class Logger {
public:
  static Logger &instance() {
    static Logger instance;
    return instance;
  }

  void setLogLevel(LogLevel level) { min_level_ = level; };

  void log(LogLevel level, const std::string &message) {
    if (level < min_level_)
      return;

    std::lock_guard<std::mutex> lock(mutex_);

    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);

    std::cout << "[" << std::put_time(std::localtime(&time), "%H:%M:%S")
              << "] [" << levelToString(level) << "]\t" << message << std::endl;
  }

  void debug(const std::string &msg) { log(LogLevel::DEBUG, msg); }
  void info(const std::string &msg) { log(LogLevel::INFO, msg); }
  void warning(const std::string &msg) { log(LogLevel::WARNING, msg); }
  void error(const std::string &msg) { log(LogLevel::ERROR, msg); }
  void critical(const std::string &msg) { log(LogLevel::CRITICAL, msg); }

private:
  Logger() : min_level_(LogLevel::INFO) {}
  LogLevel min_level_;
  std::mutex mutex_;

  std::string levelToString(LogLevel level) {
    switch (level) {
    case LogLevel::DEBUG:
      return "DEBUG";
    case LogLevel::INFO:
      return "INFO";
    case LogLevel::WARNING:
      return "WARNING";
    case LogLevel::ERROR:
      return "ERROR";
    case LogLevel::CRITICAL:
      return "CRITICAL";
    }
    return "UNKNOWN";
  }
};

#define LOG_DEBUG(msg) visioncore::utils::Logger::instance().debug(msg)
#define LOG_INFO(msg) visioncore::utils::Logger::instance().info(msg)
#define LOG_WARNING(msg) visioncore::utils::Logger::instance().warning(msg)
#define LOG_ERROR(msg) visioncore::utils::Logger::instance().error(msg)
#define LOG_CRITICAL(msg) visioncore::utils::Logger::instance().critical(msg)

} // namespace visioncore::utils

#endif
