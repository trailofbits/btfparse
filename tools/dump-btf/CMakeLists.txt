#
# Copyright (c) 2021-present, Trail of Bits, Inc.
# All rights reserved.
#
# This source code is licensed in accordance with the terms specified in
# the LICENSE file found in the root directory of this source tree.
#

add_executable("dump-btf"
  src/main.cpp

  src/utils.h
  src/utils.cpp
)

target_link_libraries("dump-btf" PRIVATE
  "btfparse_cxx_settings"
  "btfparse"
)

if(BTFPARSE_ENABLE_TESTS)
  add_test(
    NAME dump-btf_bpftool_compatibility
    COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/tests/bpftool_compatibility.sh"
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
  )
endif()
