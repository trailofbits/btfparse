#
# Copyright (c) 2021-present, Trail of Bits, Inc.
# All rights reserved.
#
# This source code is licensed in accordance with the terms specified in
# the LICENSE file found in the root directory of this source tree.
#

cmake_minimum_required(VERSION 3.16.4)

include("cmake/options.cmake")
include("cmake/utils.cmake")

project("BTFParse")

if(BTFPARSE_ENABLE_TESTS)
  include(CTest)
  enable_testing()
endif()

generateSettingsTargets()

add_subdirectory("libraries")
add_subdirectory("btfparse")

if(BTFPARSE_ENABLE_TOOLS)
  add_subdirectory("tools")
endif()
