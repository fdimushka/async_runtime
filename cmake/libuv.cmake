# Try to find the path
find_path(LIBUV_INCLUDE_DIR NAMES uv.h)

# Try to find the library
find_library(LIBUV_LIBRARY NAMES uv libuv)

# Handle the QUIETLY/REQUIRED arguments, set LIBUV_FOUND if all variables are
# found
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(LIBUV
        REQUIRED_VARS
        LIBUV_LIBRARY
        LIBUV_INCLUDE_DIR)

# Hide internal variables
MARK_AS_ADVANCED(LIBUV_INCLUDE_DIR LIBUV_LIBRARY)

# Set standard variables
if(LIBUV_FOUND)
    set(LIBUV_INCLUDE_DIRS "${LIBUV_INCLUDE_DIR}")
    set(LIBUV_LIBRARIES "${LIBUV_LIBRARY}")
endif()
