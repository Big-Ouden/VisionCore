/**
 * @brief FramePipeline implementation
 *
 * TODO: ajouter codes d'erreur
 */

#include "FramePipeline.hpp"
#include "../utils/Logger.hpp"
#include "filters/IFilter.hpp"
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

void FramePipeline::addFilter(const std::shared_ptr<filters::IFilter> &filter) {

  std::lock_guard<std::mutex> lock(filters_mutex_);
  filters_.push_back(filter);
  LOG_DEBUG("filter : " + filter->getName() +
            "to pipeline: " + this->getName());
}

void FramePipeline::removeFilter(const size_t index) {
  std::lock_guard<std::mutex> lock(filters_mutex_);
  std::string filter_name = filters_[index]->getName();
  filters_.erase(filters_.begin() + index);
  LOG_DEBUG(filter_name + "removed from pipeline: " + this->getName());
}

// TODO: Lock the mutex in all other methods that access filters_ (removeFilter,
// clear, process, moveFilter, setFilterActive, getFilters, getFilterByIndex,
// size)

void FramePipeline::clear() {
  filters_.clear();
  LOG_DEBUG("Pipeline: " + this->getName() + "cleared");
}

void FramePipeline::process(const cv::Mat &input, cv::Mat &output) const {
  std::lock_guard<std::mutex> lock(filters_mutex_);

  if (filters_.empty()) {
    output = input.clone();
    return;
  }

  cv::Mat current = input.clone();
  cv::Mat tmp;

  for (auto &f : filters_) {

    if (!f->isEnabled()) {
      LOG_DEBUG(f->getName() + "disabled");
      continue;
    }
    auto start = std::chrono::steady_clock::now();

    f->apply(current, tmp);
    current = tmp;
    auto elapsed = std::chrono::steady_clock::now() - start;
    auto ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
    LOG_DEBUG(f->getName() + "took " + std::to_string(ms) + "ms");
  }

  output = current;
}

void FramePipeline::moveFilter(size_t oldIndex, size_t newIndex) {
  std::lock_guard<std::mutex> lock(filters_mutex_);
  if (filters_.empty()) {
    return;
  }

  if (oldIndex >= filters_.size() || newIndex >= filters_.size()) {
    return;
  }

  if (oldIndex == newIndex) {
    LOG_DEBUG("FramePipeline::moveFilter failed, newIndex == oldIndex");
    return;
  }

  if (oldIndex < newIndex) {
    std::rotate(filters_.begin() + oldIndex, filters_.begin() + oldIndex + 1,
                filters_.begin() + newIndex + 1);
  } else {
    std::rotate(filters_.begin() + newIndex, filters_.begin() + oldIndex,
                filters_.begin() + oldIndex + 1);
  }
}

void FramePipeline::setFilterEnabled(size_t index, bool enabled) {
  std::lock_guard<std::mutex> lock(filters_mutex_);
  if (index >= filters_.size()) {
    return;
  }

  filters_[index]->setEnabled(enabled);
  LOG_DEBUG("filter" + filters_[index]->getName() + "set to " +
            std::to_string(enabled));
}

const std::vector<std::shared_ptr<filters::IFilter>> &
FramePipeline::getFilters() const {
  std::lock_guard<std::mutex> lock(filters_mutex_);
  return filters_;
}

std::shared_ptr<filters::IFilter>
FramePipeline::getFilterByIndex(size_t index) const {
  std::lock_guard<std::mutex> lock(filters_mutex_);
  if (index >= filters_.size()) {
    return nullptr;
  }

  if (filters_.empty()) {
    return nullptr;
  }

  return filters_[index];
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
