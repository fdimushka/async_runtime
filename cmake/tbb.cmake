find_package(TBB REQUIRED)
if(TBB_FOUND)
    set(TBB_LIBS TBB::tbb)
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__TBB_DYNAMIC_LOAD_ENABLED=0")
    set(TBB_TEST OFF)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=stringop-overflow")

    add_subdirectory(third_party/oneTBB)
    include_directories(third_party/oneTBB/include)
    set(TBB_LIBS tbb)
endif()

list(APPEND SOURCES src/tbb/tbb_executor.cpp)
list(APPEND SOURCES src/tbb/tbb_stream.cpp)
list(APPEND SOURCES src/tbb/tbb_delayed_scheduler.cpp)


include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/tbb)