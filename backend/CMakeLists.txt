cmake_minimum_required(VERSION 3.22)

project(
  VisionCore 
  VERSION 0.1.0 
  LANGUAGES CXX C
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
# Dependencies 
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

# ----------------------
# uWebSockets via FetchContent
# ----------------------
include(FetchContent)

FetchContent_Declare(
    uWebSockets
    GIT_REPOSITORY https://github.com/uNetworking/uWebSockets.git
    GIT_TAG master
)

FetchContent_MakeAvailable(uWebSockets)

# uWebSockets est une bibliothèque header-only, on crée une target INTERFACE
add_library(uWebSockets INTERFACE)

# Créer un lien symbolique ou utiliser le bon chemin d'include
# Les headers sont directement dans src/, pas dans src/uWebSockets/
target_include_directories(uWebSockets INTERFACE 
    ${uwebsockets_SOURCE_DIR}/src
    ${usockets_SOURCE_DIR}/src
)

# Alternative: créer un alias pour permettre #include <uWebSockets/App.h>
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/include)
file(CREATE_LINK 
    ${uwebsockets_SOURCE_DIR}/src 
    ${CMAKE_BINARY_DIR}/include/uWebSockets 
    SYMBOLIC
)
target_include_directories(uWebSockets INTERFACE 
    ${CMAKE_BINARY_DIR}/include
)

# uWebSockets dépend de uSockets
FetchContent_Declare(
    uSockets
    GIT_REPOSITORY https://github.com/uNetworking/uSockets.git
    GIT_TAG master
)

FetchContent_MakeAvailable(uSockets)

# Créer une bibliothèque pour uSockets avec tous les fichiers nécessaires
set(USOCKETS_SOURCES
    ${usockets_SOURCE_DIR}/src/bsd.c
    ${usockets_SOURCE_DIR}/src/context.c
    ${usockets_SOURCE_DIR}/src/loop.c
    ${usockets_SOURCE_DIR}/src/socket.c
    ${usockets_SOURCE_DIR}/src/udp.c
    ${usockets_SOURCE_DIR}/src/quic.c
)

# Ajouter les sources d'évènements
if(WIN32)
    list(APPEND USOCKETS_SOURCES ${usockets_SOURCE_DIR}/src/eventing/libuv.c)
else()
    list(APPEND USOCKETS_SOURCES ${usockets_SOURCE_DIR}/src/eventing/epoll_kqueue.c)
endif()

# Ajouter les sources crypto selon la disponibilité d'OpenSSL
find_package(OpenSSL)
if(OPENSSL_FOUND)
    list(APPEND USOCKETS_SOURCES 
        ${usockets_SOURCE_DIR}/src/crypto/openssl.c
    )
    # Vérifier si sni_tree.cpp existe avant de l'ajouter
    if(EXISTS ${usockets_SOURCE_DIR}/src/crypto/sni_tree.cpp)
        list(APPEND USOCKETS_SOURCES ${usockets_SOURCE_DIR}/src/crypto/sni_tree.cpp)
    endif()
endif()

add_library(usockets STATIC ${USOCKETS_SOURCES})
target_include_directories(usockets PUBLIC 
    ${usockets_SOURCE_DIR}/src
)

# Configuration OpenSSL
if(OPENSSL_FOUND)
    target_link_libraries(usockets PUBLIC OpenSSL::SSL OpenSSL::Crypto)
    target_compile_definitions(usockets PUBLIC LIBUS_USE_OPENSSL)
else()
    # Sans OpenSSL, on n'utilise pas les fonctionnalités SSL
    target_compile_definitions(usockets PUBLIC LIBUS_NO_SSL)
endif()

target_link_libraries(uWebSockets INTERFACE usockets)

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
        uWebSockets
        z
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
