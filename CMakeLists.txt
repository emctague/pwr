cmake_minimum_required (VERSION 2.6)
project (Pwr)

set (CMAKE_CXX_STANDARD 11)

set (Pwr_VERSION_MAJOR 1)
set (Pwr_VERSION_MINOR 0)

# configure a header file to pass some of the CMake settings
# to the source code
configure_file (
  "${PROJECT_SOURCE_DIR}/src/config.h.in"
  "${PROJECT_BINARY_DIR}/config.h"
)

include_directories ("${PROJECT_BINARY_DIR}")

add_executable (pwr src/pwr.cpp)

install (
  TARGETS pwr DESTINATION bin
  PERMISSIONS
  OWNER_READ OWNER_WRITE OWNER_EXECUTE
  GROUP_READ GROUP_EXECUTE
  WORLD_READ WORLD_EXECUTE
  SETUID
)

include (InstallRequiredSystemLibraries)
set (CPACK_RESOURCE_FILE_LICENSE  
     "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set (CPACK_PACKAGE_VERSION_MAJOR "${Pwr_VERSION_MAJOR}")
set (CPACK_PACKAGE_VERSION_MINOR "${Pwr_VERSION_MINOR}")
include (CPack)