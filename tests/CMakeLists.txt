cmake_minimum_required( VERSION 3.0 )

project( tests CXX )

OPTION(WITH_EXTRA_TESTS "build also module tests and others" OFF)

include_directories(${CATCH_HEADER_DIR})

# sources:
file(GLOB TESTS_SRC_CPP *.cpp)
set(TESTS_SRC ${TESTS_SRC_CPP})

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(COMMON_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -O3")
elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(COMMON_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -O0")
endif()

## main-s for reporter-specific test sources:
#set( NAMES_REPORTERS ar )
#
#foreach( reporter ${NAMES_REPORTERS} )
#    list( APPEND SOURCES_SPECIFIC_REPORTERS_MAIN ${BASENAMES_REPORTERS_MAIN}${reporter}.cpp )
#endforeach()

set( SOURCES_ALL ${TESTS_SRC})

foreach( name ${SOURCES_ALL} )
    list( APPEND SOURCES_ALL_PATH ${TESTS_DIR}/${name} )
endforeach()

# create target names:
string( REPLACE ".cpp" "" BASENAMES_SINGLE_FILE     "${SOURCES_ALL}" )
string( REPLACE "${CMAKE_CURRENT_SOURCE_DIR}/" "" BASENAMES_SINGLE_FILE     "${BASENAMES_SINGLE_FILE}" )

set( TARGETS_SINGLE_FILE     ${BASENAMES_SINGLE_FILE} )

set( TARGETS_ALL ${TARGETS_SINGLE_FILE})


foreach( name ${TARGETS_SINGLE_FILE} )
    message( STATUS "Unit tests: ${name}" )
    add_executable( ${name} ${CMAKE_CURRENT_SOURCE_DIR}/${name}.cpp )
endforeach()

include(CTest)
include(Catch.cmake)

foreach( name ${TARGETS_ALL} )
    target_include_directories( ${name} PRIVATE ${CATCH_HEADER_DIR} ${CATCH_DIR} )
    target_compile_options(${name} PRIVATE ${CXX_WARNING_FLAGS})
    target_link_libraries(${name} ar pthread)
endforeach()

foreach( name ${TARGETS_ALL} )
    catch_discover_tests( ${name} )
endforeach()
