#
# Copyright (c) 2021-present, Trail of Bits, Inc.
# All rights reserved.
#
# This source code is licensed in accordance with the terms specified in
# the LICENSE file found in the root directory of this source tree.
#

if(BTFPARSE_ENABLE_DUMPBTF)
  add_subdirectory("dump-btf")
endif()

if(BTFPARSE_ENABLE_BTFPARSELLVM)
  add_subdirectory("btfparse-llvm")
endif()
