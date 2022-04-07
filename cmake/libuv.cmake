# Try to find the path
find_path(LibUV_INCLUDE_DIR NAMES uv.h)

# Try to find the library
find_library(LibUV_LIBRARY NAMES uv libuv)

# Handle the QUIETLY/REQUIRED arguments, set LIBUV_FOUND if all variables are
# found
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(LibUV
        REQUIRED_VARS
        LibUV_LIBRARY
        LibUV_INCLUDE_DIR)

# Hide internal variables
MARK_AS_ADVANCED(LibUV_INCLUDE_DIR LIBUV_LIBRARY)

# Set standard variables
if(LibUV_FOUND)
    set(LibUV_INCLUDE_DIRS "${LibUV_INCLUDE_DIR}")
    set(LibUV_LIBRARIES "${LibUV_LIBRARY}")
endif()
