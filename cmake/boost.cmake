find_package(Boost 1.70.0 REQUIRED COMPONENTS thread chrono system regex context)
include_directories(${Boost_INCLUDE_DIRS})
link_directories(${Boost_LIBRARY_DIRS})