#include "../src/filters/GrayscaleFilter.hpp"
#include "../src/filters/LUTFilter.hpp"
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
  EXPECT_EQ(filter.getParameters()["width"], 320);
  EXPECT_EQ(filter.getParameters()["height"], 240);
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

// ====================  LUTFilter Tests ====================

class LUTFilterTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Create test image
    test_image_ = cv::Mat(100, 100, CV_8UC3, cv::Scalar(255, 0, 0));
    test_image_gray_ = cv::Mat(100, 100, CV_8UC1);
  }

  cv::Mat test_image_;
  cv::Mat test_image_gray_;
};

TEST_F(LUTFilterTest, Constructor) {

  LUTFilter filter;
  EXPECT_EQ(filter.getName(), "lut");
  EXPECT_TRUE(filter.isEnabled());
}

TEST_F(LUTFilterTest, ConstructorWithParameters) {
  LUTFilter filter(LUTFilter::LUTType::CUSTOM, 1.0);
  EXPECT_EQ(filter.getName(), "lut");
  EXPECT_TRUE(filter.isEnabled());
  EXPECT_EQ(filter.getParameters()["lut_type"], "custom");
  EXPECT_EQ(filter.getParameters()["param"], 1.0);
}

TEST_F(LUTFilterTest, ApplyFilter) {
  LUTFilter filter(LUTFilter::LUTType::INVERT, 1.0);
  cv::Mat output;

  filter.apply(test_image_, output);

  EXPECT_FALSE(output.empty());
  EXPECT_EQ(output.channels(), test_image_.channels());
  EXPECT_EQ(output.rows, test_image_.rows);
  EXPECT_EQ(output.cols, test_image_.cols);

  cv::Mat diff;
  cv::absdiff(output, test_image_, diff);
  std::vector<cv::Mat> channels;
  cv::split(diff, channels);
  cv::Mat diff_gray = channels[0] | channels[1] | channels[2];
  EXPECT_GT(cv::countNonZero(diff_gray),
            0); // Il y a au moins un pixel différent

  EXPECT_EQ(output.at<cv::Vec3b>(0, 0),
            cv::Vec3b(0, 255, 255)); // Inverted blue
}

TEST_F(LUTFilterTest, ApplyToEmptyImage) {
  cv::Mat empty_image;

  LUTFilter filter(LUTFilter::LUTType::INVERT, 1.0);
  filter.setEnabled(true);
  cv::Mat output;
  filter.apply(empty_image, output);

  EXPECT_TRUE(empty_image.empty());
  EXPECT_TRUE(output.empty());
}

TEST_F(LUTFilterTest, DisabledFilter) {
  LUTFilter filter(LUTFilter::LUTType::INVERT, 1.0);
  filter.setEnabled(false);

  cv::Mat output;
  filter.apply(test_image_, output);

  // When disabled, should return clone of input
  EXPECT_EQ(output.rows, test_image_.rows);
  EXPECT_EQ(output.cols, test_image_.cols);
  EXPECT_EQ(output.channels(), test_image_.channels());

  cv::Mat diff;
  cv::absdiff(output, test_image_, diff);
  std::vector<cv::Mat> channels;
  cv::split(diff, channels);
  cv::Mat diff_gray = channels[0] | channels[1] | channels[2];
  EXPECT_EQ(cv::countNonZero(diff_gray),
            0); // Toutes les valeurs sont identiques
}

TEST_F(LUTFilterTest, SetParameter) {
  LUTFilter filter(LUTFilter::LUTType::GAMMA, 2.2);
  nlohmann::json value = 1.5;
  filter.setParameter("param", value);
  auto params = filter.getParameters();
  ASSERT_TRUE(params.contains("param"));
  EXPECT_DOUBLE_EQ(params["param"], 1.5);

  filter.setParameter("lut_type", "invert");
  params = filter.getParameters();
  ASSERT_TRUE(params.contains("lut_type"));
  EXPECT_EQ(params["lut_type"], "invert");
}

TEST_F(LUTFilterTest, GetParameters) {
  LUTFilter filter(LUTFilter::LUTType::BRIGHTNESS, 0.8);
  auto params = filter.getParameters();
  ASSERT_TRUE(params.contains("lut_type"));
  ASSERT_TRUE(params.contains("param"));
  ASSERT_TRUE(params.contains("enabled"));
  EXPECT_EQ(params["lut_type"], "brightness");
  EXPECT_DOUBLE_EQ(params["param"], 0.8);
  EXPECT_TRUE(params["enabled"].is_boolean());
}

TEST_F(LUTFilterTest, GetName) {
  LUTFilter filter(LUTFilter::LUTType::INVERT, 1.0);
  EXPECT_EQ(filter.getName(), "lut");
}

TEST_F(LUTFilterTest, UpdateLUT_Indirect) {
  LUTFilter filter(LUTFilter::LUTType::CONTRAST, 1.0);
  nlohmann::json value = 2.0;
  filter.setParameter("param", value); // Should trigger updateLUT internally
  cv::Mat output;
  filter.apply(test_image_, output);
  EXPECT_EQ(output.rows, test_image_.rows);
  EXPECT_EQ(output.cols, test_image_.cols);
}

TEST_F(LUTFilterTest, SetCustomLUT) {
  LUTFilter filter;
  nlohmann::json custom_lut = nlohmann::json::array();
  for (int i = 0; i < 256; ++i)
    custom_lut.push_back(255 - i);
  filter.setParameter("custom_lut", custom_lut);
  auto params = filter.getParameters();
  EXPECT_EQ(params["lut_type"], "custom");
}

TEST_F(LUTFilterTest, SetLUTTypeIdentity) {
  LUTFilter filter(LUTFilter::LUTType::INVERT, 1.0);
  filter.setParameter("lut_type", "identity");

  cv::Mat output;
  filter.apply(test_image_, output);

  cv::Mat diff;
  cv::absdiff(output, test_image_, diff);
  EXPECT_EQ(cv::countNonZero(diff.reshape(1)), 0);
}

TEST_F(LUTFilterTest, LogarithmicLUT) {
  cv::Mat input(1, 256, CV_8UC1);
  for (int i = 0; i < 256; ++i)
    input.at<uint8_t>(i) = static_cast<uint8_t>(i);

  LUTFilter filter;
  filter.setParameter("lut_type", "logarithmic");

  cv::Mat output;
  filter.apply(input, output);

  EXPECT_EQ(output.at<uint8_t>(0), 0);
  EXPECT_LT(output.at<uint8_t>(1), output.at<uint8_t>(10));
  EXPECT_LT(output.at<uint8_t>(10), output.at<uint8_t>(100));
  EXPECT_LT(output.at<uint8_t>(100), output.at<uint8_t>(200));
}

TEST_F(LUTFilterTest, ExponentialLUT) {
  cv::Mat input(1, 256, CV_8UC1);
  for (int i = 0; i < 256; ++i)
    input.at<uint8_t>(i) = static_cast<uint8_t>(i);

  LUTFilter filter;
  filter.setParameter("lut_type", "exponential");

  cv::Mat output;
  filter.apply(input, output);

  EXPECT_EQ(output.at<uint8_t>(0), 0);
  EXPECT_LT(output.at<uint8_t>(10), output.at<uint8_t>(50));
  EXPECT_LT(output.at<uint8_t>(50), output.at<uint8_t>(200));
}

TEST_F(LUTFilterTest, ThresholdBinaryLUT) {
  LUTFilter filter;
  filter.setParameter("lut_type", "threshold");
  filter.setParameter("param", 128.0);

  cv::Mat input(1, 3, CV_8UC1);
  input.at<uint8_t>(0) = 50;
  input.at<uint8_t>(1) = 128;
  input.at<uint8_t>(2) = 200;

  cv::Mat output;
  filter.apply(input, output);

  EXPECT_EQ(output.at<uint8_t>(0), 50);
  EXPECT_EQ(output.at<uint8_t>(1), 255);
  EXPECT_EQ(output.at<uint8_t>(2), 255);
}

TEST_F(LUTFilterTest, GammaDarkeningAndBrightening) {
  cv::Mat input(1, 1, CV_8UC1);
  input.at<uint8_t>(0, 0) = 128;

  LUTFilter dark(LUTFilter::LUTType::GAMMA, 2.0);
  LUTFilter bright(LUTFilter::LUTType::GAMMA, 0.5);

  cv::Mat out_dark, out_bright;
  dark.apply(input, out_dark);
  bright.apply(input, out_bright);

  EXPECT_LT(out_dark.at<uint8_t>(0, 0), 128);
  EXPECT_GT(out_bright.at<uint8_t>(0, 0), 128);
  EXPECT_LT(out_dark.at<uint8_t>(0, 0), out_bright.at<uint8_t>(0, 0));
}

TEST_F(LUTFilterTest, InvalidCustomLUTSize) {
  LUTFilter filter;
  nlohmann::json bad_lut = nlohmann::json::array({0, 1, 2});

  filter.setParameter("custom_lut", bad_lut);

  auto params = filter.getParameters();
  EXPECT_NE(params["lut_type"], "custom");
}

TEST_F(LUTFilterTest, UnknownParameter) {
  LUTFilter filter;
  filter.setParameter("unknown_param", 42);

  auto params = filter.getParameters();
  EXPECT_TRUE(params.contains("lut_type"));
}

TEST_F(LUTFilterTest, AllLUTTypeStrings) {
  struct {
    std::string input;
    std::string expected;
  } cases[] = {
      {"identity", "identity"},
      {"invert", "invert"},
      {"contrast", "contrast"},
      {"brightness", "brightness"},
      {"gamma", "gamma"},
      {"logarithmic", "logarithmic"},
      {"exponential", "exponential"},
      {"threshold", "threshold_binary"},
  };

  for (const auto &c : cases) {
    LUTFilter filter;
    filter.setParameter("lut_type", c.input);
    EXPECT_EQ(filter.getParameters()["lut_type"], c.expected);
  }
}
