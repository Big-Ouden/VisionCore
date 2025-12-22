# backend/tests/CMakeLists.txt

# Find Google Test
find_package(GTest REQUIRED)
include(GoogleTest)

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

