find_package(OpenCV REQUIRED)

set(USE_OPENCV_CUDA OFF)

if (OpenCV_CUDA_VERSION)
    set(USE_OPENCV_CUDA ON)
    message(STATUS "OpenCV cuda version: ${OpenCV_CUDA_VERSION}")
endif()

include_directories(${OpenCV_INCLUDE_DIRS})
