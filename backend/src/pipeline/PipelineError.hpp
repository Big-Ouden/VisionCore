/**
 * @file PipelineError.hpp
 * @brief Error handling types for pipeline operations
 *
 * This file provides a comprehensive error handling system for the pipeline
 * module, including error codes, result wrappers, and utility functions for
 * error management.
 */

#ifndef PIPELINE_ERROR_HPP
#define PIPELINE_ERROR_HPP

#include <string>
#include <utility>

namespace visioncore::pipeline {
/**
 * @brief Error codes for pipeline operations
 *
 * Enumeration of all possible error conditions that can occur
 * during pipeline operations. Each erro code represent a specific
 * failure scenario.
 */
enum class PipelineError {
  Success,         ///< Operation completed successfully
  IndexOutOfRange, ///< Provided index exceeds valid range
  EmptyPipeline,   ///< Operation requires filters but pipeline is empty
  InvalidFilter,   ///< Filter pointer is null or invalid
  NullPointer,     ///< Unexpected null pointer encountered
  ThreadLockFailed ///< Failed to acquire thread synchronization lock}
};

/**
 * @brief Convert error code to human-readable string
 *
 * @param error The error code to convert
 * @return std::string A descriptive string for the error
 */
inline std::string toString(PipelineError error) {
  switch (error) {
  case PipelineError::Success:
    return "Success";
  case PipelineError::IndexOutOfRange:
    return "Index out of range";
  case PipelineError::EmptyPipeline:
    return "Pipeline is empty";
  case PipelineError::InvalidFilter:
    return "Invalid filter";
  case PipelineError::NullPointer:
    return "Null pointer";
  case PipelineError::ThreadLockFailed:
    return "Thread lock failed";
  default:
    return "Unknown error";
  }
}

/**
 * @brief Result wrapper for pipeline operations
 *
 * A discriminated union type that represents either a succesful result
 * with a value f type T, or an error with an error code and message.
 * This allows functions to return errors withour throwing exceptions.
 *
 * @tparam T the type of value returned on success
 */
template <typename T> struct PipelineResult {
  PipelineError error; ///< Error code (Success if operattions succeeded)
  T value;
  std::string message;

  /**
   * @brief Check if operation succeeded
   * @return true if operation completed successfully, false otherwise
   */
  bool isOk() const { return error == PipelineError::Success; }

  /**
   * @brief Check if operation failed
   * @return true if operation failed, false otherwise
   */
  bool isErr() const { return !isOk(); }

  /**
   * @brief Get the error type as string
   * @return Human-readable name of the error type
   */
  std::string errorType() const { return toString(error); }

  /**
   * @brief Create a successful result
   *
   * @param val The value to return on success
   * @return PipelineResult containing the successful value
   */
  static PipelineResult Ok(T val) {
    return {PipelineError::Success, std::move(val), ""};
  }

  /**
   * @brief Create an error result
   *
   * @param err The error code
   * @param msg Optional custom error message (default to error type name)
   * @return PipelineResult containing the error code and message
   */
  static PipelineResult Err(PipelineError err, const std::string &msg = "") {
    std::string fullMsg = msg.empty() ? toString(err) : msg;
    return {err, T{}, fullMsg};
  }
};

/**
 * @brief Specialization of PipelineResult for void operations
 *
 * For operations that don't return a value (void funtions),
 * This specialization provides a way to represent success or failure
 * without a value field.
 */
template <> struct PipelineResult<void> {
  PipelineError error; ///< Error code (Success if operation succeeded)
  std::string message; ///< Human-readable error or success message

  /**
   * @brief Check if operation succeeded
   * @return true if operation completed successfully, false otherwise
   */
  bool isOk() const { return error == PipelineError::Success; }

  /**
   * @brief Check if operation failed
   * @return true if operation failed, false otherwise
   */
  bool isErr() const { return !isOk(); }

  /**
   * @brief Get the error type as string
   * @return Human-readable name of the error type
   */
  std::string errorType() const { return toString(error); }

  /**
   * @brief Create a successful result
   * @return PipelineResult representing success
   */
  static PipelineResult Ok() { return {PipelineError::Success, ""}; }

  /**
   * @brief Create an error result
   *
   * @param err The error code
   * @param msg Optional custom error message (default to error type name)
   * @preturn PipelineResult containing the error code and message
   */
  static PipelineResult Err(PipelineError err, const std::string &msg = "") {
    std::string fullMsg = msg.empty() ? toString(err) : msg;
    return {err, fullMsg};
  }
};

/**
 * @brief Helper to map pipeline errors to HTTP status codes
 *
 * Useful for webhook/API integration where pipeline errors need
 * to be translated into appropriate HTTP responses.
 *
 * @param error The pipeline error code
 * @return int The corresponding HTTP status code
 *
 * @example
 * ```cpp
 * auto result = pipeline.getFilterByIndex(999);
 * if (result.isErr()) {
 *   int httpCode = toHttpCode(result.error);  // Returns 400
 *   return createErrorResponse(httpCode, result.message);
 * }
 * ```
 */
inline int toHttpCode(PipelineError error) {
  switch (error) {
  case PipelineError::Success:
    return 200; // OK
  case PipelineError::IndexOutOfRange:
  case PipelineError::InvalidFilter:
  case PipelineError::NullPointer:
    return 400; // Bad Request
  case PipelineError::EmptyPipeline:
    return 404; // Not Found
  case PipelineError::ThreadLockFailed:
    return 500; // Internal Server Error
  default:
    return 500; // Internal Server Error
  }
}

} // namespace visioncore::pipeline

#endif // PIPELINE_ERROR_HPP
