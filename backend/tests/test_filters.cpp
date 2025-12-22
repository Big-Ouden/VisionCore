#include "../src/filters/GrayscaleFilter.hpp"
#include "../src/filters/ResizeFilter.hpp"
#include <gtest/gtest.h>
#include <opencv2/opencv.hpp>

using namespace visioncore::filters;

// ==================== GrayscaleFilter Tests ====================

class GrayscaleFilterTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Create test image (blue 100x100)
    test_image_ = cv::Mat(100, 100, CV_8UC3, cv::Scalar(255, 0, 0));
  }

  cv::Mat test_image_;
};

TEST_F(GrayscaleFilterTest, Constructor) {
  GrayscaleFilter filter;
  EXPECT_EQ(filter.getName(), "grayscale");
  EXPECT_TRUE(filter.isEnabled());
}

TEST_F(GrayscaleFilterTest, ApplyFilter) {
  GrayscaleFilter filter;
  cv::Mat output;

  filter.apply(test_image_, output);

  EXPECT_FALSE(output.empty());
  EXPECT_EQ(output.channels(), 1); // Grayscale = 1 channel
  EXPECT_EQ(output.rows, test_image_.rows);
  EXPECT_EQ(output.cols, test_image_.cols);
}

TEST_F(GrayscaleFilterTest, DisabledFilter) {
  GrayscaleFilter filter;
  filter.setEnabled(false);

  cv::Mat output;
  filter.apply(test_image_, output);

  // When disabled, should return clone of input
  EXPECT_EQ(output.channels(), test_image_.channels());
}

// ==================== ResizeFilter Tests ====================

class ResizeFilterTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Create test image 640x480
    test_image_ = cv::Mat(480, 640, CV_8UC3, cv::Scalar(0, 255, 0));
  }

  cv::Mat test_image_;
};

TEST_F(ResizeFilterTest, Constructor) {
  ResizeFilter filter(320, 240);
  EXPECT_EQ(filter.getName(), "resize");
  EXPECT_TRUE(filter.isEnabled());
}

TEST_F(ResizeFilterTest, ApplyResize) {
  ResizeFilter filter(320, 240);
  cv::Mat output;

  filter.apply(test_image_, output);

  EXPECT_FALSE(output.empty());
  EXPECT_EQ(output.cols, 320);
  EXPECT_EQ(output.rows, 240);
}

TEST_F(ResizeFilterTest, SetParameterWidth) {
  ResizeFilter filter(640, 480);
  filter.setParameter("width", 800);

  cv::Mat output;
  filter.apply(test_image_, output);

  EXPECT_EQ(output.cols, 800);
  EXPECT_EQ(output.rows, 480);
}

TEST_F(ResizeFilterTest, GetParameters) {
  ResizeFilter filter(800, 600);
  auto params = filter.getParameters();

  EXPECT_TRUE(params.contains("width"));
  EXPECT_TRUE(params.contains("height"));
  
  EXPECT_EQ(params["width"], 800);
  EXPECT_EQ(params["height"], 600);
}


TEST_F(ResizeFilterTest, DisabledFilter) {
  ResizeFilter filter(800,600);
  filter.setEnabled(false);

  cv::Mat output;
  filter.apply(test_image_, output);

  // When disabled, should return clone of input
  EXPECT_EQ(output.rows, test_image_.rows);
  EXPECT_EQ(output.cols, test_image_.cols);
}
