cmake_minimum_required(VERSION 3.22)

project(
  VisionCore 
  VERSION 0.1.0 
  LANGUAGES CXX
)

option(CODE_COVERAGE "Enable coverage reporting" ON)
if(CODE_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  message(STATUS "Building with coverage flags")
  add_compile_options(--coverage -O0 -g)
  add_link_options(--coverage)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ===============
# Config Globale 
# ===============

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

option(VISIONCORE_BUILD_TESTS "Build VisionCore tests" ON)

# =================
# Depedencies 
# =================

find_package(OpenCV 4 REQUIRED COMPONENTS
    core
    imgproc
    imgcodecs
    videoio
    highgui
)
find_package(Threads REQUIRED)
find_package(nlohmann_json 3.10 REQUIRED)

# Qt5 here until network stream and display are moved to separate modules
find_package(Qt5 REQUIRED COMPONENTS Widgets)


# uWebSockets (header-only)
find_path(UWEBSOCKETS_INCLUDE_DIR 
    NAMES uWebSockets/App.h
    PATHS /usr/include /usr/local/include
)

if(NOT UWEBSOCKETS_INCLUDE_DIR)
    message(FATAL_ERROR "uWebSockets headers not found")
endif()

message(STATUS "uWebSockets headers: ${UWEBSOCKETS_INCLUDE_DIR}")

# Try to find uSockets library (may be part of uwebsockets package)
find_library(USOCKETS_LIB 
    NAMES uSockets usockets
    PATHS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu
)

if(USOCKETS_LIB)
    message(STATUS "uSockets library found: ${USOCKETS_LIB}")
    set(WEBSOCKET_LIBS ${USOCKETS_LIB})
else()
    message(STATUS "uSockets library not found, will try static linking")
    # Some distros include uSockets as static lib in uwebsockets
    find_library(USOCKETS_STATIC
        NAMES libuSockets.a libusockets.a
        PATHS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu
    )
    
    if(USOCKETS_STATIC)
        message(STATUS "uSockets static library found: ${USOCKETS_STATIC}")
        set(WEBSOCKET_LIBS ${USOCKETS_STATIC})
    else()
        message(WARNING "uSockets not found, WebSocket features may not link properly")
        set(WEBSOCKET_LIBS "")
    endif()
endif()

# ========================
# Sources auto-detection 
# ========================

file(GLOB_RECURSE VISIONCORE_SOURCES
    CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

# Explicitly remove main.cpp from files 
list(REMOVE_ITEM VISIONCORE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
)

# ====================
# VisionCore Library 
# ====================

add_library(visioncore ${VISIONCORE_SOURCES})

target_include_directories(visioncore
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${UWEBSOCKETS_INCLUDE_DIR}
)

target_link_libraries(visioncore
    PUBLIC
        opencv_core
        opencv_imgproc
        opencv_imgcodecs
        opencv_videoio
        opencv_highgui
        Threads::Threads
        nlohmann_json::nlohmann_json
        ${USOCKETS_LIB}
        z  # zlib for compression
)

target_compile_features(visioncore PUBLIC cxx_std_20)

target_compile_options(visioncore
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
)

# ========================
# Executable 
# ========================
add_executable(visioncore_app
    src/main.cpp
)

target_link_libraries(visioncore_app
    PRIVATE
        visioncore
        Qt5::Widgets
)

target_compile_features(visioncore_app PRIVATE cxx_std_20)

# ========================
# Tests (optional)
# ========================
if(VISIONCORE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# ========================
# Code Coverage Target
# ========================
find_program(LCOV_PATH lcov)
find_program(GENHTML_PATH genhtml)

file(COPY ${CMAKE_SOURCE_DIR}/assets DESTINATION ${CMAKE_BINARY_DIR})

if(LCOV_PATH AND GENHTML_PATH)
    message(STATUS "lcov found: ${LCOV_PATH}")
    message(STATUS "genhtml found: ${GENHTML_PATH}")
    
    add_custom_target(coverage
        COMMENT "Generating code coverage report"
        
        # Clean previous coverage data
        COMMAND ${LCOV_PATH} --zerocounters --directory .
        
        # Run tests
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
        
        # Capture coverage data
        COMMAND ${LCOV_PATH} 
            --capture 
            --directory . 
            --output-file coverage.info
            --rc geninfo_unexecuted_blocks=1
            --ignore-errors inconsistent,mismatch,negative,inconsistent
            --quiet
        
        # Remove unwanted files from coverage
        COMMAND ${LCOV_PATH}
            --remove coverage.info 
            '/usr/*' 
            '*/tests/*' 
            '*/build/_deps/*'
            --output-file coverage.info
            --ignore-errors inconsistent,unused 
            --quiet
        
        # Generate HTML report
        COMMAND ${GENHTML_PATH}
            coverage.info
            --output-directory coverage_html
            --ignore-errors inconsistent
            --quiet
        
        COMMAND ${CMAKE_COMMAND} -E echo "Coverage report generated in: ${CMAKE_BINARY_DIR}/coverage_html/index.html"
        
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
    
    # Add dependency on tests
    if(VISIONCORE_BUILD_TESTS)
        add_dependencies(coverage test_filters test_sources test_pipeline)
    endif()
else()
    message(STATUS "lcov or genhtml not found - coverage target will not be available")
endif()

# ========================
# Static Analysis Target
# ========================
find_program(CPPCHECK_PATH cppcheck)

if(CPPCHECK_PATH)
    message(STATUS "cppcheck found: ${CPPCHECK_PATH}")
    
    # Collect all source files (excluding tests)
    file(GLOB_RECURSE PROJECT_SOURCES 
        ${CMAKE_SOURCE_DIR}/src/*.cpp
        ${CMAKE_SOURCE_DIR}/src/*.hpp
    )
    
    add_custom_target(cppcheck
        COMMENT "Running static analysis with cppcheck on project files only"
        COMMAND ${CPPCHECK_PATH}
            --enable=warning,style,performance,portability
            --suppress=missingIncludeSystem
            --suppress=missingInclude
            --suppress=unusedFunction
            --inline-suppr
            --quiet
            -I ${CMAKE_SOURCE_DIR}/src
            ${PROJECT_SOURCES}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
else()
    message(STATUS "cppcheck not found - cppcheck target will not be available")
endif()
