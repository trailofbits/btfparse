#
# Copyright (c) 2021-present, Trail of Bits, Inc.
# All rights reserved.
#
# This source code is licensed in accordance with the terms specified in
# the LICENSE file found in the root directory of this source tree.
#

cmake_minimum_required(VERSION 3.16.4)
project("btfparse")

include("cmake/options.cmake")
include("cmake/utils.cmake")

# Needed to be able to check if the compiler supports linking with PIE,
# otherwise the resulting executable or shared library won't be PIE.
include(CheckPIESupported)
check_pie_supported()

if(NOT CMAKE_C_LINK_PIE_SUPPORTED OR NOT CMAKE_CXX_LINK_PIE_SUPPORTED)
    message(FATAL_ERROR "The current compiler does not support -fPIE or -pie")
endif()

if(BTFPARSE_ENABLE_TESTS)
  include(CTest)
  enable_testing()
endif()

add_subdirectory("libraries")
add_subdirectory("btfparse")

if(BTFPARSE_ENABLE_TOOLS)
  add_subdirectory("tools")
endif()
