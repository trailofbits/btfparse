//
// Copyright (c) 2021-present, Trail of Bits, Inc.
// All rights reserved.
//
// This source code is licensed in accordance with the terms specified in
// the LICENSE file found in the root directory of this source tree.
//

#pragma once

#include <optional>
#include <unordered_map>
#include <unordered_set>

#include <btfparse/illvmbridge.h>

namespace btfparse {

class LLVMBridge final : public ILLVMBridge {
public:
  LLVMBridge(llvm::Module &module, const IBTF &btf);
  virtual ~LLVMBridge();

  virtual Result<llvm::Type *, LLVMBridgeError>
  getType(const std::string &name) const override;

  virtual Result<llvm::Value *, LLVMBridgeError>
  load(llvm::IRBuilder<> &builder, llvm::Value *value_pointer,
       const std::string &path) const override;

private:
  struct PrivateData;
  std::unique_ptr<PrivateData> d;

  std::optional<LLVMBridgeError> importAllTypes();
  std::optional<LLVMBridgeError> indexTypesByName();
  std::optional<LLVMBridgeError> importType(std::uint32_t id,
                                            const BTFType &type);

public:
  struct Context final {
    BTFTypeMap btf_type_map;

    std::unordered_map<std::string, std::uint32_t> name_to_btf_type_id;
    std::unordered_set<std::string> blocked_name_list;

    std::unordered_map<std::uint32_t, llvm::Type *> btf_type_id_to_llvm;
    std::unordered_map<llvm::Type *, std::uint32_t> llvm_to_btf_type_id;
  };

  static Result<llvm::Type *, LLVMBridgeError> getType(const Context &context,
                                                       const std::string &name);

  static std::vector<std::string> tokenizePath(const std::string &path);

  static llvm::StructType *
  getOrCreateOpaqueStruct(Context &context, llvm::Module &module,
                          std::uint32_t id,
                          const std::optional<std::string> &opt_name);

  static void saveType(Context &context, std::uint32_t id, llvm::Type *type,
                       const std::optional<std::string> &opt_name);

  static std::optional<LLVMBridgeError> skipType(Context &context,
                                                 llvm::Module &module,
                                                 std::uint32_t id,
                                                 const BTFType &type);

  static std::optional<LLVMBridgeError> importIntType(Context &context,
                                                      llvm::Module &module,
                                                      std::uint32_t id,
                                                      const BTFType &type);

  static std::optional<LLVMBridgeError> importPtrType(Context &context,
                                                      llvm::Module &module,
                                                      std::uint32_t id,
                                                      const BTFType &type);

  static std::optional<LLVMBridgeError> importArrayType(Context &context,
                                                        llvm::Module &module,
                                                        std::uint32_t id,
                                                        const BTFType &type);

  static std::optional<LLVMBridgeError> importStructType(Context &context,
                                                         llvm::Module &module,
                                                         std::uint32_t id,
                                                         const BTFType &type);

  static std::optional<LLVMBridgeError> importUnionType(Context &context,
                                                        llvm::Module &module,
                                                        std::uint32_t id,
                                                        const BTFType &type);

  static std::optional<LLVMBridgeError> importEnumType(Context &context,
                                                       llvm::Module &module,
                                                       std::uint32_t id,
                                                       const BTFType &type);

  static std::optional<LLVMBridgeError> importFwdType(Context &context,
                                                      llvm::Module &module,
                                                      std::uint32_t id,
                                                      const BTFType &type);

  static std::optional<LLVMBridgeError> importTypedefType(Context &context,
                                                          llvm::Module &module,
                                                          std::uint32_t id,
                                                          const BTFType &type);

  static std::optional<LLVMBridgeError> importVolatileType(Context &context,
                                                           llvm::Module &module,
                                                           std::uint32_t id,
                                                           const BTFType &type);

  static std::optional<LLVMBridgeError> importRestrictType(Context &context,
                                                           llvm::Module &module,
                                                           std::uint32_t id,
                                                           const BTFType &type);

  static std::optional<LLVMBridgeError> importConstType(Context &context,
                                                        llvm::Module &module,
                                                        std::uint32_t id,
                                                        const BTFType &type);

  static std::optional<LLVMBridgeError>
  importFuncProtoType(Context &context, llvm::Module &module, std::uint32_t id,
                      const BTFType &type);

  static std::optional<LLVMBridgeError> importFloatType(Context &context,
                                                        llvm::Module &module,
                                                        std::uint32_t id,
                                                        const BTFType &type);
};

} // namespace btfparse
