include(FindPkgConfig)
find_package(PkgConfig)

find_package(OpenCV REQUIRED)

set(USE_OPENCV_CUDA OFF)

if (OpenCV_CUDA_VERSION)
    set(USE_OPENCV_CUDA ON)
endif()

if (NOT USE_OPENCV_CUDA)
    message( FATAL_ERROR "OpenCV with CUDA not found, CMake will exit." )
endif()

message(STATUS "OpenCV CUDA version: ${OpenCV_CUDA_VERSION}")

include_directories(${OpenCV_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/gpu)

list(APPEND SOURCES src/gpu/gpu.cpp)
