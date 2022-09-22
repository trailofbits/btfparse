#
# Copyright (c) 2021-present, Trail of Bits, Inc.
# All rights reserved.
#
# This source code is licensed in accordance with the terms specified in
# the LICENSE file found in the root directory of this source tree.
#

add_library("btfparse-filereader"
  include/btfparse/istream.h

  include/btfparse/ifilereader.h
  src/ifilereader.cpp

  src/filereader.h
  src/filereader.cpp

  src/memoryfileadapter.h
  src/memoryfileadapter.cpp
)

target_link_libraries("btfparse-filereader"
  PRIVATE
    "btfparse_cxx_settings"

  PUBLIC
    "btfparse-utils"
)

target_include_directories("btfparse-filereader" PRIVATE
  include
)

target_include_directories("btfparse-filereader" SYSTEM INTERFACE
  include
)

if(BTFPARSE_ENABLE_TESTS)
  add_executable("btfparse-filereader-tests"
    tests/main.cpp
  )

  target_include_directories("btfparse-filereader-tests" PRIVATE
    src
  )

  target_link_libraries("btfparse-filereader-tests" PRIVATE
    "btfparse_cxx_settings"
    "btfparse-filereader"
    "external::doctest"
  )

  add_test(
    NAME btfparse-filereader-tests
    COMMAND btfparse-filereader-tests
  )
endif()
