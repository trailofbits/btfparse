#
# Copyright (c) 2021-present, Trail of Bits, Inc.
# All rights reserved.
#
# This source code is licensed in accordance with the terms specified in
# the LICENSE file found in the root directory of this source tree.
#

if(NOT "${CMAKE_BUILD_TYPE}" STREQUAL "Release" AND
   NOT "${CMAKE_BUILD_TYPE}" STREQUAL "Debug" AND
   NOT "${CMAKE_BUILD_TYPE}" STREQUAL "RelWithDebInfo")

  set(default_cmake_build_type "RelWithDebInfo")

  if(NOT "${CMAKE_BUILD_TYPE}" STREQUAL "")
    message(WARNING "btfparse: Invalid build type specified: ${CMAKE_BUILD_TYPE}")
  endif()

  message(WARNING "btfparse: Setting CMAKE_BUILD_TYPE to ${default_cmake_build_type}")
  set(CMAKE_BUILD_TYPE "${default_cmake_build_type}" CACHE STRING "btfparse: Build type (default: ${default_cmake_build_type})" FORCE)
endif()

option(BTFPARSE_ENABLE_TOOLS "Set to ON to build the tools" false)
option(BTFPARSE_ENABLE_TESTS "Set to ON to build the tests" false)
option(BTFPARSE_OMIT_FRAME_POINTERS "Set to ON to omit frame pointers" false)
option(BTFPARSE_ENABLE_SANITIZERS "Set to ON to enable sanitizers" false)

set(CMAKE_EXPORT_COMPILE_COMMANDS true CACHE BOOL "Export the compile_commands.json file (forced)" FORCE)
