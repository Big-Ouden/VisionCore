# backend/tests/CMakeLists.txt

# Find Google Test
find_package(GTest REQUIRED)
include(GoogleTest)

# Test Logger
add_executable(test_logger test_logger.cpp)
target_link_libraries(test_logger PRIVATE 
  visioncore 
  GTest::GTest 
  GTest::Main 
)
gtest_discover_tests(test_logger)

# Test Filters
add_executable(test_filters test_filters.cpp)
target_link_libraries(test_filters PRIVATE 
  visioncore 
  GTest::GTest 
  GTest::Main  # Provides main() automatically
)
gtest_discover_tests(test_filters)

# Test Pipeline
add_executable(test_pipeline test_pipeline.cpp)
target_link_libraries(test_pipeline PRIVATE 
  visioncore 
  GTest::GTest 
  GTest::Main
)
gtest_discover_tests(test_pipeline)

# Test Sources
add_executable(test_sources test_sources.cpp)
target_link_libraries(test_sources PRIVATE 
  visioncore 
  GTest::GTest 
  GTest::Main
)
gtest_discover_tests(test_sources)


# Test FrameController
add_executable(test_framecontroller test_framecontroller.cpp)
target_link_libraries(test_framecontroller PRIVATE 
  visioncore 
  GTest::GTest 
  GTest::Main
)
gtest_discover_tests(test_framecontroller)

# Test FrameController Stop
add_executable(test_framecontroller_stop test_framecontroller_stop.cpp)
target_link_libraries(test_framecontroller_stop PRIVATE 
  visioncore 
  GTest::GTest 
  GTest::Main
)
gtest_discover_tests(test_framecontroller_stop)


