set(FLATBUFFERS_BUILD_TESTS OFF CACHE BOOL "disable tests" FORCE)
set(FLATBUFFERS_BUILD_FLATC ON CACHE BOOL "enable flatc" FORCE)
add_subdirectory(third_party/flatbuffers)