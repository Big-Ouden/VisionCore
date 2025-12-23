/**
 * @brief Tests for Logger utility
 */

#include "../src/utils/Logger.hpp"
#include <gtest/gtest.h>
#include <sstream>
#include <thread>

using namespace visioncore::utils;

class LoggerTest : public ::testing::Test {
protected:
  std::stringstream buffer;
  std::streambuf *old_cout;

  void SetUp() override {
    // Redirect stdout to our buffer
    old_cout = std::cout.rdbuf(buffer.rdbuf());
    // Reset log level to default
    Logger::instance().setLogLevel(LogLevel::DEBUG);
  }

  void TearDown() override {
    // Restore stdout
    std::cout.rdbuf(old_cout);
  }

  std::string getOutput() { return buffer.str(); }

  void clearOutput() {
    buffer.str("");
    buffer.clear();
  }
};

TEST_F(LoggerTest, DebugLevel) {
  Logger::instance().debug("Debug message");
  std::string output = getOutput();

  EXPECT_NE(output.find("DEBUG"), std::string::npos);
  EXPECT_NE(output.find("Debug message"), std::string::npos);
}

TEST_F(LoggerTest, InfoLevel) {
  Logger::instance().info("Info message");
  std::string output = getOutput();

  EXPECT_NE(output.find("INFO"), std::string::npos);
  EXPECT_NE(output.find("Info message"), std::string::npos);
}

TEST_F(LoggerTest, WarningLevel) {
  Logger::instance().warning("Warning message");
  std::string output = getOutput();

  EXPECT_NE(output.find("WARNING"), std::string::npos);
  EXPECT_NE(output.find("Warning message"), std::string::npos);
}

TEST_F(LoggerTest, ErrorLevel) {
  Logger::instance().error("Error message");
  std::string output = getOutput();

  EXPECT_NE(output.find("ERROR"), std::string::npos);
  EXPECT_NE(output.find("Error message"), std::string::npos);
}

TEST_F(LoggerTest, CriticalLevel) {
  Logger::instance().critical("Critical message");
  std::string output = getOutput();

  EXPECT_NE(output.find("CRITICAL"), std::string::npos);
  EXPECT_NE(output.find("Critical message"), std::string::npos);
}

TEST_F(LoggerTest, FilterByLogLevel_Warning) {
  Logger::instance().setLogLevel(LogLevel::WARNING);

  Logger::instance().debug("Should not appear");
  Logger::instance().info("Should not appear");
  Logger::instance().warning("Should appear");
  Logger::instance().error("Should also appear");

  std::string output = getOutput();

  EXPECT_EQ(output.find("Should not appear"), std::string::npos);
  EXPECT_NE(output.find("Should appear"), std::string::npos);
  EXPECT_NE(output.find("Should also appear"), std::string::npos);
}

TEST_F(LoggerTest, FilterByLogLevel_Error) {
  Logger::instance().setLogLevel(LogLevel::ERROR);

  Logger::instance().debug("Debug filtered");
  Logger::instance().info("Info filtered");
  Logger::instance().warning("Warning filtered");
  Logger::instance().error("Error visible");
  Logger::instance().critical("Critical visible");

  std::string output = getOutput();

  EXPECT_EQ(output.find("Debug filtered"), std::string::npos);
  EXPECT_EQ(output.find("Info filtered"), std::string::npos);
  EXPECT_EQ(output.find("Warning filtered"), std::string::npos);
  EXPECT_NE(output.find("Error visible"), std::string::npos);
  EXPECT_NE(output.find("Critical visible"), std::string::npos);
}

TEST_F(LoggerTest, FilterByLogLevel_Critical) {
  Logger::instance().setLogLevel(LogLevel::CRITICAL);

  Logger::instance().debug("Filtered");
  Logger::instance().info("Filtered");
  Logger::instance().warning("Filtered");
  Logger::instance().error("Filtered");
  Logger::instance().critical("Only this");

  std::string output = getOutput();

  EXPECT_EQ(output.find("Filtered"), std::string::npos);
  EXPECT_NE(output.find("Only this"), std::string::npos);
}

TEST_F(LoggerTest, MessageFormat) {
  Logger::instance().info("Test message");
  std::string output = getOutput();

  // Check for timestamp presence [HH:MM:SS]
  EXPECT_NE(output.find("["), std::string::npos);
  EXPECT_NE(output.find("]"), std::string::npos);
  EXPECT_NE(output.find(":"), std::string::npos);

  // Check for log level in brackets
  EXPECT_NE(output.find("[INFO]"), std::string::npos);

  // Check for message
  EXPECT_NE(output.find("Test message"), std::string::npos);
}

TEST_F(LoggerTest, MacrosWork) {
  LOG_DEBUG("Debug via macro");
  LOG_INFO("Info via macro");
  LOG_WARNING("Warning via macro");
  LOG_ERROR("Error via macro");
  LOG_CRITICAL("Critical via macro");

  std::string output = getOutput();

  EXPECT_NE(output.find("Debug via macro"), std::string::npos);
  EXPECT_NE(output.find("Info via macro"), std::string::npos);
  EXPECT_NE(output.find("Warning via macro"), std::string::npos);
  EXPECT_NE(output.find("Error via macro"), std::string::npos);
  EXPECT_NE(output.find("Critical via macro"), std::string::npos);
}

TEST_F(LoggerTest, ThreadSafety) {
  const int num_threads = 10;
  const int messages_per_thread = 100;

  std::vector<std::thread> threads;

  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back([i, messages_per_thread]() {
      for (int j = 0; j < messages_per_thread; ++j) {
        Logger::instance().info("Thread " + std::to_string(i) + " Message " +
                                std::to_string(j));
      }
    });
  }

  for (auto &thread : threads) {
    thread.join();
  }

  std::string output = getOutput();

  // Verify all messages are present
  int count = 0;
  size_t pos = 0;
  while ((pos = output.find("[INFO]", pos)) != std::string::npos) {
    ++count;
    ++pos;
  }

  EXPECT_EQ(count, num_threads * messages_per_thread);
}

TEST_F(LoggerTest, SingletonPattern) {
  // Verify that instance() always returns the same instance
  Logger &logger1 = Logger::instance();
  Logger &logger2 = Logger::instance();

  EXPECT_EQ(&logger1, &logger2);
}
