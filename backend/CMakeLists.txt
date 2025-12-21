cmake_minimum_required(VERSION 3.22)


project(
  VisionCore 
  VERSION 0.1.0 
  LANGUAGES CXX
)

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

find_package(OpenCV 4 REQUIRED)
find_package(Threads REQUIRED)



# ========================
# Sources auto-detection 
# ========================

file(GLOB_RECURSE VISIONCORE_SOURCES
    CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

# ====================
# VisionCore Library 
# ====================

add_library(visioncore ${VISIONCORE_SOURCES})

target_include_directories(visioncore
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(visioncore
    PUBLIC
        ${OpenCV_LIBS}
        Threads::Threads
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
)

target_compile_features(visioncore_app PRIVATE cxx_std_20)


# ========================
# Tests (optional)
# ========================
if(VISIONCORE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
