/**
 * @brief FramePipeline implementation
 */

#include "FramePipeline.hpp"
#include "../utils/Logger.hpp"
#include "filters/IFilter.hpp"
#include "pipeline/PipelineError.hpp"
#include <chrono>
#include <memory>
#include <mutex>
#include <pstl/glue_algorithm_defs.h>
#include <ranges>
#include <stdexcept>
#include <string>
#include <vector>

namespace visioncore::pipeline {

FramePipeline::FramePipeline(const std::string &name)
    : filters_(), active_(true), name_(name) {}

PipelineResult<void>
FramePipeline::addFilter(const std::shared_ptr<filters::IFilter> &filter) {

  std::lock_guard<std::mutex> lock(filters_mutex_);

  filters_.push_back(filter);
  LOG_DEBUG("filter : " + filter->getName() + "to pipeline: " + name_);

  return PipelineResult<void>::Ok();
}

PipelineResult<void> FramePipeline::removeFilter(const size_t index) {
  std::lock_guard<std::mutex> lock(filters_mutex_);

  if (index >= filters_.size()) {
    return PipelineResult<void>::Err(PipelineError::IndexOutOfRange,
                                     "Index" + std::to_string(index) +
                                         "is out of range");
  }

  std::string filter_name = filters_[index]->getName();
  filters_.erase(filters_.begin() + index);
  LOG_DEBUG(filter_name + "removed from pipeline: " + name_);

  return PipelineResult<void>::Ok();
}

PipelineResult<void> FramePipeline::clear() {
  std::lock_guard<std::mutex> lock(filters_mutex_);

  if (filters_.empty()) {
    LOG_DEBUG("Pipeline: " + name_ + "is already empty");
    return PipelineResult<void>::Err(PipelineError::EmptyPipeline,
                                     "Pipeline already empty");
  }

  filters_.clear();
  LOG_DEBUG("Pipeline: " + name_ + "cleared");

  return PipelineResult<void>::Ok();
}

PipelineResult<void> FramePipeline::process(const cv::Mat &input,
                                            cv::Mat &output) const {

  if (filters_.empty())
    return PipelineResult<void>::Err(PipelineError::EmptyPipeline);

  if (input.empty()) {
    output.release();
    return PipelineResult<void>::Err(PipelineError::NullPointer,
                                     "Input image is empty");
  }

  std::vector<std::shared_ptr<filters::IFilter>> local_filters;
  {
    std::lock_guard<std::mutex> lock(filters_mutex_);
    local_filters = filters_;
  }

  if (local_filters.empty()) {
    // pipeline vide → renvoyer l'image inchangée
    output = input.clone();
    return PipelineResult<void>::Ok();
  }

  cv::Mat current = input.clone();

  for (auto &f : local_filters) {

    if (!f->isEnabled()) {
      LOG_DEBUG(f->getName() + " disabled");
      continue;
    }

    cv::Mat tmp;
    auto start = std::chrono::steady_clock::now();

    // f->apply modifie tmp en sortie
    try {
      f->apply(current, tmp);
    } catch (const std::exception &e) {
      return PipelineResult<void>::Err(PipelineError::InvalidFilter,
                                       "Filter " + f->getName() +
                                           " threw: " + e.what());
    } catch (...) {
      return PipelineResult<void>::Err(PipelineError::InvalidFilter,
                                       "Filter " + f->getName() + " crashed");
    }

    if (tmp.empty()) {
      return PipelineResult<void>::Err(PipelineError::InvalidFilter,
                                       "Filter " + f->getName() +
                                           " produced empty output");
    }

    current = tmp;

    auto elapsed = std::chrono::steady_clock::now() - start;
    auto ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
    LOG_DEBUG(f->getName() + " took " + std::to_string(ms) + "ms");
  }

  output = current;
  return PipelineResult<void>::Ok();
}

PipelineResult<void> FramePipeline::moveFilter(size_t oldIndex,
                                               size_t newIndex) {
  std::lock_guard<std::mutex> lock(filters_mutex_);
  if (filters_.empty()) {
    LOG_DEBUG("FramePipeline::moveFilter failed, empty pipeline");
    return PipelineResult<void>::Err(PipelineError::EmptyPipeline,
                                     "Cannot move filter in empty pipeline");
  }

  if (oldIndex >= filters_.size() || newIndex >= filters_.size()) {
    LOG_DEBUG("FramePipeline::moveFilter failed, index out of range");
    return PipelineResult<void>::Err(PipelineError::IndexOutOfRange,
                                     "OldIndex or NewIndex is out of range");
  }

  if (oldIndex == newIndex) {
    LOG_DEBUG("FramePipeline::moveFilter no operation needed");
    return PipelineResult<void>::Ok();
  }

  if (oldIndex < newIndex) {
    std::rotate(filters_.begin() + oldIndex, filters_.begin() + oldIndex + 1,
                filters_.begin() + newIndex + 1);
  } else {
    std::rotate(filters_.begin() + newIndex, filters_.begin() + oldIndex,
                filters_.begin() + oldIndex + 1);
  }

  return PipelineResult<void>::Ok();
}

PipelineResult<void> FramePipeline::setFilterEnabled(size_t index,
                                                     bool enabled) {
  std::lock_guard<std::mutex> lock(filters_mutex_);
  if (index >= filters_.size()) {
    LOG_DEBUG("FramePipeline::setFilterEnabled failed, index out of range");
    return PipelineResult<void>::Err(PipelineError::IndexOutOfRange,
                                     "Index" + std::to_string(index) +
                                         "is out of range");
  }

  filters_[index]->setEnabled(enabled);

  LOG_DEBUG("Filter " + filters_[index]->getName() + " set to " +
            std::string(enabled ? "enabled" : "disabled"));

  return PipelineResult<void>::Ok();
}

PipelineResult<std::vector<std::shared_ptr<filters::IFilter>>>
FramePipeline::getFilters() const {
  std::lock_guard<std::mutex> lock(filters_mutex_);

  if (filters_.empty()) {
    return PipelineResult<std::vector<std::shared_ptr<filters::IFilter>>>::Err(
        PipelineError::EmptyPipeline, "Pipeline is empty");
  }

  return PipelineResult<std::vector<std::shared_ptr<filters::IFilter>>>::Ok(
      filters_);
}

PipelineResult<std::shared_ptr<filters::IFilter>>
FramePipeline::getFilterByIndex(size_t index) const {
  std::lock_guard<std::mutex> lock(filters_mutex_);

  if (filters_.empty()) {
    LOG_DEBUG("FramePipeline::getFilterByIndex failed, empty pipeline");
    return PipelineResult<std::shared_ptr<filters::IFilter>>::Err(
        PipelineError::EmptyPipeline, "Cannot get filter from empty pipeline");
  }

  if (index >= filters_.size()) {
    LOG_DEBUG("FramePipeline::getFilterByIndex failed, index out of range");
    return PipelineResult<std::shared_ptr<filters::IFilter>>::Err(
        PipelineError::IndexOutOfRange,
        "Index" + std::to_string(index) + "is out of range");
  }

  return PipelineResult<std::shared_ptr<filters::IFilter>>::Ok(filters_[index]);
}

size_t FramePipeline::size() const {
  std::lock_guard<std::mutex> lock(filters_mutex_);
  return filters_.size();
}

const std::string &FramePipeline::getName() const {
  std::lock_guard<std::mutex> lock(filters_mutex_);
  return name_;
}

bool FramePipeline::isActive() const {
  std::lock_guard<std::mutex> lock(filters_mutex_);
  return active_;
}

} // namespace visioncore::pipeline
