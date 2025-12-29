#include "../src/filters/GrayscaleFilter.hpp"
#include "../src/filters/ResizeFilter.hpp"
#include <gtest/gtest.h>
#include <opencv2/opencv.hpp>
#include <string>

using namespace visioncore::filters;

// ==================== GrayscaleFilter Tests ====================

class GrayscaleFilterTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Create test image (blue 100x100)
    test_image_ = cv::Mat(100, 100, CV_8UC3, cv::Scalar(255, 0, 0));
    test_image_gray_ = cv::Mat(100, 100, CV_8UC1);
  }

  cv::Mat test_image_;
  cv::Mat test_image_gray_;
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

TEST_F(GrayscaleFilterTest, ApplyToGrayImage) {
  GrayscaleFilter filter;

  cv::Mat output;
  filter.apply(test_image_gray_, output);

  EXPECT_EQ(output.channels(), test_image_gray_.channels());
  EXPECT_EQ(output.rows, test_image_gray_.rows);
  EXPECT_EQ(output.cols, test_image_gray_.cols);
}

TEST_F(GrayscaleFilterTest, SetParameter) {
  GrayscaleFilter filter;

  // GrayscaleFilter has no configurable parameters
  // Test that setParameter doesn't crash
  filter.setParameter("any_param", 42);
  filter.setParameter("another", "value");

  // Filter should still work normally
  cv::Mat output;
  filter.apply(test_image_, output);
  EXPECT_FALSE(output.empty());
  EXPECT_EQ(output.channels(), 1);
}

TEST_F(GrayscaleFilterTest, GetParameters) {
  GrayscaleFilter filter;
  auto params = filter.getParameters();

  // GrayscaleFilter has no parameters, should return empty or minimal JSON
  EXPECT_TRUE(params.is_object());
}

TEST_F(GrayscaleFilterTest, GetName) {
  GrayscaleFilter filter;
  EXPECT_EQ(filter.getName(), "grayscale");
}

TEST_F(GrayscaleFilterTest, ApplyToEmptyImage) {
  cv::Mat empty_image;

  GrayscaleFilter filter;
  filter.setEnabled(true);
  cv::Mat output;
  filter.apply(empty_image, output);

  // Both should be empty after applying filter to empty input
  EXPECT_TRUE(empty_image.empty());
  EXPECT_TRUE(output.empty());
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

TEST_F(ResizeFilterTest, ConstructorWH) {
  ResizeFilter filter(320, 240);
  EXPECT_EQ(filter.getName(), "resize");
  EXPECT_TRUE(filter.isEnabled());
}

TEST_F(ResizeFilterTest, ConstructorScale) {
  ResizeFilter filter(0.5);
  EXPECT_EQ(filter.getName(), "resize");
  EXPECT_TRUE(filter.isEnabled());
}

TEST_F(ResizeFilterTest, ApplyResizeWH) {
  ResizeFilter filter(320, 240);
  cv::Mat output;

  filter.apply(test_image_, output);

  EXPECT_FALSE(output.empty());
  EXPECT_EQ(output.cols, 320);
  EXPECT_EQ(output.rows, 240);
}

TEST_F(ResizeFilterTest, ApplyResizeScale) {
  ResizeFilter filter(0.5);
  cv::Mat output;

  filter.apply(test_image_, output);

  EXPECT_FALSE(output.empty());
  EXPECT_EQ(output.cols, 320); // 640 * 0.5
  EXPECT_EQ(output.rows, 240); // 480 * 0.5
}

TEST_F(ResizeFilterTest, SetParameterScale) {
  ResizeFilter filter(640, 480);
  filter.setParameter("scale", 0.25);

  cv::Mat output;
  filter.apply(test_image_, output);

  EXPECT_EQ(output.cols, 160); // 640 * 0.25
  EXPECT_EQ(output.rows, 120); // 480 * 0.25
}

TEST_F(ResizeFilterTest, InvalidScaleIgnored) {
  ResizeFilter filter(640, 480);
  filter.setParameter("scale", -1.0); // Invalid scale

  cv::Mat output;
  filter.apply(test_image_, output);

  // Should remain original size since invalid scale was ignored
  EXPECT_EQ(output.cols, 640);
  EXPECT_EQ(output.rows, 480);
}

TEST_F(ResizeFilterTest, SetParameterScaleZeroIgnored) {
  ResizeFilter filter(640, 480);
  filter.setParameter("scale", 0.0); // Invalid scale

  cv::Mat output;
  filter.apply(test_image_, output);

  // Should remain original size since invalid scale was ignored
  EXPECT_EQ(output.cols, 640);
  EXPECT_EQ(output.rows, 480);
}

TEST_F(ResizeFilterTest, SetParameterWidth) {
  ResizeFilter filter(640, 480);
  filter.setParameter("width", 800);

  cv::Mat output;
  filter.apply(test_image_, output);

  EXPECT_EQ(output.cols, 800);
  EXPECT_EQ(output.rows, 480);
}

TEST_F(ResizeFilterTest, SetParameterHeight) {
  ResizeFilter filter(640, 480);
  filter.setParameter("height", 800);

  cv::Mat output;
  filter.apply(test_image_, output);

  EXPECT_EQ(output.cols, 640);
  EXPECT_EQ(output.rows, 800);
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
  ResizeFilter filter(800, 600);
  filter.setEnabled(false);

  cv::Mat output;
  filter.apply(test_image_, output);

  // When disabled, should return clone of input
  EXPECT_EQ(output.rows, test_image_.rows);
  EXPECT_EQ(output.cols, test_image_.cols);
  EXPECT_EQ(output.channels(), test_image_.channels());
}

TEST_F(ResizeFilterTest, SameSizeResize) {
  ResizeFilter filter(640, 480);
  filter.setEnabled(true);

  cv::Mat output;
  filter.apply(test_image_, output);
  EXPECT_EQ(output.rows, test_image_.rows);
  EXPECT_EQ(output.cols, test_image_.cols);
  EXPECT_EQ(output.channels(), test_image_.channels());
}

TEST_F(ResizeFilterTest, InvalidWidthRejected) {
  ResizeFilter filter(640, 480);

  // Try to set invalid negative width
  filter.setParameter("width", -100);
  auto params = filter.getParameters();
  EXPECT_EQ(params["width"], 640); // Should remain unchanged

  // Try to set zero width
  filter.setParameter("width", 0);
  params = filter.getParameters();
  EXPECT_EQ(params["width"], 640); // Should remain unchanged
}

TEST_F(ResizeFilterTest, InvalidHeightRejected) {
  ResizeFilter filter(640, 480);

  // Try to set invalid negative height
  filter.setParameter("height", -50);
  auto params = filter.getParameters();
  EXPECT_EQ(params["height"], 480); // Should remain unchanged

  // Try to set zero height
  filter.setParameter("height", 0);
  params = filter.getParameters();
  EXPECT_EQ(params["height"], 480); // Should remain unchanged
}

TEST_F(ResizeFilterTest, UnknownParameterIgnored) {
  ResizeFilter filter(640, 480);

  // Try to set unknown parameter (should just log warning, not crash)
  filter.setParameter("unknown_param", 42);

  auto params = filter.getParameters();
  EXPECT_EQ(params["width"], 640);
  EXPECT_EQ(params["height"], 480);
}
