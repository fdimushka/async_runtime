cmake_minimum_required(VERSION 3.16)

set(LIB_NAME ar)
set(VERSION 0.1.0)

project(${LIB_NAME} VERSION ${VERSION})

option(WITH_TRACE "Build with tracing" OFF)
option(WITH_TESTS "Build with tests" OFF)
option(WITH_EXAMPLES "Build with examples" ON)
option(WITH_DOCS "Build with docs" OFF)
option(WITH_BENCHMARKS "Build with benchmarks" OFF)

message(STATUS "Build with tracing: ${WITH_TRACE}")
message(STATUS "Build with tests: ${WITH_TESTS}")
message(STATUS "Build with examples: ${WITH_EXAMPLES}")
message(STATUS "Build with docs: ${WITH_DOCS}")
message(STATUS "Build with benchmarks: ${WITH_BENCHMARKS}")

set(USE_TRACE OFF)
set(USE_TESTS OFF)

if (WITH_TESTS)
    set(USE_TESTS ON)
endif()

if (WITH_TRACE)
    set(USE_TRACE ON)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(COMMON_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -O3")
elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(COMMON_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -O0")
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

configure_file(version.in ${CMAKE_CURRENT_BINARY_DIR}/version.h)

include(cmake/asm.cmake)
include(cmake/libuv.cmake)

if (WITH_TRACE)
    include(cmake/trace.cmake)
endif()

if (WITH_DOCS)
    include(cmake/doxygen.cmake)
endif()

configure_file(config.in ${CMAKE_CURRENT_SOURCE_DIR}/include/ar/config.hpp)

file(GLOB SOURCES src/*.cpp)
file(GLOB HEADERS include/${LIB_NAME}/*.hpp)

include_directories(include/)
include_directories(src/)
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${LibUV_INCLUDE_DIR})

add_library(${LIB_NAME} ${ASM_SOURCES} ${SOURCES})

target_link_libraries(${LIB_NAME} pthread ${LibUV_LIBRARIES})

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

file(COPY data/ DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/data)