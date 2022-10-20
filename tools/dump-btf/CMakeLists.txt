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
