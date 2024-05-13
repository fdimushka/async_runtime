cmake_minimum_required(VERSION 3.16)

set(LIB_NAME ar)
set(VERSION 0.1.0)

project(${LIB_NAME} VERSION ${VERSION})

option(WITH_TRACE "Build with tracing" OFF)
option(WITH_TESTS "Build with tests" OFF)
option(WITH_EXAMPLES "Build with examples" ON)
option(WITH_DOCS "Build with docs" OFF)
option(WITH_PROFILER "Build with profiler" ON)
option(WITH_BENCHMARKS "Build with benchmarks" OFF)
option(WITH_NUMA "Use numa topology" ON)
option(WITH_TBB "Use tbb" ON)
option(WITH_OPENCV "Use opencv" OFF)

message(STATUS "Build with tracing: ${WITH_TRACE}")
message(STATUS "Build with tests: ${WITH_TESTS}")
message(STATUS "Build with examples: ${WITH_EXAMPLES}")
message(STATUS "Build with docs: ${WITH_DOCS}")
message(STATUS "Build with benchmarks: ${WITH_BENCHMARKS}")
message(STATUS "Build with profiler: ${WITH_PROFILER}")
message(STATUS "Build with numa: ${WITH_NUMA}")
message(STATUS "Build with opencv: ${WITH_OPENCV}")
message(STATUS "Build with tbb: ${WITH_TBB}")

set(USE_TRACE OFF)
set(USE_TESTS OFF)
set(USE_PROFILER OFF)
set(USE_OPENCV OFF)
set(USE_TBB OFF)

if (WITH_TESTS)
    set(USE_TESTS ON)
endif()

if (WITH_TRACE)
    set(USE_TRACE ON)
endif()

if (WITH_PROFILER)
    set(USE_PROFILER ON)
    include(cmake/flatbuffers.cmake)
endif()

if (WITH_NUMA)
    set(USE_NUMA ON)
endif()

if (WITH_OPENCV)
    set(USE_OPENCV ON)
endif()

if (WITH_TBB)
    set(USE_TBB ON)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(COMMON_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0")
    set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -fstack-protector-strong -fno-omit-frame-pointer")
elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(COMMON_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0")
    set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -fstack-protector-strong -fno-omit-frame-pointer")
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

configure_file(version.in ${CMAKE_CURRENT_BINARY_DIR}/version.h)

include(cmake/boost.cmake)

if (WITH_TRACE)
    include(cmake/trace.cmake)
endif()

if (WITH_DOCS)
    include(cmake/doxygen.cmake)
endif()

configure_file(config.in ${CMAKE_CURRENT_BINARY_DIR}/config.hpp)

file(GLOB SOURCES   src/*.cpp
                    src/io/*.cpp
                    src/dataflow/*.cpp)
file(GLOB HEADERS include/${LIB_NAME}/*.hpp include/${LIB_NAME}/dataflow/*.hpp)

if (WITH_OPENCV)
    include(cmake/opencv.cmake)
else()
    list(REMOVE_ITEM SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/opencv_executor.cpp")
endif()

if (WITH_TBB)
    include(cmake/tbb.cmake)
endif()

include_directories(include/)
include_directories(src/)
include_directories(src/io)
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${LibUV_INCLUDE_DIR})
include_directories(third_party)

add_library(${LIB_NAME} ${ASM_SOURCES} ${SOURCES})

target_link_libraries(${LIB_NAME} pthread flatbuffers ${BOOST_LIBRARIES})

if (WITH_NUMA)
    target_link_libraries(${LIB_NAME} numa)
endif()

if (WITH_OPENCV)
    target_link_libraries(${LIB_NAME} ${OpenCV_LIBS})
endif()

if (WITH_TBB)
    target_link_libraries(${LIB_NAME} ${TBB_LIBS})
endif()

if (WITH_PROFILER)
    add_custom_target(
            fbs_generation
            COMMAND ${CMAKE_CURRENT_BINARY_DIR}/third_party/flatbuffers/flatc --cpp ${CMAKE_CURRENT_SOURCE_DIR}/src/fbs/profiler_schema.fbs &&
                    ${CMAKE_CURRENT_BINARY_DIR}/third_party/flatbuffers/flatc -o ${CMAKE_CURRENT_SOURCE_DIR}/profiler-viewer/ --ts ${CMAKE_CURRENT_SOURCE_DIR}/src/fbs/profiler_schema.fbs
            DEPENDS flatc)
    add_dependencies(${LIB_NAME} fbs_generation)
endif()

if (WITH_TESTS)
    add_subdirectory(tests)
endif()

if (WITH_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()

if (WITH_EXAMPLES)
    add_subdirectory(examples)
endif()

set(INSTALL_HEADERS ${HEADERS})
set_target_properties(${LIB_NAME} PROPERTIES PUBLIC_HEADER "${INSTALL_HEADERS}")

include(GNUInstallDirs)
install(TARGETS ${LIB_NAME}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PUBLIC_HEADER DESTINATION include/${LIB_NAME} COMPONENT
        )