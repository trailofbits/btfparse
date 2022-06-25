#
# Copyright (c) 2021-present, Trail of Bits, Inc.
# All rights reserved.
#
# This source code is licensed in accordance with the terms specified in
# the LICENSE file found in the root directory of this source tree.
#

add_executable("include-gen"
  src/main.cpp
)

target_link_libraries("include-gen" PRIVATE
  "btfparse_cxx_settings"
  "btfparse"
)
