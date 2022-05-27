#
# Copyright (c) 2021-present, Trail of Bits, Inc.
# All rights reserved.
#
# This source code is licensed in accordance with the terms specified in
# the LICENSE file found in the root directory of this source tree.
#

add_library("btfparse-utils" INTERFACE)
target_include_directories("btfparse-utils" SYSTEM INTERFACE
  include
)

if(BTFPARSE_ENABLE_TESTS)
  add_executable("btfparse-utils-tests"
    tests/main.cpp
    tests/result.cpp
  )

  target_include_directories("btfparse-utils-tests" PRIVATE
    src
  )

  target_link_libraries("btfparse-utils-tests" PRIVATE
    "btfparse_cxx_settings"
    "btfparse-utils"
    "external::doctest"
  )

  add_test(
    NAME btfparse-utils-tests
    COMMAND btfparse-utils-tests
  )
endif()
