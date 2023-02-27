cmake_minimum_required( VERSION 3.0 )

project( examples CXX )

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(AR_INSTALL_DIR /usr/local/)

OPTION(WITH_EXTRA_TESTS "build also module tests and others" OFF)

include_directories(${CATCH_HEADER_DIR})

link_directories(/usr/local/lib64/)

# sources:
file(GLOB EXAMPLES_SRC_CPP *.cpp)
set(EXAMPLES_SRC ${EXAMPLES_SRC_CPP})

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(COMMON_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -O3")
elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(COMMON_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -O0")
endif()

set( SOURCES_ALL ${EXAMPLES_SRC})

# create target names:
string( REPLACE ".cpp" "" BASENAMES_SINGLE_FILE     "${SOURCES_ALL}" )
string( REPLACE "${CMAKE_CURRENT_SOURCE_DIR}/" "" BASENAMES_SINGLE_FILE     "${BASENAMES_SINGLE_FILE}" )

set( TARGETS_SINGLE_FILE     ${BASENAMES_SINGLE_FILE} )

set( TARGETS_ALL ${TARGETS_SINGLE_FILE})

foreach( name ${TARGETS_SINGLE_FILE} )
    message( STATUS "Example: ${name}" )
    add_executable( ${name} ${CMAKE_CURRENT_SOURCE_DIR}/${name}.cpp )
    target_link_libraries(${name} ar uv)
endforeach()
