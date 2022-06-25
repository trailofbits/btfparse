#
# Copyright (c) 2021-present, Trail of Bits, Inc.
# All rights reserved.
#
# This source code is licensed in accordance with the terms specified in
# the LICENSE file found in the root directory of this source tree.
#

add_library("btfparse"
  include/btfparse/ibtf.h
  src/ibtf.cpp

  include/btfparse/ibtfheadergenerator.h
  src/ibtfheadergenerator.cpp

  src/btf.h
  src/btf.cpp

  src/btfheadergenerator.h
  src/btfheadergenerator.cpp

  src/btf_types.h
)

target_link_libraries("btfparse"
  PRIVATE
    "btfparse_cxx_settings"

  PUBLIC
    "btfparse-utils"
    "btfparse-filereader"
)

target_include_directories("btfparse" PRIVATE
  include
)

target_include_directories("btfparse" SYSTEM INTERFACE
  include
)
