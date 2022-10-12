//
// Copyright (c) 2021-present, Trail of Bits, Inc.
// All rights reserved.
//
// This source code is licensed in accordance with the terms specified in
// the LICENSE file found in the root directory of this source tree.
//

#include "btfheadergenerator.h"

#include <algorithm>
#include <optional>
#include <streambuf>
#include <unordered_set>
#include <variant>

#include <btfparse/ibtf.h>

namespace btfparse {

namespace {

template <typename Type> const Type &getTypeAs(const BTFType &btf_type) {
  if (!std::holds_alternative<Type>(btf_type)) {
    throw std::logic_error("Invalid getTypeAs in file " __FILE__ " at line " +
                           std::to_string(__LINE__));
  }

  return std::get<Type>(btf_type);
}

template <typename Type> Type &getTypeAsMutable(BTFType &btf_type) {
  if (!std::holds_alternative<Type>(btf_type)) {
    throw std::logic_error("Invalid getTypeAs in file " __FILE__ " at line " +
                           std::to_string(__LINE__));
  }

  return std::get<Type>(btf_type);
}

bool createInverseTypeTree(BTFHeaderGenerator::Context &context) {
  context.inverse_type_tree = {};

  for (const auto &p : context.type_tree) {
    const auto &parent_id = p.first;
    const auto &child_link_map = p.second;

    for (const auto &c : child_link_map) {
      const auto &child_id = c.first;

      auto it = context.inverse_type_tree.find(child_id);
      if (it == context.inverse_type_tree.end()) {
        auto insert_status = context.inverse_type_tree.insert({child_id, {}});
        it = insert_status.first;
      }

      auto &parent_links = it->second;
      parent_links.insert(parent_id);
    }
  }

  return true;
}

std::unordered_set<std::uint32_t>
collectChildNodes(BTFHeaderGenerator::Context context, std::uint32_t start) {
  std::unordered_set<std::uint32_t> next_queue{start};
  std::unordered_set<std::uint32_t> visited;

  while (!next_queue.empty()) {
    auto queue = std::move(next_queue);
    next_queue.clear();

    for (const auto &id : queue) {
      if (visited.count(id) > 0) {
        continue;
      }

      visited.insert(id);

      auto type_tree_it = context.type_tree.find(id);
      if (type_tree_it == context.type_tree.end()) {
        continue;
      }

      const auto &type_map = type_tree_it->second;
      for (const auto &p : type_map) {
        const auto &child_id = p.first;
        next_queue.insert(child_id);
      }
    }
  }

  return visited;
}

} // namespace

BTFHeaderGenerator::~BTFHeaderGenerator() {}

bool BTFHeaderGenerator::generate(std::string &header,
                                  const IBTF::Ptr &btf) const {
  header.clear();

  Context context;
  if (!saveBTFTypeMap(context, btf)) {
    return false;
  }

  if (!adjustTypeNames(context)) {
    return false;
  }

  scanTypes(context);

  if (!materializePadding(context)) {
    return false;
  }

  if (!createTypeTree(context)) {
    return false;
  }

  if (!adjustTypedefDependencyLoops(context)) {
    return false;
  }

  if (!createTypeQueue(context)) {
    return false;
  }

  std::stringstream buffer;
  if (!generateHeader(context, buffer)) {
    return false;
  }

  header = buffer.str();
  return true;
}

BTFHeaderGenerator::BTFHeaderGenerator() {}

bool BTFHeaderGenerator::saveBTFTypeMap(Context &context,
                                        const IBTF::Ptr &btf) {
  auto btf_type_map = btf->getAll();
  if (btf_type_map.size() == 0) {
    return false;
  }

  context.btf_type_map = std::move(btf_type_map);
  return true;
}

bool BTFHeaderGenerator::adjustTypeNames(Context &context) {
  std::unordered_set<std::string> visited_name_list;

  for (auto &p : context.btf_type_map) {
    const auto &id = p.first;

    bool can_be_named{false};
    bool can_be_renamed{false};
    bool uses_tag_type{false};
    bool is_enum{false};

    {
      const auto &btf_type = p.second;

      switch (btfparse::IBTF::getBTFTypeKind(btf_type)) {
      case btfparse::BTFKind::Struct:
      case btfparse::BTFKind::Union:
        uses_tag_type = true;
        can_be_named = true;
        can_be_renamed = true;
        break;

      case btfparse::BTFKind::Enum:
        is_enum = true;
        uses_tag_type = true;
        can_be_named = true;
        can_be_renamed = true;
        break;

      case btfparse::BTFKind::Typedef:
        can_be_named = true;
        can_be_renamed = true;
        break;

      case btfparse::BTFKind::Void:
      case btfparse::BTFKind::Int:
        can_be_named = true;
        break;

      case btfparse::BTFKind::Fwd:
      case btfparse::BTFKind::Ptr:
      case btfparse::BTFKind::Array:
      case btfparse::BTFKind::Volatile:
      case btfparse::BTFKind::Const:
      case btfparse::BTFKind::Restrict:
      case btfparse::BTFKind::Func:
      case btfparse::BTFKind::FuncProto:
      case btfparse::BTFKind::Var:
      case btfparse::BTFKind::DataSec:
      case btfparse::BTFKind::Float:
      default:
        break;
      }
    }

    if (can_be_named) {
      auto opt_current_type_name = getTypeName(context, id);
      if (!opt_current_type_name.has_value()) {
        if (!is_enum) {
          continue;
        }

        // Sometimes enum with the same exact ID are re-used by two similar
        // structs that have the same name. Always give them a name so that
        // the type in C matches
        auto new_name = "AnonymousEnum" + std::to_string(id);
        if (!setTypeName(context, id, new_name)) {
          return false;
        }

        opt_current_type_name = new_name;
      }

      auto current_type_name = opt_current_type_name.value();

      auto key = current_type_name;
      if (uses_tag_type) {
        key = std::string("tag-") + key;
      }

      if (visited_name_list.count(key) > 0) {
        if (!can_be_renamed) {
          return false;
        }

        current_type_name += "_" + std::to_string(id);
        if (!setTypeName(context, id, current_type_name)) {
          return false;
        }

        key = current_type_name;
        if (uses_tag_type) {
          key = std::string("tag-") + key;
        }
      }

      visited_name_list.insert(key);
    }

    if (is_enum) {
      auto &btf_type = p.second;
      auto &enum_btf_type = getTypeAsMutable<EnumBTFType>(btf_type);

      bool rename_values{false};
      for (const auto &value : enum_btf_type.value_list) {
        if (visited_name_list.count(value.name) > 0) {
          rename_values = true;
          break;
        }
      }

      if (rename_values) {
        const auto &enum_name = enum_btf_type.opt_name.value();
        for (auto &value : enum_btf_type.value_list) {
          value.name = enum_name + "_" + value.name;
        }
      }

      for (const auto &value : enum_btf_type.value_list) {
        visited_name_list.insert(value.name);
      }
    }
  }

  return true;
}

std::uint32_t BTFHeaderGenerator::generateTypeID(Context &context) {
  return ++context.btf_type_id_generator;
}

bool BTFHeaderGenerator::materializePadding(Context &context) {
  // Create a custom byte type that we'll use to generate
  // padding
  IntBTFType byte_type;
  byte_type.name = "unsigned char";
  byte_type.size = 1;
  byte_type.encoding = IntBTFType::Encoding::None;
  byte_type.offset = 0;
  byte_type.bits = 8;

  context.padding_byte_id = generateTypeID(context);
  context.btf_type_map.insert({context.padding_byte_id, std::move(byte_type)});

  // Add padding to all the struct types
  for (auto &p : context.btf_type_map) {
    auto btf_id = p.first;
    auto &btf_type = p.second;

    if (IBTF::getBTFTypeKind(btf_type) != BTFKind::Struct) {
      continue;
    }

    auto &struct_type = std::get<StructBTFType>(btf_type);
    if (!materializeStructPadding(context, btf_id, struct_type)) {
      return false;
    }
  }

  return true;
}

bool BTFHeaderGenerator::materializeStructPadding(
    Context &context, std::uint32_t id, StructBTFType &struct_btf_type) {
  auto member_list = std::move(struct_btf_type.member_list);
  struct_btf_type.member_list.clear();

  std::uint32_t current_offset{0};

  StructBTFType::Member padding_byte;
  padding_byte.type = context.padding_byte_id;

  for (const auto &member : member_list) {
    if (current_offset > member.offset) {
      return false;
    }

    if (member.offset != current_offset) {
      auto padding_bit_size = member.offset - current_offset;
      auto byte_padding = padding_bit_size / 8;

      padding_byte.opt_bitfield_size = 8;

      for (std::uint32_t i = 0; i < byte_padding; ++i) {
        padding_byte.offset = current_offset;
        struct_btf_type.member_list.push_back(padding_byte);

        current_offset += 8;
      }

      auto bit_padding = padding_bit_size % 8;
      if (bit_padding != 0) {
        padding_byte.offset = current_offset;
        padding_byte.opt_bitfield_size = bit_padding;

        struct_btf_type.member_list.push_back(padding_byte);

        current_offset += bit_padding;
      }
    }

    struct_btf_type.member_list.push_back(member);

    if (isBitfield(member)) {
      current_offset += member.opt_bitfield_size.value();

    } else {
      auto opt_type_size = getBTFTypeSize(context, member.type);
      if (!opt_type_size.has_value()) {
        return false;
      }

      auto type_size = opt_type_size.value();
      current_offset += type_size;
    }
  }

  auto padding_bit_size = (struct_btf_type.size * 8) - current_offset;
  if (padding_bit_size != 0) {
    auto byte_padding = padding_bit_size / 8;

    padding_byte.opt_bitfield_size = 8;

    for (std::uint32_t i = 0; i < byte_padding; ++i) {
      padding_byte.offset = current_offset;
      struct_btf_type.member_list.push_back(padding_byte);

      current_offset += 8;
    }

    auto bit_padding = padding_bit_size % 8;
    if (bit_padding != 0) {
      padding_byte.offset = current_offset;
      padding_byte.opt_bitfield_size = bit_padding;

      struct_btf_type.member_list.push_back(padding_byte);

      current_offset += bit_padding;
    }
  }

  if (current_offset != (struct_btf_type.size * 8)) {
    return false;
  }

  return true;
}

bool BTFHeaderGenerator::isBitfield(const StructBTFType::Member &member) {
  return (member.opt_bitfield_size.has_value() &&
          member.opt_bitfield_size.value() != 0);
}

std::optional<std::uint32_t>
BTFHeaderGenerator::getBTFTypeSize(const Context &context,
                                   const BTFType &type) {

  switch (IBTF::getBTFTypeKind(type)) {
  case BTFKind::Void: {
    return std::nullopt;
  }

  case BTFKind::Int: {
    const auto &btf_type = std::get<IntBTFType>(type);
    return btf_type.size * 8;
  }

  case BTFKind::Ptr: {
    return sizeof(void *) * 8;
  }

  case BTFKind::Array: {
    const auto &btf_type = std::get<ArrayBTFType>(type);

    auto opt_elem_size = getBTFTypeSize(context, btf_type.type);
    if (!opt_elem_size.has_value()) {
      return opt_elem_size;
    }

    return opt_elem_size.value() * btf_type.nelems;
  }

  case BTFKind::Struct: {
    const auto &btf_type = std::get<StructBTFType>(type);
    return btf_type.size * 8;
  }

  case BTFKind::Union: {
    const auto &btf_type = std::get<UnionBTFType>(type);
    return btf_type.size * 8;
  }

  case BTFKind::Enum: {
    const auto &btf_type = std::get<EnumBTFType>(type);
    return btf_type.size * 8;
  }

  case BTFKind::Fwd: {
    return std::nullopt;
  }

  case BTFKind::Typedef: {
    const auto &btf_type = std::get<TypedefBTFType>(type);
    return getBTFTypeSize(context, btf_type.type);
  }

  case BTFKind::Volatile: {
    const auto &btf_type = std::get<VolatileBTFType>(type);
    return getBTFTypeSize(context, btf_type.type);
  }

  case BTFKind::Const: {
    const auto &btf_type = std::get<ConstBTFType>(type);
    return getBTFTypeSize(context, btf_type.type);
  }

  case BTFKind::Restrict: {
    return std::nullopt;
  }

  case BTFKind::Func: {
    return std::nullopt;
  }

  case BTFKind::FuncProto: {
    return std::nullopt;
  }

  case BTFKind::Var: {
    return std::nullopt;
  }

  case BTFKind::DataSec: {
    return std::nullopt;
  }

  case BTFKind::Float: {
    const auto &btf_type = std::get<FloatBTFType>(type);
    return btf_type.size * 8;
  }
  }

  return 0;
}

std::optional<std::uint32_t>
BTFHeaderGenerator::getBTFTypeSize(const Context &context, std::uint32_t type) {

  auto type_it = context.btf_type_map.find(type);
  if (type_it == context.btf_type_map.end()) {
    return std::nullopt;
  }

  return getBTFTypeSize(context, type_it->second);
}

bool BTFHeaderGenerator::isValidTypeId(const Context &context,
                                       std::uint32_t id) {
  return context.btf_type_map.count(id) > 0;
}

bool BTFHeaderGenerator::isRenameableType(const Context &context,
                                          std::uint32_t id) {
  if (!isValidTypeId(context, id)) {
    return false;
  }

  const auto &btf_type = context.btf_type_map.at(id);

  switch (IBTF::getBTFTypeKind(btf_type)) {
  case BTFKind::Struct: {
    const auto &struct_btf_type = getTypeAs<StructBTFType>(btf_type);
    return struct_btf_type.opt_name.has_value();
  }

  case BTFKind::Union: {
    const auto &union_btf_type = getTypeAs<UnionBTFType>(btf_type);
    return union_btf_type.opt_name.has_value();
  }

  case BTFKind::Enum: {
    const auto &enum_btf_type = getTypeAs<EnumBTFType>(btf_type);
    return enum_btf_type.opt_name.has_value();
  }

  case BTFKind::Typedef: {
    return true;
  }

  default:
    return false;
  }
}

void BTFHeaderGenerator::scanTypes(Context &context) {
  context.top_level_type_list.clear();
  context.highest_btf_type_id = 0;

  for (const auto &p : context.btf_type_map) {
    const auto &id = p.first;
    context.highest_btf_type_id = std::max(context.highest_btf_type_id, id);

    const auto &btf_type = p.second;
    auto btf_kind = IBTF::getBTFTypeKind(btf_type);

    bool skip_type{true};

    switch (btf_kind) {
    case BTFKind::Struct:
    case BTFKind::Union:
    case BTFKind::Enum:
    case BTFKind::Typedef:
    case BTFKind::Fwd:
      skip_type = false;
      break;

    case BTFKind::Void:
    case BTFKind::Int:
    case BTFKind::Ptr:
    case BTFKind::Array:
    case BTFKind::Volatile:
    case BTFKind::Const:
    case BTFKind::Restrict:
    case BTFKind::Func:
    case BTFKind::FuncProto:
    case BTFKind::Var:
    case BTFKind::DataSec:
    case BTFKind::Float:
      break;
    }

    if (skip_type) {
      continue;
    }

    auto opt_type_name = getTypeName(context, id);
    if (!opt_type_name.has_value()) {
      continue;
    }

    if (btf_kind == BTFKind::Fwd) {
      const auto &type_name = opt_type_name.value();
      context.fwd_type_map.insert({type_name, id});
    }

    context.top_level_type_list.insert(id);
  }

  context.btf_type_id_generator = context.highest_btf_type_id + 1;
}

bool BTFHeaderGenerator::getTypeDependencies(
    const Context &context, std::vector<std::uint32_t> &dependency_list,
    std::uint32_t id) {

  dependency_list.clear();

  if (!isValidTypeId(context, id)) {
    return false;
  }

  const auto &btf_type = context.btf_type_map.at(id);
  switch (IBTF::getBTFTypeKind(btf_type)) {
  case BTFKind::Ptr: {
    const auto &ptr_btf_type = getTypeAs<PtrBTFType>(btf_type);
    dependency_list.push_back(ptr_btf_type.type);
    break;
  }

  case BTFKind::Array: {
    const auto &array_btf_type = getTypeAs<ArrayBTFType>(btf_type);
    dependency_list.push_back(array_btf_type.type);
    break;
  }

  case BTFKind::Struct: {
    const auto &struct_btf_type = getTypeAs<StructBTFType>(btf_type);
    for (const auto &member : struct_btf_type.member_list) {
      dependency_list.push_back(member.type);
    }

    break;
  }

  case BTFKind::Union: {
    const auto &union_btf_type = getTypeAs<UnionBTFType>(btf_type);
    for (const auto &member : union_btf_type.member_list) {
      dependency_list.push_back(member.type);
    }

    break;
  }

  case BTFKind::Typedef: {
    const auto &typedef_btf_type = getTypeAs<TypedefBTFType>(btf_type);
    dependency_list.push_back(typedef_btf_type.type);

    if (typedef_btf_type.type == 0) {
      break;
    }

    const auto &child_btf_type = context.btf_type_map.at(typedef_btf_type.type);
    auto child_btf_kind = IBTF::getBTFTypeKind(child_btf_type);

    bool recurse{false};
    if (child_btf_kind == BTFKind::Struct) {
      const auto &child_struct_type = getTypeAs<StructBTFType>(child_btf_type);
      recurse = !child_struct_type.opt_name.has_value();

    } else if (child_btf_kind == BTFKind::Union) {
      const auto &child_union_type = getTypeAs<UnionBTFType>(child_btf_type);
      recurse = !child_union_type.opt_name.has_value();
    }

    if (recurse) {
      if (!getTypeDependencies(context, dependency_list,
                               typedef_btf_type.type)) {
        return false;
      }
    }

    break;
  }

  case BTFKind::Volatile: {
    const auto &volatile_btf_type = getTypeAs<VolatileBTFType>(btf_type);
    dependency_list.push_back(volatile_btf_type.type);
    break;
  }

  case BTFKind::Const: {
    const auto &const_btf_type = getTypeAs<ConstBTFType>(btf_type);
    dependency_list.push_back(const_btf_type.type);
    break;
  }

  case BTFKind::Restrict: {
    const auto &restrict_btf_type = getTypeAs<RestrictBTFType>(btf_type);
    dependency_list.push_back(restrict_btf_type.type);
    break;
  }

  case BTFKind::FuncProto: {
    const auto &func_proto_btf_type = getTypeAs<FuncProtoBTFType>(btf_type);

    dependency_list.push_back(func_proto_btf_type.return_type);
    for (const auto &param : func_proto_btf_type.param_list) {
      dependency_list.push_back(param.type);
    }

    break;
  }

  case BTFKind::Void:
  case BTFKind::Int:
  case BTFKind::Enum:
  case BTFKind::Fwd:
  case BTFKind::Func:
  case BTFKind::Var:
  case BTFKind::DataSec:
  case BTFKind::Float:
    break;
  }

  return true;
}

std::optional<std::string>
BTFHeaderGenerator::getTypeName(const Context &context, std::uint32_t id) {
  if (!isValidTypeId(context, id)) {
    return std::nullopt;
  }

  const auto &btf_type = context.btf_type_map.at(id);

  switch (IBTF::getBTFTypeKind(btf_type)) {
  case BTFKind::Struct: {
    const auto &struct_btf_type = getTypeAs<StructBTFType>(btf_type);
    return struct_btf_type.opt_name;
  }

  case BTFKind::Union: {
    const auto &union_btf_type = getTypeAs<UnionBTFType>(btf_type);
    return union_btf_type.opt_name;
  }

  case BTFKind::Enum: {
    const auto &enum_btf_type = getTypeAs<EnumBTFType>(btf_type);
    return enum_btf_type.opt_name;
  }

  case BTFKind::Typedef: {
    const auto &typedef_btf_type = getTypeAs<TypedefBTFType>(btf_type);
    return typedef_btf_type.name;
  }

  case BTFKind::Fwd: {
    const auto &fwd_btf_type = getTypeAs<FwdBTFType>(btf_type);
    return fwd_btf_type.name;
  }

  case BTFKind::Void: {
    return "void";
  }

  case BTFKind::Int: {
    const auto &int_btf_type = getTypeAs<IntBTFType>(btf_type);
    return int_btf_type.name;
  }

  default:
    return std::nullopt;
  }
}

bool BTFHeaderGenerator::setTypeName(Context &context, std::uint32_t id,
                                     const std::string &name) {

  if (!isValidTypeId(context, id)) {
    return false;
  }

  auto &btf_type = context.btf_type_map.at(id);

  switch (IBTF::getBTFTypeKind(btf_type)) {
  case BTFKind::Struct: {
    auto &struct_btf_type = getTypeAsMutable<StructBTFType>(btf_type);
    struct_btf_type.opt_name = name;

    return true;
  }

  case BTFKind::Union: {
    auto &union_btf_type = getTypeAsMutable<UnionBTFType>(btf_type);
    union_btf_type.opt_name = name;

    return true;
  }

  case BTFKind::Enum: {
    auto &enum_btf_type = getTypeAsMutable<EnumBTFType>(btf_type);
    enum_btf_type.opt_name = name;

    return true;
  }

  case BTFKind::Typedef: {
    auto &typedef_btf_type = getTypeAsMutable<TypedefBTFType>(btf_type);
    typedef_btf_type.name = name;

    return true;
  }

  case BTFKind::Fwd: {
    auto &fwd_btf_type = getTypeAsMutable<FwdBTFType>(btf_type);
    fwd_btf_type.name = name;
  }

  default:
    return false;
  }
}

void BTFHeaderGenerator::resetIndent(Context &context) {
  context.indent_level = 0;
}

void BTFHeaderGenerator::increaseIndent(Context &context) {
  ++context.indent_level;
}

void BTFHeaderGenerator::decreaseIndent(Context &context) {
  --context.indent_level;
}

void BTFHeaderGenerator::generateIndent(const Context &context,
                                        std::stringstream &buffer) {
  for (std::size_t i = 0; i < context.indent_level; ++i) {
    buffer << "  ";
  }
}

bool BTFHeaderGenerator::createTypeTree(Context &context) {
  context.type_tree.clear();

  // We can only have the followind kinds in this vector:
  // * void (type 0)
  // * Struct
  // * Union
  // * Enum
  // * Typedef
  context.visited_type_list = {0};

  for (const auto &id : context.top_level_type_list) {
    std::vector<std::uint32_t> dependency_list;
    if (!getTypeDependencies(context, dependency_list, id)) {
      return false;
    }

    for (const auto &dependency_id : dependency_list) {
      if (!createTypeTreeHelper(context, false, id, dependency_id)) {
        return false;
      }
    }
  }

  return true;
}

bool BTFHeaderGenerator::createTypeTreeHelper(Context &context,
                                              bool inside_pointer,
                                              const std::uint32_t &parent,
                                              const std::uint32_t &id) {

  // Ignore void types
  if (id == 0) {
    return true;
  }

  const auto &btf_type = context.btf_type_map.at(id);

  auto btf_kind = btfparse::IBTF::getBTFTypeKind(btf_type);

  if (btf_kind == btfparse::BTFKind::Ptr) {
    const auto &ptr_btf_type = getTypeAs<btfparse::PtrBTFType>(btf_type);
    return createTypeTreeHelper(context, true, parent, ptr_btf_type.type);

  } else if (btf_kind == btfparse::BTFKind::Array) {
    const auto &array_btf_type = getTypeAs<btfparse::ArrayBTFType>(btf_type);
    return createTypeTreeHelper(context, inside_pointer, parent,
                                array_btf_type.type);

  } else if (btf_kind == btfparse::BTFKind::Volatile) {
    const auto &volatile_btf_type =
        getTypeAs<btfparse::VolatileBTFType>(btf_type);

    return createTypeTreeHelper(context, inside_pointer, parent,
                                volatile_btf_type.type);

  } else if (btf_kind == btfparse::BTFKind::Const) {
    const auto &const_btf_type = getTypeAs<btfparse::ConstBTFType>(btf_type);
    return createTypeTreeHelper(context, inside_pointer, parent,
                                const_btf_type.type);

  } else if (btf_kind == btfparse::BTFKind::Restrict) {
    const auto &restrict_btf_type =
        getTypeAs<btfparse::RestrictBTFType>(btf_type);

    return createTypeTreeHelper(context, inside_pointer, parent,
                                restrict_btf_type.type);

  } else if (btf_kind == btfparse::BTFKind::FuncProto) {
    const auto &func_proto_btf_type =
        getTypeAs<btfparse::FuncProtoBTFType>(btf_type);

    if (!createTypeTreeHelper(context, inside_pointer, parent,
                              func_proto_btf_type.return_type)) {
      return false;
    }

    for (const auto &param : func_proto_btf_type.param_list) {
      if (!createTypeTreeHelper(context, inside_pointer, parent, param.type)) {
        return false;
      }
    }

    return true;

  } else if (!isTopLevelTypeDeclaration(context, id)) {
    if (btf_kind == btfparse::BTFKind::Union ||
        btf_kind == btfparse::BTFKind::Struct) {

      std::vector<std::uint32_t> dependency_list;
      if (!getTypeDependencies(context, dependency_list, id)) {
        return false;
      }

      // Recurse into anonymous structs/unions. there should be no
      // way to cull this out: since it has no name, there is no
      // chance we have seen this already
      //
      // Since this is a nested type, we have to clear the 'inside_pointer'
      // flag
      for (const auto &dependency_id : dependency_list) {
        if (!createTypeTreeHelper(context, false, parent, dependency_id)) {
          return false;
        }
      }

      return true;
    }

    switch (btf_kind) {
    case btfparse::BTFKind::Int:
    case btfparse::BTFKind::Float:
    case btfparse::BTFKind::Enum:
      break;

    default: {
      std::stringstream error_buffer;
      error_buffer << "Invalid state. Encountered a BTF type #" << id
                   << " of unexpected kind: " << static_cast<int>(btf_kind);

      throw std::logic_error(error_buffer.str());
    }
    }

    return true;
  }

  auto link_list_it = context.type_tree.find(parent);
  if (link_list_it == context.type_tree.end()) {
    auto insert_status = context.type_tree.insert({parent, {}});
    link_list_it = insert_status.first;
  }

  auto &link_list = link_list_it->second;

  // this is a weak reference only if we can forward declare it
  auto weak_reference =
      inside_pointer && (btf_kind == btfparse::BTFKind::Struct ||
                         btf_kind == btfparse::BTFKind::Union);

  auto link_it = link_list.find(id);
  if (link_it == link_list.end()) {
    link_list.insert({id, weak_reference});
  } else {
    // always upgrade from weak to strong link
    auto &link_kind = link_it->second;
    if (link_kind) {
      link_kind = weak_reference;
    }
  }

  if (context.visited_type_list.count(id) > 0) {
    // Do not recurse into this type if we have seen it already
    return true;
  }

  context.visited_type_list.insert(id);

  std::vector<std::uint32_t> dependency_list;
  if (!getTypeDependencies(context, dependency_list, id)) {
    return false;
  }

  for (const auto &dependency_id : dependency_list) {
    if (!createTypeTreeHelper(context, false, id, dependency_id)) {
      return false;
    }
  }

  return true;
}

bool BTFHeaderGenerator::adjustTypedefDependencyLoops(Context &context) {
  // TODO(alessandro): We should just have a list of types
  // that need to be re-evaluated
  bool try_again{false};

  std::unordered_map<std::uint32_t, std::uint32_t> typedef_map;

  do {
    try_again = false;

    for (const auto &struct_id : context.top_level_type_list) {
      auto &struct_btf_type = context.btf_type_map.at(struct_id);

      auto btf_kind = btfparse::IBTF::getBTFTypeKind(struct_btf_type);
      if (btf_kind != btfparse::BTFKind::Struct &&
          btf_kind != btfparse::BTFKind::Union) {
        continue;
      }

      auto is_union = btf_kind == btfparse::BTFKind::Union;

      auto struct_dependency_list_it = context.type_tree.find(struct_id);
      if (struct_dependency_list_it == context.type_tree.end()) {
        continue;
      }

      auto &struct_dependency_list = struct_dependency_list_it->second;
      if (struct_dependency_list.empty()) {
        continue;
      }

      auto opt_struct_name = getTypeName(context, struct_id);
      if (!opt_struct_name.has_value()) {
        // Since this is a top level type, this should not be possible
        return false;
      }

      const auto &struct_name = opt_struct_name.value();

      for (const auto &p : struct_dependency_list) {
        auto &typedef_id = p.first;
        auto &typedef_btf_type = context.btf_type_map.at(typedef_id);

        btf_kind = btfparse::IBTF::getBTFTypeKind(typedef_btf_type);
        if (btf_kind != btfparse::BTFKind::Typedef) {
          continue;
        }

        auto typedef_dependency_list_it = context.type_tree.find(typedef_id);
        if (typedef_dependency_list_it == context.type_tree.end()) {
          // This typedef may not have a top level dependency. If that is
          // the case, then skip it
          continue;
        }

        auto &typedef_dependency_list = typedef_dependency_list_it->second;
        if (typedef_dependency_list.count(struct_id) == 0) {
          continue;
        }

        typedef_dependency_list.erase(struct_id);

        auto fwd_id = getOrCreateFwdType(context, is_union, struct_name);
        typedef_dependency_list.insert({fwd_id, false});

        typedef_map.insert({typedef_id, struct_id});

        try_again = true;
      }
    }
  } while (try_again);

  // Update the types that depend on the typedefs we patched. Since
  // the typedef and the struct are now generated together, we can
  // just change the typedef parents to point to the struct
  createInverseTypeTree(context);

  std::unordered_map<std::uint32_t, std::unordered_set<std::uint32_t>>
      child_node_list_map;

  for (const auto &p : typedef_map) {
    const auto &typedef_id = p.first;
    const auto &typedef_struct_id = p.second;

    auto inverse_type_tree_it = context.inverse_type_tree.find(typedef_id);
    if (inverse_type_tree_it == context.inverse_type_tree.end()) {
      continue;
    }

    const auto &typedef_user_list = inverse_type_tree_it->second;

    auto child_node_list_map_it = child_node_list_map.find(typedef_struct_id);

    if (child_node_list_map_it == child_node_list_map.end()) {
      auto struct_child_nodes = collectChildNodes(context, typedef_struct_id);

      auto insert_status = child_node_list_map.insert(
          {typedef_struct_id, std::move(struct_child_nodes)});

      child_node_list_map_it = insert_status.first;
    }

    const auto &struct_child_nodes = child_node_list_map_it->second;

    for (const auto &typedef_user : typedef_user_list) {
      if (typedef_user == typedef_struct_id) {
        continue;
      }

      if (struct_child_nodes.count(typedef_user) > 0) {
        continue;
      }

      auto type_tree_it = context.type_tree.find(typedef_user);
      if (type_tree_it == context.type_tree.end()) {
        continue;
      }

      auto &typedef_user_deps = type_tree_it->second;

      typedef_user_deps.erase(typedef_struct_id);
      typedef_user_deps.insert({typedef_struct_id, false});
    }
  }

  return true;
}

bool BTFHeaderGenerator::createTypeQueue(Context &context) {
  context.type_queue.clear();
  context.visited_type_list = {0};

  for (const auto &id : context.top_level_type_list) {
    if (!createTypeQueueHelper(context, id)) {
      return false;
    }
  }

  return true;
}

bool BTFHeaderGenerator::createTypeQueueHelper(Context &context,
                                               const std::uint32_t &id) {
  if (id == 0) {
    return true;
  }

  if (context.visited_type_list.count(id) > 0) {
    return true;
  }

  context.visited_type_list.insert(id);

  auto link_list_it = context.type_tree.find(id);
  if (link_list_it != context.type_tree.end()) {
    auto &link_list = link_list_it->second;

    for (const auto &p : link_list) {
      auto linked_type = p.first;
      const auto &weak_reference = p.second;

      if (weak_reference) {
        const auto &btf_type = context.btf_type_map.at(linked_type);

        auto btf_kind = btfparse::IBTF::getBTFTypeKind(btf_type);
        bool is_union;

        if (btf_kind == btfparse::BTFKind::Union) {
          is_union = true;

        } else if (btf_kind == btfparse::BTFKind::Struct) {
          is_union = false;

        } else {
          return false;
        }

        auto opt_type_name = getTypeName(context, linked_type);
        auto fwd_type_id =
            getOrCreateFwdType(context, is_union, opt_type_name.value());

        if (!createTypeQueueHelper(context, fwd_type_id)) {
          return false;
        }

        linked_type = fwd_type_id;
      }

      if (!createTypeQueueHelper(context, linked_type)) {
        return false;
      }
    }
  }

  context.type_queue.push_back(id);
  return true;
}

bool BTFHeaderGenerator::isTopLevelTypeDeclaration(const Context &context,
                                                   std::uint32_t id) {
  return context.top_level_type_list.count(id) > 0;
}

void BTFHeaderGenerator::setVariableName(Context &context,
                                         const std::string &name) {
  context.opt_variable_name = name;
}

std::optional<std::string>
BTFHeaderGenerator::takeVariableName(Context &context) {
  auto opt_variable_name = std::move(context.opt_variable_name);
  context.opt_variable_name = std::nullopt;

  return opt_variable_name;
}

void BTFHeaderGenerator::pushVariableName(Context &context) {
  context.variable_name_stack.push_back(std::move(context.opt_variable_name));
  context.opt_variable_name = std::nullopt;
}

void BTFHeaderGenerator::popVariableName(Context &context) {
  if (context.variable_name_stack.empty()) {
    context.opt_variable_name = std::nullopt;
  } else {
    context.opt_variable_name = std::move(context.variable_name_stack.back());
    context.variable_name_stack.pop_back();
  }
}

void BTFHeaderGenerator::setTypedefName(Context &context,
                                        const std::string &name) {
  context.opt_typedef_name = name;
}

std::optional<std::string>
BTFHeaderGenerator::takeTypedefName(Context &context) {
  auto opt_typedef_name = std::move(context.opt_typedef_name);
  context.opt_typedef_name = std::nullopt;

  return opt_typedef_name;
}

void BTFHeaderGenerator::pushTypedefName(Context &context) {
  context.typedef_name_stack.push_back(std::move(context.opt_typedef_name));
  context.opt_typedef_name = std::nullopt;
}

void BTFHeaderGenerator::popTypedefName(Context &context) {
  if (context.typedef_name_stack.empty()) {
    context.opt_typedef_name = std::nullopt;
  } else {
    context.opt_typedef_name = std::move(context.typedef_name_stack.back());
    context.typedef_name_stack.pop_back();
  }
}

void BTFHeaderGenerator::pushState(Context &context) {
  pushVariableName(context);
  pushModifierList(context);
  pushTypedefName(context);
}

void BTFHeaderGenerator::popState(Context &context) {
  popVariableName(context);
  popModifierList(context);
  popTypedefName(context);
}

void BTFHeaderGenerator::resetState(Context &context) {
  context.modifier_list_stack.clear();
  context.modifier_list.clear();

  context.typedef_name_stack.clear();
  context.opt_typedef_name = std::nullopt;

  context.variable_name_stack.clear();
  context.opt_variable_name = std::nullopt;
}

std::uint32_t BTFHeaderGenerator::getOrCreateFwdType(Context &context,
                                                     bool is_union,
                                                     const std::string &name) {

  auto fwd_type_id_it = context.fwd_type_map.find(name);
  if (fwd_type_id_it == context.fwd_type_map.end()) {
    btfparse::FwdBTFType fwd_btf_type;
    fwd_btf_type.is_union = is_union;
    fwd_btf_type.name = name;

    auto id = generateTypeID(context);
    context.btf_type_map.insert({id, std::move(fwd_btf_type)});

    auto insert_status = context.fwd_type_map.insert({name, id});
    fwd_type_id_it = insert_status.first;
  }

  return fwd_type_id_it->second;
}

template <typename Type>
bool generateStructOrUnion(BTFHeaderGenerator::Context &context,
                           std::stringstream &buffer, std::uint32_t id,
                           const Type &btf_type, bool as_type_definition) {

  static_assert(std::is_same<Type, StructBTFType>::value ||
                    std::is_same<Type, UnionBTFType>::value,
                "Invalid type passed to generateStructOrUnion");

  BTFHeaderGenerator::generateTypeHeader(context, buffer, id);
  BTFHeaderGenerator::generateIndent(context, buffer);

  if (!BTFHeaderGenerator::generateLeftModifiers(context, buffer)) {
    return false;
  }

  buffer << (std::is_same<Type, StructBTFType>::value ? "struct" : "union");

  std::optional<std::string> opt_name;
  if (btf_type.opt_name.has_value() && !btf_type.opt_name.value().empty()) {
    opt_name = btf_type.opt_name;
  }

  if (opt_name.has_value()) {
    buffer << " " << opt_name.value();
  }

  auto emit_body = as_type_definition ||
                   (!as_type_definition && !btf_type.opt_name.has_value());

  if (emit_body) {
    BTFHeaderGenerator::pushState(context);

    buffer << " {\n";

    BTFHeaderGenerator::increaseIndent(context);

    for (const auto &member : btf_type.member_list) {
      if (member.opt_name.has_value()) {
        BTFHeaderGenerator::setVariableName(context, member.opt_name.value());
      }

      if (!BTFHeaderGenerator::generateType(context, buffer, member.type,
                                            false)) {
        return false;
      }

      std::optional<std::uint8_t> opt_bitfield_size{std::nullopt};
      if (member.opt_bitfield_size.has_value() &&
          member.opt_bitfield_size.value() != 0) {
        opt_bitfield_size = member.opt_bitfield_size.value();
      }

      if (opt_bitfield_size.has_value()) {
        buffer << " : " << static_cast<int>(opt_bitfield_size.value());
      }

      buffer << ";\n";
    }

    BTFHeaderGenerator::decreaseIndent(context);
    BTFHeaderGenerator::generateIndent(context, buffer);

    buffer << "}";

    BTFHeaderGenerator::popState(context);
  }

  if (!BTFHeaderGenerator::generateMiddleModifiers(context, buffer)) {
    return false;
  }

  opt_name = BTFHeaderGenerator::takeVariableName(context);
  if (!opt_name.has_value()) {
    opt_name = BTFHeaderGenerator::takeTypedefName(context);
  }

  if (opt_name.has_value()) {
    buffer << " " << opt_name.value();
  }

  if (!BTFHeaderGenerator::generateRightModifiers(context, buffer)) {
    return false;
  }

  return true;
}

bool BTFHeaderGenerator::generateType(Context &context,
                                      std::stringstream &buffer,
                                      std::uint32_t id,
                                      const StructBTFType &struct_btf_type,
                                      bool as_type_definition) {
  return generateStructOrUnion(context, buffer, id, struct_btf_type,
                               as_type_definition);
}

bool BTFHeaderGenerator::generateType(Context &context,
                                      std::stringstream &buffer,
                                      std::uint32_t id,
                                      const UnionBTFType &union_btf_type,
                                      bool as_type_definition) {
  return generateStructOrUnion(context, buffer, id, union_btf_type,
                               as_type_definition);
}

bool BTFHeaderGenerator::generateType(Context &context,
                                      std::stringstream &buffer,
                                      std::uint32_t id,
                                      const EnumBTFType &enum_btf_type,
                                      bool as_type_definition) {

  generateTypeHeader(context, buffer, id);
  generateIndent(context, buffer);

  if (!generateLeftModifiers(context, buffer)) {
    return false;
  }

  buffer << "enum";

  if (enum_btf_type.opt_name.has_value()) {
    buffer << " " << enum_btf_type.opt_name.value();
  }

  auto emit_body = (as_type_definition && !enum_btf_type.value_list.empty()) ||
                   (!as_type_definition && !enum_btf_type.opt_name.has_value());

  if (emit_body) {
    buffer << " {\n";

    increaseIndent(context);

    for (auto value_it = enum_btf_type.value_list.begin();
         value_it != enum_btf_type.value_list.end(); ++value_it) {

      const auto &value = *value_it;

      generateIndent(context, buffer);

      buffer << value.name << " = " << value.val;
      if (std::next(value_it, 1) != enum_btf_type.value_list.end()) {
        buffer << ",";
      }

      buffer << "\n";
    }

    decreaseIndent(context);
    generateIndent(context, buffer);

    buffer << "}";
  }

  if (!generateMiddleModifiers(context, buffer)) {
    return false;
  }

  auto opt_name = BTFHeaderGenerator::takeVariableName(context);
  if (!opt_name.has_value()) {
    opt_name = BTFHeaderGenerator::takeTypedefName(context);
  }

  if (opt_name.has_value()) {
    buffer << " " << opt_name.value();
  }

  if (!generateRightModifiers(context, buffer)) {
    return false;
  }

  return true;
}

bool BTFHeaderGenerator::generateType(Context &context,
                                      std::stringstream &buffer,
                                      std::uint32_t id,
                                      const TypedefBTFType &typedef_btf_type,
                                      bool as_type_definition) {

  if (as_type_definition) {
    generateTypeHeader(context, buffer, id);

    buffer << "typedef\n";
    increaseIndent(context);

    setTypedefName(context, typedef_btf_type.name);
    if (!generateType(context, buffer, typedef_btf_type.type, false)) {
      return false;
    }

    auto opt_name = BTFHeaderGenerator::takeTypedefName(context);
    if (opt_name.has_value()) {
      buffer << " " << opt_name.value();
    }

    decreaseIndent(context);

  } else {
    generateTypeHeader(context, buffer, id);
    generateIndent(context, buffer);

    if (!generateLeftModifiers(context, buffer)) {
      return false;
    }

    buffer << typedef_btf_type.name;

    if (!generateMiddleModifiers(context, buffer)) {
      return false;
    }

    auto opt_name = BTFHeaderGenerator::takeVariableName(context);
    if (!opt_name.has_value()) {
      opt_name = BTFHeaderGenerator::takeTypedefName(context);
    }

    if (opt_name.has_value()) {
      buffer << " " << opt_name.value();
    }

    if (!generateRightModifiers(context, buffer)) {
      return false;
    }
  }

  return true;
}

bool BTFHeaderGenerator::generateType(Context &context,
                                      std::stringstream &buffer,
                                      std::uint32_t id,
                                      const IntBTFType &int_btf_type, bool) {

  generateTypeHeader(context, buffer, id);
  generateIndent(context, buffer);

  if (!generateLeftModifiers(context, buffer)) {
    return false;
  }

  buffer << int_btf_type.name;

  if (!generateMiddleModifiers(context, buffer)) {
    return false;
  }

  auto opt_name = BTFHeaderGenerator::takeVariableName(context);
  if (!opt_name.has_value()) {
    opt_name = BTFHeaderGenerator::takeTypedefName(context);
  }

  if (opt_name.has_value()) {
    buffer << " " << opt_name.value();
  }

  if (!generateRightModifiers(context, buffer)) {
    return false;
  }

  return true;
}

bool BTFHeaderGenerator::generateType(
    Context &context, std::stringstream &buffer, std::uint32_t id,
    const FuncProtoBTFType &func_proto_btf_type, bool as_type_definition) {

  filterFuncProtoModifiers(context);
  generateTypeHeader(context, buffer, id);
  increaseIndent(context);

  pushState(context);

  if (!generateType(context, buffer, func_proto_btf_type.return_type, false)) {
    return false;
  }

  popState(context);

  increaseIndent(context);
  generateIndent(context, buffer);

  buffer << "\n";

  generateIndent(context, buffer);

  buffer << "(";

  if (!generateLeftModifiers(context, buffer)) {
    return false;
  }

  if (!generateMiddleModifiers(context, buffer)) {
    return false;
  }

  auto opt_name = BTFHeaderGenerator::takeVariableName(context);
  if (!opt_name.has_value()) {
    opt_name = BTFHeaderGenerator::takeTypedefName(context);
  }

  if (opt_name.has_value()) {
    buffer << " " << opt_name.value();
  }

  if (!generateRightModifiers(context, buffer)) {
    return false;
  }

  buffer << ")(\n";

  increaseIndent(context);

  pushState(context);

  for (auto param_it = func_proto_btf_type.param_list.begin();
       param_it != func_proto_btf_type.param_list.end(); ++param_it) {

    const auto &param = *param_it;

    if (!generateType(context, buffer, param.type, false)) {
      return false;
    }

    auto is_last_param =
        std::next(param_it, 1) == func_proto_btf_type.param_list.end();

    if (!is_last_param || (is_last_param && func_proto_btf_type.is_variadic)) {
      buffer << ",";
    }

    buffer << "\n";
  }

  popState(context);

  if (func_proto_btf_type.is_variadic) {
    generateIndent(context, buffer);
    buffer << "...\n";
  }

  decreaseIndent(context);

  generateIndent(context, buffer);
  buffer << ")";

  decreaseIndent(context);
  decreaseIndent(context);

  return true;
}

bool BTFHeaderGenerator::generateType(Context &context,
                                      std::stringstream &buffer,
                                      std::uint32_t id,
                                      const FloatBTFType &float_btf_type,
                                      bool) {

  generateTypeHeader(context, buffer, id);
  generateIndent(context, buffer);

  if (!generateLeftModifiers(context, buffer)) {
    return false;
  }

  buffer << float_btf_type.name;

  if (!generateMiddleModifiers(context, buffer)) {
    return false;
  }

  auto opt_name = BTFHeaderGenerator::takeVariableName(context);
  if (!opt_name.has_value()) {
    opt_name = BTFHeaderGenerator::takeTypedefName(context);
  }

  if (opt_name.has_value()) {
    buffer << " " << opt_name.value();
  }

  if (!generateRightModifiers(context, buffer)) {
    return false;
  }

  return true;
}

bool BTFHeaderGenerator::generateType(Context &context,
                                      std::stringstream &buffer,
                                      std::uint32_t id,
                                      const PtrBTFType &ptr_btf_type,
                                      bool as_type_definition) {

  pushModifier(context, id);
  return generateType(context, buffer, ptr_btf_type.type, as_type_definition);
}

bool BTFHeaderGenerator::generateType(Context &context,
                                      std::stringstream &buffer,
                                      std::uint32_t id,
                                      const ArrayBTFType &array_btf_type,
                                      bool as_type_definition) {

  pushModifier(context, id);
  return generateType(context, buffer, array_btf_type.type, as_type_definition);
}

bool BTFHeaderGenerator::generateType(Context &context,
                                      std::stringstream &buffer,
                                      std::uint32_t id,
                                      const VolatileBTFType &volatile_btf_type,
                                      bool as_type_definition) {

  pushModifier(context, id);
  return generateType(context, buffer, volatile_btf_type.type,
                      as_type_definition);
}

bool BTFHeaderGenerator::generateType(Context &context,
                                      std::stringstream &buffer,
                                      std::uint32_t id,
                                      const ConstBTFType &const_btf_type,
                                      bool as_type_definition) {

  pushModifier(context, id);
  return generateType(context, buffer, const_btf_type.type, as_type_definition);
}

bool BTFHeaderGenerator::generateType(Context &context,
                                      std::stringstream &buffer,
                                      std::uint32_t id,
                                      const RestrictBTFType &restrict_btf_type,
                                      bool as_type_definition) {

  pushModifier(context, id);
  return generateType(context, buffer, restrict_btf_type.type,
                      as_type_definition);
}

bool BTFHeaderGenerator::generateType(Context &context,
                                      std::stringstream &buffer,
                                      std::uint32_t id,
                                      const FwdBTFType &fwd_btf_type, bool) {

  BTFHeaderGenerator::generateTypeHeader(context, buffer, id);
  BTFHeaderGenerator::generateIndent(context, buffer);

  if (!BTFHeaderGenerator::generateLeftModifiers(context, buffer)) {
    return false;
  }

  buffer << (fwd_btf_type.is_union ? "union" : "struct");

  buffer << " " << fwd_btf_type.name;

  if (!BTFHeaderGenerator::generateMiddleModifiers(context, buffer)) {
    return false;
  }

  auto opt_name = BTFHeaderGenerator::takeVariableName(context);
  if (!opt_name.has_value()) {
    opt_name = BTFHeaderGenerator::takeTypedefName(context);
  }

  if (opt_name.has_value()) {
    buffer << " " << opt_name.value();
  }

  if (!BTFHeaderGenerator::generateRightModifiers(context, buffer)) {
    return false;
  }

  return true;
}

bool BTFHeaderGenerator::generateType(Context &context,
                                      std::stringstream &buffer,
                                      std::uint32_t id,
                                      bool as_type_definition) {

  if (id == 0) {
    return generateVoidType(context, buffer);
  }

  const auto &btf_type = context.btf_type_map.at(id);

  switch (btfparse::IBTF::getBTFTypeKind(btf_type)) {
  case btfparse::BTFKind::Struct: {
    const auto &struct_btf_type = getTypeAs<StructBTFType>(btf_type);
    return generateType(context, buffer, id, struct_btf_type,
                        as_type_definition);
  }

  case btfparse::BTFKind::Union: {
    const auto &union_btf_type = getTypeAs<UnionBTFType>(btf_type);
    return generateType(context, buffer, id, union_btf_type,
                        as_type_definition);
  }

  case btfparse::BTFKind::Enum: {
    const auto &enum_btf_type = getTypeAs<EnumBTFType>(btf_type);
    return generateType(context, buffer, id, enum_btf_type, as_type_definition);
  }

  case btfparse::BTFKind::Typedef: {
    const auto &typedef_btf_type = getTypeAs<TypedefBTFType>(btf_type);
    return generateType(context, buffer, id, typedef_btf_type,
                        as_type_definition);
  }

  case btfparse::BTFKind::Int: {
    const auto &int_btf_type = getTypeAs<IntBTFType>(btf_type);
    return generateType(context, buffer, id, int_btf_type, as_type_definition);
  }

  case btfparse::BTFKind::FuncProto: {
    const auto &func_proto_btf_type = getTypeAs<FuncProtoBTFType>(btf_type);
    return generateType(context, buffer, id, func_proto_btf_type,
                        as_type_definition);
  }

  case btfparse::BTFKind::Float: {
    const auto &float_btf_type = getTypeAs<FloatBTFType>(btf_type);
    return generateType(context, buffer, id, float_btf_type,
                        as_type_definition);
  }

  case btfparse::BTFKind::Ptr: {
    const auto &ptr_btf_type = getTypeAs<PtrBTFType>(btf_type);
    return generateType(context, buffer, id, ptr_btf_type, as_type_definition);
  }

  case btfparse::BTFKind::Array: {
    const auto &array_btf_type = getTypeAs<ArrayBTFType>(btf_type);
    return generateType(context, buffer, id, array_btf_type,
                        as_type_definition);
  }

  case btfparse::BTFKind::Volatile: {
    const auto &volatile_btf_type = getTypeAs<VolatileBTFType>(btf_type);
    return generateType(context, buffer, id, volatile_btf_type,
                        as_type_definition);
  }

  case btfparse::BTFKind::Const: {
    const auto &const_btf_type = getTypeAs<ConstBTFType>(btf_type);
    return generateType(context, buffer, id, const_btf_type,
                        as_type_definition);
  }

  case btfparse::BTFKind::Restrict: {
    const auto &restrict_btf_type = getTypeAs<RestrictBTFType>(btf_type);
    return generateType(context, buffer, id, restrict_btf_type,
                        as_type_definition);
  }

  case btfparse::BTFKind::Fwd: {
    const auto &fwd_btf_type = getTypeAs<FwdBTFType>(btf_type);
    return generateType(context, buffer, id, fwd_btf_type, as_type_definition);
  }

  case btfparse::BTFKind::Func:
  case btfparse::BTFKind::Var:
  case btfparse::BTFKind::DataSec:
  case btfparse::BTFKind::Void:
    return true;
  }

  return true;
}

bool BTFHeaderGenerator::generateVoidType(Context &context,
                                          std::stringstream &buffer) {

  generateTypeHeader(context, buffer, 0);
  generateIndent(context, buffer);

  if (!generateLeftModifiers(context, buffer)) {
    return false;
  }

  buffer << "void";

  if (!generateMiddleModifiers(context, buffer)) {
    return false;
  }

  auto opt_name = BTFHeaderGenerator::takeVariableName(context);
  if (!opt_name.has_value()) {
    opt_name = BTFHeaderGenerator::takeTypedefName(context);
  }

  if (opt_name.has_value()) {
    buffer << " " << opt_name.value();
  }

  if (!generateRightModifiers(context, buffer)) {
    return false;
  }

  return true;
}

bool BTFHeaderGenerator::generateTypeHeader(const Context &context,
                                            std::stringstream &buffer,
                                            std::uint32_t id) {

  generateIndent(context, buffer);

  buffer << "/* "
         << "BTF Type #" << id << " */\n";

  return true;
}

void BTFHeaderGenerator::pushModifierList(Context &context) {
  context.modifier_list_stack.push_back(std::move(context.modifier_list));
  context.modifier_list.clear();
}

void BTFHeaderGenerator::popModifierList(Context &context) {
  if (context.modifier_list_stack.empty()) {
    context.modifier_list.clear();

  } else {
    context.modifier_list = std::move(context.modifier_list_stack.back());
    context.modifier_list_stack.pop_back();
  }
}

void BTFHeaderGenerator::pushModifier(Context &context, std::uint32_t id) {
  context.modifier_list.push_back(id);
}

void BTFHeaderGenerator::filterFuncProtoModifiers(Context &context) {
  for (auto modifier_it = context.modifier_list.begin();
       modifier_it != context.modifier_list.end();) {

    const auto &modifier = *modifier_it;

    if (context.btf_type_map.count(modifier) == 0) {
      continue;
    }

    const auto &btf_type = context.btf_type_map.at(modifier);

    auto btf_kind = IBTF::getBTFTypeKind(btf_type);
    if (btf_kind == BTFKind::Volatile) {
      modifier_it = context.modifier_list.erase(modifier_it);
    } else {
      ++modifier_it;
    }
  }
}

bool BTFHeaderGenerator::generateLeftModifiers(Context &context,
                                               std::stringstream &buffer) {

  std::vector<const char *> string_list;

  for (auto modifier_it = context.modifier_list.rbegin();
       modifier_it != context.modifier_list.rend(); ++modifier_it) {

    const auto &id = *modifier_it;
    const auto &btf_type = context.btf_type_map.at(id);
    auto btf_type_kind = IBTF::getBTFTypeKind(btf_type);

    if (btf_type_kind == BTFKind::Volatile) {
      string_list.push_back("volatile");

    } else if (btf_type_kind == BTFKind::Const) {
      string_list.push_back("const");

    } else if (btf_type_kind == BTFKind::Restrict) {
      string_list.push_back("restrict");

    } else {
      break;
    }
  }

  auto count = static_cast<int>(string_list.size());
  auto start_it = std::prev(context.modifier_list.end(), count);
  context.modifier_list.erase(start_it, context.modifier_list.end());

  if (!string_list.empty()) {
    buffer << " ";
  }

  for (auto string_it = string_list.begin(); string_it != string_list.end();
       ++string_it) {

    const auto &string = *string_it;
    buffer << string << " ";
  }

  return true;
}

bool BTFHeaderGenerator::generateMiddleModifiers(Context &context,
                                                 std::stringstream &buffer) {
  std::vector<const char *> string_list;

  for (auto modifier_it = context.modifier_list.rbegin();
       modifier_it != context.modifier_list.rend(); ++modifier_it) {

    const auto &id = *modifier_it;
    const auto &btf_type = context.btf_type_map.at(id);
    auto btf_type_kind = IBTF::getBTFTypeKind(btf_type);

    if (btf_type_kind == BTFKind::Const) {
      string_list.push_back("const");

    } else if (btf_type_kind == BTFKind::Ptr) {
      string_list.push_back("*");

    } else {
      break;
    }
  }

  auto count = static_cast<int>(string_list.size());
  auto start_it = std::prev(context.modifier_list.end(), count);
  context.modifier_list.erase(start_it, context.modifier_list.end());

  if (!string_list.empty()) {
    buffer << " ";
  }

  for (auto string_it = string_list.begin(); string_it != string_list.end();
       ++string_it) {

    const auto &string = *string_it;
    buffer << string;

    if (std::next(string_it, 1) != string_list.end()) {
      buffer << " ";
    }
  }

  return true;
}

bool BTFHeaderGenerator::generateRightModifiers(Context &context,
                                                std::stringstream &buffer) {

  std::size_t consumed_modifier_count{0};
  bool is_array{false};

  for (auto modifier_it = context.modifier_list.rbegin();
       modifier_it != context.modifier_list.rend(); ++modifier_it) {

    const auto &id = *modifier_it;
    const auto &btf_type = context.btf_type_map.at(id);
    auto btf_type_kind = IBTF::getBTFTypeKind(btf_type);

    if (btf_type_kind == BTFKind::Array) {
      is_array = true;

      const auto &array_btf_type = getTypeAs<ArrayBTFType>(btf_type);
      buffer << "[" << array_btf_type.nelems << "]";

      ++consumed_modifier_count;

    } else {
      // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=8354
      // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=102195
      if (is_array && (btf_type_kind == BTFKind::Const ||
                       btf_type_kind == BTFKind::Volatile)) {
        ++consumed_modifier_count;
        continue;
      }

      is_array = false;
      break;
    }
  }

  auto count = static_cast<int>(consumed_modifier_count);
  auto start_it = std::prev(context.modifier_list.end(), count);
  context.modifier_list.erase(start_it, context.modifier_list.end());

  if (!context.modifier_list.empty()) {
    buffer << " /* Unused modifiers: ";

    for (auto modifier_it = context.modifier_list.begin();
         modifier_it != context.modifier_list.end(); ++modifier_it) {
      buffer << static_cast<int>(*modifier_it);

      if (std::next(modifier_it, 1) != context.modifier_list.end()) {
        buffer << ", ";
      }
    }

    buffer << " */ ";

    context.modifier_list.clear();
    return true;
  }

  return true;
}

bool BTFHeaderGenerator::generateHeader(Context &context,
                                        std::stringstream &buffer) {

  buffer << "#pragma pack(push, 1)\n";

  for (const auto &id : context.type_queue) {
    resetState(context);

    {
      auto opt_name = getTypeName(context, id);

      const auto &name = opt_name.value();
      if (name.find("__builtin_") == 0) {
        continue;
      }
    }

    if (!generateType(context, buffer, id, true)) {
      return false;
    }

    buffer << ";\n\n";
  }

  buffer << "#pragma pack(pop)\n";

  return true;
}

} // namespace btfparse
