#include "../src/core/ImageSource.hpp"
#include "../src/core/WebcamSource.hpp"
#include <gtest/gtest.h>
#include <opencv2/opencv.hpp>

using namespace visioncore::core;

// ==================== ImageSource Tests ====================

class ImageSourceTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Create test images before each test
    cv::Mat test_img(100, 100, CV_8UC3, cv::Scalar(255, 0, 0));
    cv::imwrite("/tmp/test_image.jpg", test_img);
  }

  void TearDown() override {
    // Cleanup if needed
  }
};

TEST_F(ImageSourceTest, OpenInvalidImage) {
  ImageSource source("nonexistent_file.jpg");
  EXPECT_FALSE(source.open());
  EXPECT_FALSE(source.isOpened());
}

TEST_F(ImageSourceTest, OpenValidImage) {
  ImageSource source("/tmp/test_image.jpg");
  EXPECT_TRUE(source.open());
  EXPECT_TRUE(source.isOpened());
}

TEST_F(ImageSourceTest, GetDimensions) {
  ImageSource source("/tmp/test_image.jpg");
  source.open();

  EXPECT_GT(source.getWidth(), 0);
  EXPECT_GT(source.getHeight(), 0);
  EXPECT_EQ(source.getWidth(), 100);
  EXPECT_EQ(source.getHeight(), 100);
}

TEST_F(ImageSourceTest, GetFPS) {
  ImageSource source("/tmp/test_image.jpg");
  source.open();

  // Static images should return 0.0 FPS
  EXPECT_DOUBLE_EQ(source.getFPS(), 0.0);
}

TEST_F(ImageSourceTest, GetName) {
  ImageSource source("/tmp/test_image.jpg");
  std::string name = source.getName();

  EXPECT_FALSE(name.empty());
  EXPECT_NE(name.find("Image"), std::string::npos);
}

TEST_F(ImageSourceTest, ReadFrame) {
  ImageSource source("/tmp/test_image.jpg");
  source.open();

  cv::Mat frame;
  EXPECT_TRUE(source.readFrame(frame));
  EXPECT_FALSE(frame.empty());
  EXPECT_EQ(frame.rows, 100);
  EXPECT_EQ(frame.cols, 100);
}

TEST_F(ImageSourceTest, ReadFrameConsistency) {
  ImageSource source("/tmp/test_image.jpg");
  source.open();

  cv::Mat frame1, frame2;
  EXPECT_TRUE(source.readFrame(frame1));
  EXPECT_TRUE(source.readFrame(frame2));

  // Both frames should be identical (static image)
  cv::Mat diff;
  cv::absdiff(frame1, frame2, diff);
  double sum = cv::sum(diff)[0];
  EXPECT_DOUBLE_EQ(sum, 0.0);
}

TEST_F(ImageSourceTest, ReadFrameBeforeOpen) {
  ImageSource source("/tmp/test_image.jpg");
  cv::Mat frame;

  // Should fail if not opened
  EXPECT_FALSE(source.readFrame(frame));
}

TEST_F(ImageSourceTest, CloseSource) {
  ImageSource source("/tmp/test_image.jpg");
  source.open();
  EXPECT_TRUE(source.isOpened());

  source.close();
  // Behavior after close depends on implementation
}

// ==================== WebcamSource Tests ====================

class WebcamSourceTest : public ::testing::Test {
protected:
  // Helper to check if webcam is available
  bool isWebcamAvailable() {
    cv::VideoCapture cap(0);
    bool available = cap.isOpened();
    cap.release();
    return available;
  }
};

TEST_F(WebcamSourceTest, Constructor) {
  WebcamSource webcam(0);
  std::string name = webcam.getName();

  EXPECT_FALSE(name.empty());
  EXPECT_NE(name.find("Webcam"), std::string::npos);
}

TEST_F(WebcamSourceTest, OpenWebcam) {
  if (!isWebcamAvailable()) {
    GTEST_SKIP() << "No webcam available, skipping test";
  }

  WebcamSource webcam(0);
  EXPECT_TRUE(webcam.open());
  EXPECT_TRUE(webcam.isOpened());

  webcam.close();
}

TEST_F(WebcamSourceTest, GetDimensions) {
  if (!isWebcamAvailable()) {
    GTEST_SKIP() << "No webcam available, skipping test";
  }

  WebcamSource webcam(0);
  webcam.open();

  EXPECT_GT(webcam.getWidth(), 0);
  EXPECT_GT(webcam.getHeight(), 0);
}

TEST_F(WebcamSourceTest, GetFPS) {
  if (!isWebcamAvailable()) {
    GTEST_SKIP() << "No webcam available, skipping test";
  }

  WebcamSource webcam(0);
  webcam.open();

  // Webcam should have positive FPS
  EXPECT_GT(webcam.getFPS(), 0.0);
}

TEST_F(WebcamSourceTest, ReadFrame) {
  if (!isWebcamAvailable()) {
    GTEST_SKIP() << "No webcam available, skipping test";
  }

  WebcamSource webcam(0);
  webcam.open();

  cv::Mat frame;
  EXPECT_TRUE(webcam.readFrame(frame));
  EXPECT_FALSE(frame.empty());
  EXPECT_EQ(frame.rows, webcam.getHeight());
  EXPECT_EQ(frame.cols, webcam.getWidth());

  webcam.close();
}

TEST_F(WebcamSourceTest, CloseWebcam) {
  if (!isWebcamAvailable()) {
    GTEST_SKIP() << "No webcam available, skipping test";
  }

  WebcamSource webcam(0);
  webcam.open();
  EXPECT_TRUE(webcam.isOpened());

  webcam.close();
  EXPECT_FALSE(webcam.isOpened());
}
