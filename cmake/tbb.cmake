#set(BUILD_SHARED_LIBS ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__TBB_DYNAMIC_LOAD_ENABLED=0")
set(TBB_TEST OFF)
add_subdirectory(third_party/oneTBB)

list(APPEND SOURCES src/tbb/tbb_executor.cpp)
list(APPEND SOURCES src/tbb/tbb_io_executor.cpp)
list(APPEND SOURCES src/tbb/tbb_stream.cpp)
list(APPEND SOURCES src/tbb/tbb_delayed_scheduler.cpp)

include_directories(third_party/oneTBB/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/tbb)