//
// Copyright (c) 2021-present, Trail of Bits, Inc.
// All rights reserved.
//
// This source code is licensed in accordance with the terms specified in
// the LICENSE file found in the root directory of this source tree.
//

#include "llvmbridge.h"

#include <optional>

namespace btfparse {

namespace {

const std::unordered_map<BTFKind, std::optional<LLVMBridgeError> (*)(
                                      LLVMBridge::Context &, llvm::Module &,
                                      std::uint32_t, const BTFType &)>
    kBTFTypeImporterMap = {
        {BTFKind::Void, LLVMBridge::skipType},
        {BTFKind::Int, LLVMBridge::importIntType},
        {BTFKind::Ptr, LLVMBridge::importPtrType},
        {BTFKind::Array, LLVMBridge::importArrayType},
        {BTFKind::Struct, LLVMBridge::importStructType},
        {BTFKind::Union, LLVMBridge::importUnionType},
        {BTFKind::Enum, LLVMBridge::importEnumType},
        {BTFKind::Fwd, LLVMBridge::importFwdType},
        {BTFKind::Typedef, LLVMBridge::importTypedefType},
        {BTFKind::Volatile, LLVMBridge::importVolatileType},
        {BTFKind::Const, LLVMBridge::importConstType},
        {BTFKind::Restrict, LLVMBridge::importRestrictType},
        {BTFKind::Func, LLVMBridge::skipType},
        {BTFKind::FuncProto, LLVMBridge::importFuncProtoType},
        {BTFKind::Var, LLVMBridge::skipType},
        {BTFKind::DataSec, LLVMBridge::skipType},
        {BTFKind::Float, LLVMBridge::importFloatType},
};

}

struct LLVMBridge::PrivateData final {
  PrivateData(llvm::Module &module_) : module(module_) {}

  llvm::Module &module;
  Context context;
};

LLVMBridge::LLVMBridge(llvm::Module &module, const IBTF &btf)
    : d(new PrivateData(module)) {

  d->context.btf_type_map = btf.getAll();

  auto opt_error = importAllTypes();
  if (opt_error.has_value()) {
    throw opt_error.value();
  }

  opt_error = indexTypesByName();
  if (opt_error.has_value()) {
    throw opt_error.value();
  }
}

LLVMBridge::~LLVMBridge() {}

Result<llvm::Type *, LLVMBridgeError>
LLVMBridge::getType(const std::string &name) const {
  return getType(d->context, name);
}

Result<llvm::Value *, LLVMBridgeError>
LLVMBridge::load(llvm::IRBuilder<> &builder, llvm::Value *value_pointer,
                 const std::string &path) const {
  auto tokenized_path = tokenizePath(path);

  for (const auto &x : tokenized_path) {
    std::cout << x << ", ";
  }
  std::cout << std::endl;

  return nullptr;
}

std::optional<LLVMBridgeError> LLVMBridge::importAllTypes() {
  // Initialize the type queue
  std::vector<std::uint32_t> next_id_queue(d->context.btf_type_map.size());

  std::size_t i{};
  for (const auto &btf_type_p : d->context.btf_type_map) {
    next_id_queue[i] = btf_type_p.first;
    ++i;
  }

  // BTF id 0 is never referenced, and is defined as the `void` type. Manually
  // initialize this into the map
  auto &llvm_context = d->module.getContext();
  auto void_type = llvm::Type::getVoidTy(llvm_context);

  d->context.btf_type_id_to_llvm.insert({0, void_type});
  d->context.llvm_to_btf_type_id.insert({void_type, 0});

  // Attempt to import types in a loop until there is no new update
  while (!next_id_queue.empty()) {
    auto current_id_queue = std::move(next_id_queue);
    next_id_queue.clear();

    bool updated{false};

    for (const auto &id : current_id_queue) {
      const auto &btf_type = d->context.btf_type_map.at(id);

      // In case we fail with a `MissingDependency` error, put this
      // type back into the queue so we'll try again later
      auto opt_error = importType(id, btf_type);
      if (opt_error.has_value()) {
        auto error = opt_error.value();
        if (error.get() != LLVMBridgeErrorCode::MissingDependency) {
          return error;
        }

        next_id_queue.push_back(id);

      } else {
        updated = true;
      }
    }

    if (!updated) {
      break;
    }
  }

  // If the next queue is not empty, we have failed to import one or
  // more types
  if (!next_id_queue.empty()) {
    return LLVMBridgeError(LLVMBridgeErrorCode::MissingDependency);
  }

  return std::nullopt;
}

std::optional<LLVMBridgeError> LLVMBridge::indexTypesByName() {
  return std::nullopt;
}

std::optional<LLVMBridgeError> LLVMBridge::importType(std::uint32_t id,
                                                      const BTFType &type) {
  auto importer_it = kBTFTypeImporterMap.find(IBTF::getBTFTypeKind(type));
  if (importer_it == kBTFTypeImporterMap.end()) {
    return LLVMBridgeError(LLVMBridgeErrorCode::UnsupportedBTFType);
  }

  const auto &importer = importer_it->second;
  return importer(d->context, d->module, id, type);
}

Result<llvm::Type *, LLVMBridgeError>
LLVMBridge::getType(const Context &context, const std::string &name) {
  auto btf_type_id_it = context.name_to_btf_type_id.find(name);
  if (btf_type_id_it == context.name_to_btf_type_id.end()) {
    return LLVMBridgeError(LLVMBridgeErrorCode::NotFound);
  }

  auto btf_type_id = btf_type_id_it->second;

  auto llvm_type_it = context.btf_type_id_to_llvm.find(btf_type_id);
  if (llvm_type_it == context.btf_type_id_to_llvm.end()) {
    return LLVMBridgeError(LLVMBridgeErrorCode::UnsupportedBTFType);
  }

  return llvm_type_it->second;
}

std::vector<std::string> LLVMBridge::tokenizePath(const std::string &path) {
  std::vector<std::string> string_list;

  std::size_t start{};

  while (start < path.size()) {
    auto end = path.find_first_of('.', start);
    if (end == std::string::npos) {
      end = path.size();
    }

    auto str = path.substr(start, end - start);
    string_list.push_back(std::move(str));

    start = end + 1;
  }

  return string_list;
}

llvm::StructType *LLVMBridge::getOrCreateOpaqueStruct(
    Context &context, llvm::Module &module, std::uint32_t id,
    const std::optional<std::string> &opt_name) {
  llvm::StructType *llvm_struct_type{nullptr};

  auto llvm_type_it = context.btf_type_id_to_llvm.find(id);
  if (llvm_type_it != context.btf_type_id_to_llvm.end()) {
    llvm_struct_type = static_cast<llvm::StructType *>(llvm_type_it->second);

  } else {
    auto &llvm_context = module.getContext();
    llvm_struct_type = llvm::StructType::create(llvm_context);
    saveType(context, id, llvm_struct_type, opt_name);
  }

  return llvm_struct_type;
}

void LLVMBridge::saveType(Context &context, std::uint32_t id, llvm::Type *type,
                          const std::optional<std::string> &opt_name) {
  context.btf_type_id_to_llvm.insert({id, type});
  context.llvm_to_btf_type_id.insert({type, id});

  if (opt_name.has_value()) {
    const auto &name = opt_name.value();

    if (context.blocked_name_list.count(name) != 0) {
      return;
    }

    if (context.name_to_btf_type_id.count(name) > 0) {
      context.blocked_name_list.insert(name);
      context.name_to_btf_type_id.erase(name);

    } else {
      context.name_to_btf_type_id.insert({name, id});
    }
  }
}

std::optional<LLVMBridgeError> LLVMBridge::skipType(Context &, llvm::Module &,
                                                    std::uint32_t,
                                                    const BTFType &) {
  return std::nullopt;
}

std::optional<LLVMBridgeError> LLVMBridge::importIntType(Context &context,
                                                         llvm::Module &module,
                                                         std::uint32_t id,
                                                         const BTFType &type) {
  if (context.btf_type_id_to_llvm.count(id) > 0) {
    return std::nullopt;
  }

  auto &llvm_context = module.getContext();
  llvm::Type *llvm_type{nullptr};

  const auto &int_type = std::get<IntBTFType>(type);
  switch (int_type.size) {
  case 1:
    llvm_type = llvm::Type::getInt8Ty(llvm_context);
    break;

  case 2:
    llvm_type = llvm::Type::getInt16Ty(llvm_context);
    break;

  case 4:
    llvm_type = llvm::Type::getInt32Ty(llvm_context);
    break;

  case 8:
    llvm_type = llvm::Type::getInt64Ty(llvm_context);
    break;

  case 16:
    llvm_type = llvm::Type::getInt128Ty(llvm_context);
    break;

  default:
    return LLVMBridgeError(LLVMBridgeErrorCode::UnsupportedBTFType);
  }

  saveType(context, id, llvm_type, std::nullopt);
  return std::nullopt;
}

std::optional<LLVMBridgeError> LLVMBridge::importPtrType(Context &context,
                                                         llvm::Module &,
                                                         std::uint32_t id,
                                                         const BTFType &type) {
  if (context.btf_type_id_to_llvm.count(id) > 0) {
    return std::nullopt;
  }

  const auto &ptr_type = std::get<PtrBTFType>(type);

  auto llvm_type_it = context.btf_type_id_to_llvm.find(ptr_type.type);
  if (llvm_type_it == context.btf_type_id_to_llvm.end()) {
    return LLVMBridgeError(LLVMBridgeErrorCode::MissingDependency);
  }

  auto base_llvm_type = llvm_type_it->second;
  auto llvm_type = base_llvm_type->getPointerTo();

  saveType(context, id, llvm_type, std::nullopt);
  return std::nullopt;
}

std::optional<LLVMBridgeError>
LLVMBridge::importArrayType(Context &context, llvm::Module &module,
                            std::uint32_t id, const BTFType &type) {
  if (context.btf_type_id_to_llvm.count(id) > 0) {
    return std::nullopt;
  }

  const auto &array_type = std::get<ArrayBTFType>(type);

  auto llvm_elem_type_it = context.btf_type_id_to_llvm.find(array_type.type);
  if (llvm_elem_type_it == context.btf_type_id_to_llvm.end()) {
    return LLVMBridgeError(LLVMBridgeErrorCode::MissingDependency);
  }

  auto llvm_elem_type = llvm_elem_type_it->second;
  auto llvm_type = llvm::ArrayType::get(llvm_elem_type, array_type.nelems);

  saveType(context, id, llvm_type, std::nullopt);
  return std::nullopt;
}

std::optional<LLVMBridgeError>
LLVMBridge::importStructType(Context &context, llvm::Module &module,
                             std::uint32_t id, const BTFType &type) {
  const auto &struct_type = std::get<StructBTFType>(type);

  auto llvm_struct_type =
      getOrCreateOpaqueStruct(context, module, id, struct_type.opt_name);
  if (!llvm_struct_type->isOpaque()) {
    return std::nullopt;
  }

  auto member_list = struct_type.member_list;
  std::sort(member_list.begin(), member_list.end(),
            [](const StructBTFType::Member &lhs,
               const StructBTFType::Member &rhs) -> bool {
              return lhs.offset < rhs.offset;
            });

  llvm::DataLayout data_layout(&module);
  auto &llvm_context = module.getContext();

  std::vector<llvm::Type *> llvm_type_list;
  std::uint32_t current_offset{};

  for (const auto &member : member_list) {
    auto member_offset = member.offset / 8;

    auto padding_byte_count = member_offset - current_offset;
    if (padding_byte_count != 0) {
      auto byte_type = llvm::Type::getInt8Ty(llvm_context);
      auto padding_type = llvm::ArrayType::get(byte_type, padding_byte_count);
      llvm_type_list.push_back(padding_type);
    }

    auto llvm_member_type_it = context.btf_type_id_to_llvm.find(member.type);
    if (llvm_member_type_it == context.btf_type_id_to_llvm.end()) {
      return LLVMBridgeError(LLVMBridgeErrorCode::MissingDependency);
    }

    auto llvm_member_type = llvm_member_type_it->second;
    llvm_type_list.push_back(llvm_member_type);

    if (!llvm_member_type->isSized()) {
      return LLVMBridgeError(LLVMBridgeErrorCode::MissingDependency);
    }

    auto member_size = static_cast<std::uint32_t>(
        data_layout.getTypeAllocSize(llvm_member_type));

    current_offset = member_offset + member_size;
  }

  llvm_struct_type->setBody(llvm_type_list, true);
  return std::nullopt;
}

std::optional<LLVMBridgeError>
LLVMBridge::importUnionType(Context &context, llvm::Module &module,
                            std::uint32_t id, const BTFType &type) {
  const auto &union_type = std::get<UnionBTFType>(type);

  auto llvm_struct_type =
      getOrCreateOpaqueStruct(context, module, id, union_type.opt_name);
  if (!llvm_struct_type->isOpaque()) {
    return std::nullopt;
  }

  llvm::DataLayout data_layout(&module);
  std::uint32_t union_size{};

  for (const auto &member : union_type.member_list) {
    auto llvm_member_type_it = context.btf_type_id_to_llvm.find(member.type);
    if (llvm_member_type_it == context.btf_type_id_to_llvm.end()) {
      return LLVMBridgeError(LLVMBridgeErrorCode::MissingDependency);
    }

    auto llvm_member_type = llvm_member_type_it->second;
    if (!llvm_member_type->isSized()) {
      return LLVMBridgeError(LLVMBridgeErrorCode::MissingDependency);
    }

    auto member_size = static_cast<std::uint32_t>(
        data_layout.getTypeAllocSize(llvm_member_type));

    union_size = std::max(union_size, member_size);
  }

  auto &llvm_context = module.getContext();
  auto byte_type = llvm::Type::getInt8Ty(llvm_context);

  std::vector<llvm::Type *> llvm_type_list{
      llvm::ArrayType::get(byte_type, union_size)};

  llvm_struct_type->setBody(llvm_type_list, true);
  return std::nullopt;
}

std::optional<LLVMBridgeError> LLVMBridge::importEnumType(Context &context,
                                                          llvm::Module &module,
                                                          std::uint32_t id,
                                                          const BTFType &type) {
  if (context.btf_type_id_to_llvm.count(id) > 0) {
    return std::nullopt;
  }

  auto &llvm_context = module.getContext();
  const auto &enum_type = std::get<EnumBTFType>(type);

  llvm::Type *llvm_type{nullptr};

  switch (enum_type.size) {
  case 1:
    llvm_type = llvm::Type::getInt8Ty(llvm_context);
    break;

  case 2:
    llvm_type = llvm::Type::getInt16Ty(llvm_context);
    break;

  case 4:
    llvm_type = llvm::Type::getInt32Ty(llvm_context);
    break;

  case 8:
    llvm_type = llvm::Type::getInt64Ty(llvm_context);
    break;

  case 16:
    llvm_type = llvm::Type::getInt128Ty(llvm_context);
    break;

  default:
    return LLVMBridgeError(LLVMBridgeErrorCode::UnsupportedBTFType);
  }

  saveType(context, id, llvm_type, std::nullopt);
  return std::nullopt;
}

std::optional<LLVMBridgeError> LLVMBridge::importFwdType(Context &context,
                                                         llvm::Module &module,
                                                         std::uint32_t id,
                                                         const BTFType &type) {
  if (context.btf_type_id_to_llvm.count(id) > 0) {
    return std::nullopt;
  }

  auto &llvm_context = module.getContext();
  auto llvm_type = llvm::StructType::get(llvm_context);

  saveType(context, id, llvm_type, std::nullopt);
  return std::nullopt;
}

std::optional<LLVMBridgeError>
LLVMBridge::importTypedefType(Context &context, llvm::Module &module,
                              std::uint32_t id, const BTFType &type) {
  if (context.btf_type_id_to_llvm.count(id) > 0) {
    return std::nullopt;
  }

  const auto &typedef_type = std::get<TypedefBTFType>(type);

  auto llvm_type_it = context.btf_type_id_to_llvm.find(typedef_type.type);
  if (llvm_type_it == context.btf_type_id_to_llvm.end()) {
    return LLVMBridgeError(LLVMBridgeErrorCode::MissingDependency);
  }

  auto llvm_type = llvm_type_it->second;

  saveType(context, id, llvm_type, typedef_type.name);
  return std::nullopt;
}

std::optional<LLVMBridgeError>
LLVMBridge::importVolatileType(Context &context, llvm::Module &module,
                               std::uint32_t id, const BTFType &type) {
  if (context.btf_type_id_to_llvm.count(id) > 0) {
    return std::nullopt;
  }

  const auto &volatile_type = std::get<VolatileBTFType>(type);

  auto llvm_type_it = context.btf_type_id_to_llvm.find(volatile_type.type);
  if (llvm_type_it == context.btf_type_id_to_llvm.end()) {
    return LLVMBridgeError(LLVMBridgeErrorCode::MissingDependency);
  }

  auto llvm_type = llvm_type_it->second;

  saveType(context, id, llvm_type, std::nullopt);
  return std::nullopt;
}

std::optional<LLVMBridgeError>
LLVMBridge::importRestrictType(Context &context, llvm::Module &module,
                               std::uint32_t id, const BTFType &type) {
  if (context.btf_type_id_to_llvm.count(id) > 0) {
    return std::nullopt;
  }

  const auto &restrict_type = std::get<RestrictBTFType>(type);

  auto llvm_type_it = context.btf_type_id_to_llvm.find(restrict_type.type);
  if (llvm_type_it == context.btf_type_id_to_llvm.end()) {
    return LLVMBridgeError(LLVMBridgeErrorCode::MissingDependency);
  }

  auto llvm_type = llvm_type_it->second;

  saveType(context, id, llvm_type, std::nullopt);
  return std::nullopt;
}

std::optional<LLVMBridgeError>
LLVMBridge::importConstType(Context &context, llvm::Module &module,
                            std::uint32_t id, const BTFType &type) {
  if (context.btf_type_id_to_llvm.count(id) > 0) {
    return std::nullopt;
  }

  const auto &const_type = std::get<ConstBTFType>(type);

  auto llvm_type_it = context.btf_type_id_to_llvm.find(const_type.type);
  if (llvm_type_it == context.btf_type_id_to_llvm.end()) {
    return LLVMBridgeError(LLVMBridgeErrorCode::MissingDependency);
  }

  auto llvm_type = llvm_type_it->second;

  saveType(context, id, llvm_type, std::nullopt);
  return std::nullopt;
}

std::optional<LLVMBridgeError>
LLVMBridge::importFuncProtoType(Context &context, llvm::Module &module,
                                std::uint32_t id, const BTFType &type) {
  if (context.btf_type_id_to_llvm.count(id) > 0) {
    return std::nullopt;
  }

  std::vector<llvm::Type *> param_type_list;

  const auto &func_proto_type = std::get<FuncProtoBTFType>(type);
  for (const auto &param : func_proto_type.param_list) {
    auto param_llvm_type_it = context.btf_type_id_to_llvm.find(param.type);
    if (param_llvm_type_it == context.btf_type_id_to_llvm.end()) {
      return LLVMBridgeError(LLVMBridgeErrorCode::MissingDependency);
    }

    auto param_llvm_type = param_llvm_type_it->second;
    param_type_list.push_back(param_llvm_type);
  }

  auto return_llvm_type_it =
      context.btf_type_id_to_llvm.find(func_proto_type.return_type);

  if (return_llvm_type_it == context.btf_type_id_to_llvm.end()) {
    return LLVMBridgeError(LLVMBridgeErrorCode::MissingDependency);
  }

  auto return_llvm_type = return_llvm_type_it->second;

  auto llvm_type = llvm::FunctionType::get(return_llvm_type, param_type_list,
                                           func_proto_type.is_variadic);

  saveType(context, id, llvm_type, std::nullopt);
  return std::nullopt;
}

std::optional<LLVMBridgeError>
LLVMBridge::importFloatType(Context &context, llvm::Module &module,
                            std::uint32_t id, const BTFType &type) {
  if (context.btf_type_id_to_llvm.count(id) > 0) {
    return std::nullopt;
  }

  const auto &float_type = std::get<FloatBTFType>(type);

  auto &llvm_context = module.getContext();
  llvm::Type *llvm_type{nullptr};

  switch (float_type.size) {
  case 4:
    llvm_type = llvm::Type::getFloatTy(llvm_context);
    break;

  case 8:
    llvm_type = llvm::Type::getDoubleTy(llvm_context);
    break;

  default:
    return LLVMBridgeError(LLVMBridgeErrorCode::UnsupportedBTFType);
  }

  saveType(context, id, llvm_type, std::nullopt);
  return std::nullopt;
}

} // namespace btfparse
