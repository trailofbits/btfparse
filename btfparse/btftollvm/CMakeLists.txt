#
# Copyright (c) 2021-present, Trail of Bits, Inc.
# All rights reserved.
#
# This source code is licensed in accordance with the terms specified in
# the LICENSE file found in the root directory of this source tree.
#

find_package(LLVM REQUIRED)

add_library(thirdparty_llvm INTERFACE)

target_link_libraries(thirdparty_llvm INTERFACE
  LLVMCore
)

target_include_directories(thirdparty_llvm SYSTEM INTERFACE
 "${LLVM_INCLUDE_DIRS}"
)

add_library(btftollvm
  include/btftollvm/btftollvm.h
  src/btftollvm.cpp
)

target_link_libraries(btftollvm 
  PRIVATE
    btfparse_cxx_target_settings

  PUBLIC
    btfparse
    thirdparty_llvm
)

target_include_directories(btftollvm PUBLIC
  include
)

if(BTFPARSE_ENABLE_TESTS)
  add_executable(btftollvm-tests
    tests/main.cpp
  )

  target_link_libraries(btftollvm-tests PRIVATE
    btfparse_cxx_target_settings
    btftollvm
    thirdparty_doctest
  )

  target_include_directories(btftollvm-tests PRIVATE
    include
  )

endif()
