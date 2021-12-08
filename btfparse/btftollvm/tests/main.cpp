//
// Copyright (c) 2021-present, Trail of Bits, Inc.
// All rights reserved.
//
// This source code is licensed in accordance with the terms specified in
// the LICENSE file found in the root directory of this source tree.
//

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <btftollvm/btftollvm.h>

namespace btfparse {
TEST_CASE("dump llvm") {
  llvm::LLVMContext context;
  BTFTypeMap map;
  IntBTFType int_type{};
  int_type.name = "btf_i32";
  int_type.bits = 32;
  int_type.size = 4;
  int_type.offset = 0;

  PtrBTFType ptr_type;
  ptr_type.type = 1;

  map.emplace(1, std::move(int_type));
  map.emplace(2, std::move(ptr_type));

  auto llvm_map = convertBTFTypesToLLVMTypes(map, context);
}

} // namespace btfparse
