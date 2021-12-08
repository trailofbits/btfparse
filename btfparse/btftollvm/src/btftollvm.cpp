//
// Copyright (c) 2021-present, Trail of Bits, Inc.
// All rights reserved.
//
// This source code is licensed in accordance with the terms specified in
// the LICENSE file found in the root directory of this source tree.
//

#include "btftollvm/btftollvm.h"
#include "btfparse/ibtf.h"

#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Verifier.h>

#include <iostream>
#include <unordered_map>

namespace btfparse {

llvm::StructType *btfTypeToLLVMType(const BTFType &btf_type_variant,
                                    const BTFTypeMap &btf_type_map,
                                    const BTFIdToLLVMTypeMap &llvm_map,
                                    llvm::LLVMContext &context) {
  auto btf_type_visitor = [&context, &btf_type_map, &llvm_map](
                              const auto &btf_type) -> llvm::StructType * {
    using T = std::decay_t<decltype(btf_type)>;

    if constexpr (std::is_same_v<T, std::monostate>) {
      return nullptr;
    } else if constexpr (std::is_same_v<T, IntBTFType>) {
      return intBTFToLLVM(btf_type, context);

    } else if constexpr (std::is_same_v<T, PtrBTFType>) {
      return ptrBTFToLLVM(btf_type, btf_type_map, llvm_map, context);
    }

    return nullptr;
  };

  return std::visit(btf_type_visitor, btf_type_variant);
}

llvm::StructType *intBTFToLLVM(const IntBTFType &btf_type,
                               llvm::LLVMContext &context) {
  llvm::StructType *st = llvm::StructType::create(
      {llvm::Type::getIntNTy(context, btf_type.bits)}, btf_type.name);

  return st;
}

llvm::StructType *ptrBTFToLLVM(const PtrBTFType &btf_type,
                               const BTFTypeMap &btf_type_map,
                               const BTFIdToLLVMTypeMap &llvm_map,
                               llvm::LLVMContext &context) {

  auto llvm_it = llvm_map.find(btf_type.type);

  if (llvm_it == llvm_map.end()) {
    // Error ?
    return nullptr;
  }

  const auto &underlying_struct_type = llvm_it->second;

  return llvm::StructType::create(
      {llvm::PointerType::get(underlying_struct_type, 0)},
      "ptr" + underlying_struct_type->getStructName().str());
}

BTFIdToLLVMTypeMap convertBTFTypesToLLVMTypes(const BTFTypeMap &btf_type_map,
                                              llvm::LLVMContext &context) {
  BTFIdToLLVMTypeMap llvm_map;

  for (const auto &[btf_type_id, btf_type] : btf_type_map) {
    auto st = btfTypeToLLVMType(btf_type, btf_type_map, llvm_map, context);

    if (st == nullptr) {
      // Not Supported for now
      continue;
    }

    llvm_map.emplace(btf_type_id, st);
  }

  llvm::Module module("btfmodule", context);

  auto *st = llvm_map.begin()->second;

  if (st == nullptr) {
    std::cerr << "Failed to get a valid struct" << std::endl;
    return llvm_map;
  }

  auto function_type =
      llvm::FunctionType::get(llvm::Type::getVoidTy(context), {}, false);
  auto function = llvm::Function::Create(
      function_type, llvm::Function::ExternalLinkage, "my_main", module);

  llvm::BasicBlock *bb = llvm::BasicBlock::Create(context, "entry", function);

  if (bb == nullptr) {
    std::cerr << "Failed to create a BasicBlock!" << std::endl;
    return llvm_map;
  }

  llvm::IRBuilder builder(context);
  builder.SetInsertPoint(bb);
  builder.CreateAlloca(st);
  builder.CreateRetVoid();

  std::string error_buffer;
  llvm::raw_string_ostream error_stream(error_buffer);
  llvm::verifyModule(module, &error_stream);

  std::string output_buffer;
  llvm::raw_string_ostream output_stream(output_buffer);
  module.print(output_stream, nullptr);

  std::cout << output_buffer << std::endl;

  return llvm_map;
}
} // namespace btfparse
