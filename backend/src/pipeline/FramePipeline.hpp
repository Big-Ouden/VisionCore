/**
 * @brief FramePipeline manages a sequence of filters to process frames.
 *
 * This class allows dynamic composition of filters, enabling/disabling them,
 * and sequentially applying them to input frames (cv::Mat).
 */

#ifndef FRAMEPIPELINE_HPP
#define FRAMEPIPELINE_HPP

#include "../filters/IFilter.hpp"
#include "PipelineError.hpp"
#include <memory>
#include <nlohmann/json.hpp>
#include <opencv2/opencv.hpp>
#include <string>
#include <vector>

namespace visioncore::pipeline {

class FramePipeline {
public:
  /**
   * @brief Construct a new FramePipeline
   * @param name Optional pipeline name
   */
  explicit FramePipeline(const std::string &name = "");

  /**
   * @brief Add a filter to the pipeline
   * @param filter Shared pointer to a filter
   */
  PipelineResult<void>
  addFilter(const std::shared_ptr<filters::IFilter> &filter);

  /**
   * @brief Remove a filter by index
   * @param index Index of the filter to remove
   */
  PipelineResult<void> removeFilter(const size_t index);

  /**
   * @brief Remove all filters from the pipeline
   */
  PipelineResult<void> clear();

  /**
   * @brief Process an input frame through all active filters
   * @param Input frame (cv::Mat)
   * @param Output frame (cv::Mat)
   */
  PipelineResult<void> process(const cv::Mat &input, cv::Mat &output) const;

  /**
   * @brief Move a filter from one position to another
   * @param oldIndex Current index
   * @param newIndex Target index
   */
  PipelineResult<void> moveFilter(size_t oldIndex, size_t newIndex);

  /**
   * @brief Enable or disable a filter
   * @param index Index of the filter
   * @param active True to enable, false to disable
   */
  PipelineResult<void> setFilterEnabled(size_t index, bool enabled);

  /**
   * @brief Get the list of filters (const ref)
   */
  PipelineResult<std::vector<std::shared_ptr<filters::IFilter>>>
  getFilters() const;

  /**
   * @brief Get a filter by index (const ref)
   * @param index Index of the filter
   * @return Shared pointer to the filter
   */
  PipelineResult<std::shared_ptr<filters::IFilter>>
  getFilterByIndex(size_t index) const;

  /**
   * @brief Get the number of filters in the pipeline
   */
  size_t size() const;

  /**
   * @brief Get the pipeline name
   */
  const std::string &getName() const;

  /**
   * @brief Return true if pipeline is active, false else
   */
  bool isActive() const;

private:
  mutable std::mutex filters_mutex_; ///< Mutex for thread safety
  std::vector<std::shared_ptr<filters::IFilter>> filters_; ///< List of filters
  bool active_;      ///< Activation state of the pipeline
  std::string name_; ///< Pipeline name
};

} // namespace visioncore::pipeline

#endif
