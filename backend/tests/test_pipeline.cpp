
// tests/test_framepipeline_full.cpp
#include "filters/GrayscaleFilter.hpp"
#include "pipeline/FramePipeline.hpp"
#include "pipeline/PipelineError.hpp"
#include "gtest/gtest.h"
#include <memory>
#include <opencv2/opencv.hpp>
#include <string>
#include <vector>

using namespace visioncore::pipeline;
using namespace visioncore::filters;

// -------------------- FramePipeline Comprehensive Tests --------------------

TEST(FramePipelineFullTest, AddGetRemoveClearFilters) {
  FramePipeline pipeline("pipeline1");

  auto filter1 = std::make_shared<GrayscaleFilter>();
  auto filter2 = std::make_shared<GrayscaleFilter>();

  // Add filters
  EXPECT_TRUE(pipeline.addFilter(filter1).isOk());
  EXPECT_TRUE(pipeline.addFilter(filter2).isOk());
  EXPECT_EQ(pipeline.size(), 2u);

  // Get filters by index
  auto res0 = pipeline.getFilterByIndex(0);
  ASSERT_TRUE(res0.isOk());
  EXPECT_EQ(res0.value->getName(), filter1->getName());

  auto res1 = pipeline.getFilterByIndex(1);
  ASSERT_TRUE(res1.isOk());
  EXPECT_EQ(res1.value->getName(), filter2->getName());

  // Remove invalid index
  auto remErr = pipeline.removeFilter(10);
  EXPECT_TRUE(remErr.isErr());
  EXPECT_EQ(remErr.error, PipelineError::IndexOutOfRange);

  // Remove valid filter
  EXPECT_TRUE(pipeline.removeFilter(0).isOk());
  EXPECT_EQ(pipeline.size(), 1u);

  // Clear filters
  EXPECT_TRUE(pipeline.clear().isOk());
  EXPECT_EQ(pipeline.size(), 0u);

  // Clear empty pipeline
  auto clearErr = pipeline.clear();
  EXPECT_TRUE(clearErr.isErr());
  EXPECT_EQ(clearErr.error, PipelineError::EmptyPipeline);
}

TEST(FramePipelineFullTest, MoveFiltersAndEnableDisable) {
  FramePipeline pipeline("pipeline2");

  auto f1 = std::make_shared<GrayscaleFilter>();
  auto f2 = std::make_shared<GrayscaleFilter>();
  pipeline.addFilter(f1);
  pipeline.addFilter(f2);

  // Move filters valid
  EXPECT_TRUE(pipeline.moveFilter(0, 1).isOk());
  auto allFilters = pipeline.getFilters();
  ASSERT_TRUE(allFilters.isOk());
  EXPECT_EQ(allFilters.value[0]->getName(), f2->getName());

  // Move same index
  EXPECT_TRUE(pipeline.moveFilter(1, 1).isOk());

  // Move invalid index
  auto moveErr = pipeline.moveFilter(0, 10);
  EXPECT_TRUE(moveErr.isErr());
  EXPECT_EQ(moveErr.error, PipelineError::IndexOutOfRange);

  // Disable filter valid
  EXPECT_TRUE(pipeline.setFilterEnabled(0, false).isOk());

  // Disable filter invalid index
  auto disErr = pipeline.setFilterEnabled(10, true);
  EXPECT_TRUE(disErr.isErr());
  EXPECT_EQ(disErr.error, PipelineError::IndexOutOfRange);
}

TEST(FramePipelineFullTest, ProcessSingleAndMultipleFilters) {
  FramePipeline pipeline("pipeline3");

  // Single filter
  auto f1 = std::make_shared<GrayscaleFilter>();
  pipeline.addFilter(f1);

  cv::Mat input = cv::Mat::zeros(5, 5, CV_8UC3); // 3-channel image
  cv::Mat output;

  auto res = pipeline.process(input, output);
  EXPECT_TRUE(res.isOk());
  EXPECT_EQ(output.channels(), 1); // grayscale applied

  // Multiple filters sequentially
  auto f2 = std::make_shared<GrayscaleFilter>();
  pipeline.addFilter(f2);

  cv::Mat out2;
  EXPECT_TRUE(pipeline.process(input, out2).isOk());
  EXPECT_EQ(out2.channels(), 1);

  // Disable first filter, second active
  EXPECT_TRUE(pipeline.setFilterEnabled(0, false).isOk());
  cv::Mat out3;
  EXPECT_TRUE(pipeline.process(input, out3).isOk());
}

TEST(FramePipelineFullTest, ProcessErrors) {
  FramePipeline pipeline("pipeline4");

  auto f1 = std::make_shared<GrayscaleFilter>();
  pipeline.addFilter(f1);

  cv::Mat empty;
  cv::Mat out;

  // Empty input image
  auto res = pipeline.process(empty, out);
  EXPECT_TRUE(res.isErr());
  EXPECT_EQ(res.error, PipelineError::NullPointer);

  // Remove all filters and try processing
  EXPECT_TRUE(pipeline.clear().isOk());
  auto res2 = pipeline.process(cv::Mat::zeros(1, 1, CV_8UC3), out);
  EXPECT_TRUE(res2.isErr());
  EXPECT_EQ(res2.error, PipelineError::EmptyPipeline);
}

TEST(FramePipelineFullTest, AccessorsAndPipelineName) {
  FramePipeline pipeline("pipeline5");
  EXPECT_EQ(pipeline.getName(), "pipeline5");
  EXPECT_TRUE(pipeline.isActive());
  EXPECT_EQ(pipeline.size(), 0u);
}

// -------------------- PipelineResult Tests --------------------

TEST(PipelineResultFullTest, VoidOkAndErr) {
  auto ok = PipelineResult<void>::Ok();
  EXPECT_TRUE(ok.isOk());
  EXPECT_FALSE(ok.isErr());
  EXPECT_EQ(ok.errorType(), "Success");

  auto err = PipelineResult<void>::Err(PipelineError::EmptyPipeline, "custom");
  EXPECT_TRUE(err.isErr());
  EXPECT_EQ(err.error, PipelineError::EmptyPipeline);
  EXPECT_EQ(err.message, "custom");
}

TEST(PipelineResultFullTest, TemplateOkAndErr) {
  auto f1 = std::make_shared<GrayscaleFilter>();

  auto ok = PipelineResult<std::shared_ptr<IFilter>>::Ok(f1);
  EXPECT_TRUE(ok.isOk());
  EXPECT_EQ(ok.value->getName(), f1->getName());

  auto err = PipelineResult<std::shared_ptr<IFilter>>::Err(
      PipelineError::InvalidFilter, "fail");
  EXPECT_TRUE(err.isErr());
  EXPECT_EQ(err.error, PipelineError::InvalidFilter);
  EXPECT_EQ(err.message, "fail");
}

TEST(PipelineResultFullTest, VectorOkAndErr) {
  std::vector<std::shared_ptr<IFilter>> vec;
  auto ok = PipelineResult<std::vector<std::shared_ptr<IFilter>>>::Ok(vec);
  EXPECT_TRUE(ok.isOk());
  EXPECT_TRUE(ok.value.empty());

  auto err = PipelineResult<std::vector<std::shared_ptr<IFilter>>>::Err(
      PipelineError::EmptyPipeline);
  EXPECT_TRUE(err.isErr());
  EXPECT_EQ(err.error, PipelineError::EmptyPipeline);
}

// -------------------- HTTP Code Mapping --------------------

TEST(PipelineResultFullTest, ToHttpCodeMapping) {
  EXPECT_EQ(toHttpCode(PipelineError::Success), 200);
  EXPECT_EQ(toHttpCode(PipelineError::IndexOutOfRange), 400);
  EXPECT_EQ(toHttpCode(PipelineError::InvalidFilter), 400);
  EXPECT_EQ(toHttpCode(PipelineError::NullPointer), 400);
  EXPECT_EQ(toHttpCode(PipelineError::EmptyPipeline), 404);
  EXPECT_EQ(toHttpCode(PipelineError::ThreadLockFailed), 500);
}
