//
// Copyright (c) 2021-present, Trail of Bits, Inc.
// All rights reserved.
//
// This source code is licensed in accordance with the terms specified in
// the LICENSE file found in the root directory of this source tree.
//

#pragma once

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Type.h>

#include "btfparse/ibtf.h"

namespace btfparse {
using BTFIdentifier = std::uint32_t;
using BTFIdToLLVMTypeMap = std::unordered_map<BTFIdentifier, llvm::Type *>;

BTFIdToLLVMTypeMap convertBTFTypesToLLVMTypes(const BTFTypeMap &btf_type_map,
                                              llvm::LLVMContext &context);
} // namespace btfparse
