#
# Copyright (c) 2021-present, Trail of Bits, Inc.
# All rights reserved.
#
# This source code is licensed in accordance with the terms specified in
# the LICENSE file found in the root directory of this source tree.
#

add_library(btfparse-llvm
  include/btfparse/illvmbridge.h
  src/illvmbridge.cpp

  src/llvmbridge.h
  src/llvmbridge.cpp
)

target_include_directories(btfparse-llvm PUBLIC
  include
)

target_link_libraries(btfparse-llvm
  PRIVATE
    btfparse_cxx_target_settings

  PUBLIC
    btfparse
)

find_package(LLVM REQUIRED CONFIG)
llvm_map_components_to_libnames(llvm_library_list
  core
)

add_library(thirdparty_llvm INTERFACE)
target_link_libraries(thirdparty_llvm INTERFACE ${llvm_library_list})

target_include_directories(thirdparty_llvm SYSTEM INTERFACE
  ${LLVM_INCLUDE_DIRS}
)

target_compile_definitions(thirdparty_llvm INTERFACE
  ${LLVM_DEFINITIONS}
  LLVM_VERSION_MAJOR=${LLVM_VERSION_MAJOR}
  LLVM_VERSION_MINOR=${LLVM_VERSION_MINOR}
)

# Ubuntu/Debian workaround
if(EXISTS "/usr/include/llvm-${LLVM_VERSION_MAJOR}")
  target_include_directories(thirdparty_llvm SYSTEM INTERFACE
    "/usr/include/llvm-${LLVM_VERSION_MAJOR}"
  )
endif()

target_link_libraries(btfparse-llvm PUBLIC
  thirdparty_llvm
)
