#include "../src/core/ImageSource.hpp"
#include "../src/core/VideoFileSource.hpp"
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
  EXPECT_EQ("/tmp/test_image.jpg", name);
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

TEST_F(WebcamSourceTest, NoWebcamAvailable) {
  if (!isWebcamAvailable()) {
    WebcamSource webcam(0);
    EXPECT_FALSE(webcam.open());
    EXPECT_FALSE(webcam.isOpened());
  }
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

TEST_F(WebcamSourceTest, ReadFrameWebcamNotOpened) {
  if (!isWebcamAvailable()) {
    GTEST_SKIP() << "No webcam available, skipping test";
  }

  WebcamSource webcam(0);
  // webcam.open();
  cv::Mat frame;

  EXPECT_FALSE(webcam.readFrame(frame));
  EXPECT_TRUE(frame.empty());
  EXPECT_EQ(frame.rows, 0);
  EXPECT_EQ(frame.cols, 0);

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

// ==================== VideoFileSource Tests ====================
class VideoFileSourceTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Create a test video file before each test
    cv::VideoWriter writer("/tmp/test_video.mp4",
                           cv::VideoWriter::fourcc('m', 'p', '4', 'v'), 30,
                           cv::Size(100, 100));
    for (int i = 0; i < 60; ++i) {
      cv::Mat frame(100, 100, CV_8UC3, cv::Scalar(i * 4, i * 4, i * 4));
      writer.write(frame);
    }
    writer.release();
  }
};

TEST_F(VideoFileSourceTest, Constructor) {
  VideoFileSource video("/tmp/test_video.mp4");
  std::string name = video.getName();

  EXPECT_FALSE(name.empty());
  EXPECT_EQ("/tmp/test_video.mp4", name);
}

TEST_F(VideoFileSourceTest, ConstructorLoopEnabled) {
  VideoFileSource video("/tmp/test_video.mp4", true);
  std::string name = video.getName();

  EXPECT_FALSE(name.empty());
  EXPECT_EQ("/tmp/test_video.mp4", name);
  EXPECT_TRUE(video.isLoopEnabled());
}

TEST_F(VideoFileSourceTest, OpenInvalidVideo) {
  VideoFileSource video("nonexistent_file.mp4");
  EXPECT_FALSE(video.open());
  EXPECT_FALSE(video.isOpened());
}

TEST_F(VideoFileSourceTest, OpenValidVideo) {
  VideoFileSource video("/tmp/test_video.mp4", true);
  EXPECT_TRUE(video.open());
  EXPECT_TRUE(video.isOpened());
}

TEST_F(VideoFileSourceTest, GetVideoProperties) {
  VideoFileSource video("/tmp/test_video.mp4");
  video.open();

  EXPECT_EQ(video.getWidth(), 100);
  EXPECT_EQ(video.getHeight(), 100);
  EXPECT_DOUBLE_EQ(video.getFPS(), 30.0);
}

TEST_F(VideoFileSourceTest, getName) {
  VideoFileSource video("/tmp/test_video.mp4");
  std::string name = video.getName();

  EXPECT_FALSE(name.empty());
  EXPECT_EQ("/tmp/test_video.mp4", name);
}

TEST_F(VideoFileSourceTest, ReadFrames) {
  VideoFileSource video("/tmp/test_video.mp4");
  video.open();

  cv::Mat frame;
  int frame_count = 0;
  while (video.readFrame(frame)) {
    EXPECT_FALSE(frame.empty());
    EXPECT_EQ(frame.rows, 100);
    EXPECT_EQ(frame.cols, 100);
    frame_count++;
  }

  EXPECT_EQ(frame_count, 60); // 60 frames in test video
}

TEST_F(VideoFileSourceTest, ReadFrameLoopDisabled) {
  VideoFileSource video("/tmp/test_video.mp4", false);
  video.open();

  cv::Mat frame;
  int frame_count = 0;
  while (video.readFrame(frame)) {
    frame_count++;
  }

  EXPECT_EQ(frame_count, 60); // 60 frames in test video

  // Next read should fail (EOF)
  EXPECT_FALSE(video.readFrame(frame));
}

TEST_F(VideoFileSourceTest, ReadFrameLoopEnabled) {
  VideoFileSource video("/tmp/test_video.mp4", true);
  video.open();

  cv::Mat frame;
  int total_frames_to_read = 120; // Read twice the video length
  int frame_count = 0;
  for (int i = 0; i < total_frames_to_read; ++i) {
    EXPECT_TRUE(video.readFrame(frame));
    EXPECT_FALSE(frame.empty());
    frame_count++;
  }

  EXPECT_EQ(frame_count, total_frames_to_read);
}

TEST_F(VideoFileSourceTest, ReadFrameBeforeOpen) {
  VideoFileSource video("/tmp/test_video.mp4");
  cv::Mat frame;

  // Should fail if not opened
  EXPECT_FALSE(video.readFrame(frame));
}

TEST_F(VideoFileSourceTest, CloseVideo) {
  VideoFileSource video("/tmp/test_video.mp4");
  video.open();
  EXPECT_TRUE(video.isOpened());

  video.close();
  EXPECT_FALSE(video.isOpened());
}

TEST_F(VideoFileSourceTest, CloseVideoNotOpened) {
  VideoFileSource video("/tmp/test_video.mp4");
  // video.open();
  EXPECT_FALSE(video.isOpened());

  // Should not crash or throw
  video.close();
}
