# backend/tests/CMakeLists.txt

add_executable(test_filters test_filters.cpp)
target_link_libraries(test_filters PRIVATE visioncore)
add_test(NAME filters COMMAND test_filters)

add_executable(test_pipeline test_pipeline.cpp)
target_link_libraries(test_pipeline PRIVATE visioncore)
add_test(NAME pipeline COMMAND test_pipeline)

add_executable(test_sources test_sources.cpp)
target_link_libraries(test_sources PRIVATE visioncore)
add_test(NAME sources COMMAND test_sources)

