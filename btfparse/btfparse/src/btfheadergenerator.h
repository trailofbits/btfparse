//
// Copyright (c) 2021-present, Trail of Bits, Inc.
// All rights reserved.
//
// This source code is licensed in accordance with the terms specified in
// the LICENSE file found in the root directory of this source tree.
//

#pragma once

#include <sstream>
#include <unordered_map>
#include <unordered_set>

#include <btfparse/ibtfheadergenerator.h>

namespace btfparse {

class BTFHeaderGenerator final : public IBTFHeaderGenerator {
public:
  virtual ~BTFHeaderGenerator() override;

  virtual bool generate(std::string &header,
                        const IBTF::Ptr &btf) const override;

private:
  BTFHeaderGenerator();

public:
  struct Context final {
    BTFTypeMap btf_type_map;
    std::unordered_set<std::uint32_t> top_level_type_list;
    std::unordered_map<std::string, std::uint32_t> fwd_type_map;

    std::uint32_t padding_byte_id{0};

    std::uint32_t highest_btf_type_id{0};
    std::uint32_t btf_type_id_generator{0};

    std::vector<std::uint32_t> type_queue;

    std::unordered_set<std::uint32_t> visited_type_list;

    std::unordered_map<std::uint32_t, std::unordered_map<std::uint32_t, bool>>
        type_tree;

    std::unordered_map<std::uint32_t, std::unordered_set<std::uint32_t>>
        inverse_type_tree;

    std::vector<std::vector<std::uint32_t>> modifier_list_stack;
    std::vector<std::uint32_t> modifier_list;

    std::unordered_set<std::uint32_t> generated_type_list;

    std::vector<std::optional<std::string>> typedef_name_stack;
    std::optional<std::string> opt_typedef_name;

    std::vector<std::optional<std::string>> variable_name_stack;
    std::optional<std::string> opt_variable_name;

    std::size_t indent_level{0};
  };

  static bool saveBTFTypeMap(Context &context, const IBTF::Ptr &btf);
  static bool adjustTypeNames(Context &context);

  static std::uint32_t generateTypeID(Context &context);
  static bool materializePadding(Context &context);
  static bool materializeStructPadding(Context &context, std::uint32_t id,
                                       StructBTFType &struct_btf_type);

  static bool isBitfield(const StructBTFType::Member &member);

  static std::optional<std::uint32_t> getBTFTypeSize(const Context &context,
                                                     const BTFType &type);

  static std::optional<std::uint32_t> getBTFTypeSize(const Context &context,
                                                     std::uint32_t type);

  static bool isValidTypeId(const Context &context, std::uint32_t id);
  static bool isRenameableType(const Context &context, std::uint32_t id);
  static void scanTypes(Context &context);

  static bool getTypeDependencies(const Context &context,
                                  std::vector<std::uint32_t> &dependency_list,
                                  std::uint32_t id);

  static std::optional<std::string> getTypeName(const Context &context,
                                                std::uint32_t id);

  static bool setTypeName(Context &context, std::uint32_t id,
                          const std::string &name);

  static void resetIndent(Context &context);
  static void increaseIndent(Context &context);
  static void decreaseIndent(Context &context);
  static void generateIndent(const Context &context, std::stringstream &buffer);

  static bool createTypeTree(Context &context);
  static bool createTypeTreeHelper(BTFHeaderGenerator::Context &context,
                                   bool inside_pointer,
                                   const std::uint32_t &parent,
                                   const std::uint32_t &id);

  static bool adjustTypedefDependencyLoops(Context &context);

  static bool createTypeQueue(Context &context);
  static bool createTypeQueueHelper(Context &context, const std::uint32_t &id);

  static bool isTopLevelTypeDeclaration(const Context &context,
                                        std::uint32_t id);

  static void setVariableName(Context &context, const std::string &name);
  static std::optional<std::string> takeVariableName(Context &context);
  static void pushVariableName(Context &context);
  static void popVariableName(Context &context);

  static void setTypedefName(Context &context, const std::string &name);
  static std::optional<std::string> takeTypedefName(Context &context);
  static void pushTypedefName(Context &context);
  static void popTypedefName(Context &context);

  static void pushState(Context &context);
  static void popState(Context &context);
  static void resetState(Context &context);

  static std::uint32_t getOrCreateFwdType(Context &context, bool is_union,
                                          const std::string &name);

  static bool generateType(Context &context, std::stringstream &buffer,
                           std::uint32_t id,
                           const StructBTFType &struct_btf_type, bool as_type);

  static bool generateType(Context &context, std::stringstream &buffer,
                           std::uint32_t id, const UnionBTFType &union_btf_type,
                           bool as_type);

  static bool generateType(Context &context, std::stringstream &buffer,
                           std::uint32_t id, const EnumBTFType &enum_btf_type,
                           bool as_type);

  static bool generateType(Context &context, std::stringstream &buffer,
                           std::uint32_t id,
                           const TypedefBTFType &typedef_btf_type,
                           bool as_type);

  static bool generateType(Context &context, std::stringstream &buffer,
                           std::uint32_t id, const IntBTFType &int_btf_type,
                           bool as_type);

  static bool generateType(Context &context, std::stringstream &buffer,
                           std::uint32_t id,
                           const FuncProtoBTFType &func_proto_btf_type,
                           bool as_type);

  static bool generateType(Context &context, std::stringstream &buffer,
                           std::uint32_t id, const FloatBTFType &float_btf_type,
                           bool as_type);

  static bool generateType(Context &context, std::stringstream &buffer,
                           std::uint32_t id, const PtrBTFType &ptr_btf_type,
                           bool as_type);

  static bool generateType(Context &context, std::stringstream &buffer,
                           std::uint32_t id, const ArrayBTFType &array_btf_type,
                           bool as_type);

  static bool generateType(Context &context, std::stringstream &buffer,
                           std::uint32_t id,
                           const VolatileBTFType &volatile_btf_type,
                           bool as_type);

  static bool generateType(Context &context, std::stringstream &buffer,
                           std::uint32_t id, const ConstBTFType &const_btf_type,
                           bool as_type);

  static bool generateType(Context &context, std::stringstream &buffer,
                           std::uint32_t id,
                           const RestrictBTFType &restrict_btf_type,
                           bool as_type);

  static bool generateType(Context &context, std::stringstream &buffer,
                           std::uint32_t id, const FwdBTFType &fwd_btf_type,
                           bool as_type);

  static bool generateType(Context &context, std::stringstream &buffer,
                           std::uint32_t id, bool as_type);

  static bool generateVoidType(Context &context, std::stringstream &buffer);

  static bool generateTypeHeader(const Context &context,
                                 std::stringstream &buffer, std::uint32_t id);

  static void pushModifierList(Context &context);
  static void popModifierList(Context &context);
  static void pushModifier(Context &context, std::uint32_t id);

  static void filterFuncProtoModifiers(Context &context);

  static bool generateLeftModifiers(Context &context,
                                    std::stringstream &buffer);

  static bool generateMiddleModifiers(Context &context,
                                      std::stringstream &buffer);

  static bool generateRightModifiers(Context &context,
                                     std::stringstream &buffer);

  static bool generateHeader(Context &context, std::stringstream &buffer);

  friend class IBTFHeaderGenerator;
};

} // namespace btfparse
