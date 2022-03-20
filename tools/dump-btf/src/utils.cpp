//
// Copyright (c) 2021-present, Trail of Bits, Inc.
// All rights reserved.
//
// This source code is licensed in accordance with the terms specified in
// the LICENSE file found in the root directory of this source tree.
//

#include "utils.h"

#include <sstream>

namespace {

template <typename Type>
void printStructOrUnionBTFType(std::ostream &stream, const Type &type) {
  static_assert(std::is_same<Type, btfparse::StructBTFType>::value ||
                    std::is_same<Type, btfparse::UnionBTFType>::value,
                "Type must be either StructBTFType or UnionBTFType");

  stream << "'"
         << (type.opt_name.has_value() ? type.opt_name.value() : "(anon)")
         << "' "
         << "size=" << type.size << " "
         << "vlen=" << type.member_list.size();

  if (!type.member_list.empty()) {
    stream << "\n";
  }

  for (auto it = type.member_list.begin(); it != type.member_list.end(); ++it) {
    const auto &member = *it;

    stream << "\t"
           << "'"
           << (member.opt_name.has_value() ? member.opt_name.value() : "(anon)")
           << "' "
           << "type_id=" << member.type << " "
           << "bits_offset=" << member.offset;

    if (member.opt_bitfield_size.has_value()) {
      auto bitfield_size =
          static_cast<std::uint32_t>(member.opt_bitfield_size.value());

      if (bitfield_size != 0) {
        stream << " "
               << "bitfield_size=" << bitfield_size;
      }
    }

    if (std::next(it, 1) != type.member_list.end()) {
      stream << "\n";
    }
  }
}

std::ostream &operator<<(std::ostream &stream,
                         const btfparse::IntBTFType::Encoding &encoding) {
  switch (encoding) {
  case btfparse::IntBTFType::Encoding::None:
    stream << "(none)";
    break;

  case btfparse::IntBTFType::Encoding::Signed:
    stream << "SIGNED";
    break;

  case btfparse::IntBTFType::Encoding::Char:
    stream << "CHAR";
    break;

  case btfparse::IntBTFType::Encoding::Bool:
    stream << "BOOL";
    break;

  default:
    stream << "UNKNOWN";
    break;
  }

  return stream;
}

std::ostream &operator<<(std::ostream &stream,
                         const btfparse::IntBTFType &type) {
  stream << "'" << type.name << "' "
         << "size=" << type.size << " "
         << "bits_offset=" << static_cast<int>(type.offset) << " "
         << "nr_bits=" << static_cast<int>(type.bits) << " "
         << "encoding=" << type.encoding;

  return stream;
}

std::ostream &operator<<(std::ostream &stream,
                         const btfparse::PtrBTFType &type) {
  stream << "'(anon)' "
         << "type_id=" << type.type;

  return stream;
}

std::ostream &operator<<(std::ostream &stream,
                         const btfparse::ConstBTFType &type) {
  stream << "'(anon)' "
         << "type_id=" << type.type;

  return stream;
}

std::ostream &operator<<(std::ostream &stream,
                         const btfparse::ArrayBTFType &type) {
  stream << "'(anon)' "
         << "type_id=" << type.type << " "
         << "index_type_id=" << type.index_type << " "
         << "nr_elems=" << type.nelems;

  return stream;
}

std::ostream &operator<<(std::ostream &stream,
                         const btfparse::TypedefBTFType &type) {
  stream << "'" << type.name << "' "
         << "type_id=" << type.type;

  return stream;
}

std::ostream &operator<<(std::ostream &stream,
                         const btfparse::EnumBTFType &type) {
  stream << "'"
         << (type.opt_name.has_value() ? type.opt_name.value() : "(anon)")
         << "' "
         << "size=" << type.size << " "
         << "vlen=" << type.value_list.size();

  if (!type.value_list.empty()) {
    stream << "\n";
  }

  for (auto it = type.value_list.begin(); it != type.value_list.end(); ++it) {
    const auto &value = *it;

    // Even though `val` is marked as signed in the "BTF Type Format"
    // documentation, the `bpftool` prints it as unsigned
    stream << "\t"
           << "'" << value.name << "' "
           << "val=" << static_cast<std::uint32_t>(value.val);

    if (std::next(it, 1) != type.value_list.end()) {
      stream << "\n";
    }
  }

  return stream;
}

// When the last item in the BTF format is unnamed and has type 0, then it's
// a variadic function. This library removes the last item and enables the
// `is_variadic` flag in the object.
//
// Retain this behavior when outputting data in bptftool format
std::ostream &operator<<(std::ostream &stream,
                         const btfparse::FuncProtoBTFType &type) {

  auto vlen = type.param_list.size();
  if (type.is_variadic) {
    ++vlen;
  }

  stream << "'(anon)' "
         << "ret_type_id=" << type.return_type << " "
         << "vlen=" << vlen;

  if (!type.param_list.empty()) {
    stream << "\n";
  }

  for (auto it = type.param_list.begin(); it != type.param_list.end(); ++it) {
    const auto &param = *it;

    stream << "\t"
           << "'"
           << (param.opt_name.has_value() ? param.opt_name.value() : "(anon)")
           << "' "
           << "type_id=" << param.type;

    if (std::next(it, 1) != type.param_list.end()) {
      stream << "\n";
    }
  }

  if (type.is_variadic) {
    if (!type.param_list.empty()) {
      stream << "\n";
    }

    stream << "\t'(anon)' type_id=0";
  }

  return stream;
}

std::ostream &operator<<(std::ostream &stream,
                         const btfparse::VolatileBTFType &type) {
  stream << "'(anon)' "
         << "type_id=" << type.type;

  return stream;
}

std::ostream &operator<<(std::ostream &stream,
                         const btfparse::StructBTFType &type) {
  printStructOrUnionBTFType(stream, type);
  return stream;
}

std::ostream &operator<<(std::ostream &stream,
                         const btfparse::UnionBTFType &type) {
  printStructOrUnionBTFType(stream, type);
  return stream;
}

std::ostream &operator<<(std::ostream &stream,
                         const btfparse::FwdBTFType &type) {
  stream << "'" << type.name << "' "
         << "fwd_kind=" << (type.is_union ? "union" : "struct");

  return stream;
}

std::ostream &operator<<(std::ostream &stream,
                         const btfparse::FloatBTFType &type) {
  stream << "'" << type.name << "' "
         << "size=" << type.size;

  return stream;
}

std::ostream &operator<<(std::ostream &stream,
                         const btfparse::RestrictBTFType &type) {
  stream << "'(anon)' "
         << "type_id=" << type.type;

  return stream;
}

std::ostream &operator<<(std::ostream &stream,
                         const btfparse::VarBTFType &type) {
  stream << "'" << type.name << "' "
         << "type_id=" << type.type << ", "
         << "linkage=";

  switch (type.linkage) {
  case 0:
    stream << "static";
    break;

  case 1:
    stream << "global-alloc";
    break;

  default:
    stream << type.linkage;
  }

  return stream;
}

std::ostream &operator<<(std::ostream &stream,
                         const btfparse::DataSecBTFType &type) {
  stream << "'" << type.name << "' "
         << "size=" << type.size << " "
         << "vlen=" << type.variable_list.size();

  if (!type.variable_list.empty()) {
    stream << "\n";
  }

  for (auto it = type.variable_list.begin(); it != type.variable_list.end();
       ++it) {
    const auto &variable = *it;

    stream << "\ttype_id=" << variable.type << " offset=" << variable.offset
           << " "
           << "size=" << variable.size;

    if (std::next(it, 1) != type.variable_list.end()) {
      stream << "\n";
    }
  }

  return stream;
}

std::ostream &operator<<(std::ostream &stream,
                         const btfparse::FuncBTFType::Linkage &linkage) {
  switch (linkage) {
  case btfparse::FuncBTFType::Linkage::Static:
    stream << "static";
    break;

  case btfparse::FuncBTFType::Linkage::Global:
    stream << "global";
    break;

  case btfparse::FuncBTFType::Linkage::Extern:
    stream << "extern";
    break;
  }

  return stream;
}

std::ostream &operator<<(std::ostream &stream,
                         const btfparse::FuncBTFType &type) {
  stream << "'" << type.name << "' "
         << "type_id=" << type.type << " "
         << "linkage=" << type.linkage;

  return stream;
}

} // namespace

std::ostream &operator<<(std::ostream &stream, btfparse::BTFKind kind) {
  switch (kind) {
  case btfparse::BTFKind::Void:
    stream << "VOID";
    break;

  case btfparse::BTFKind::Int:
    stream << "INT";
    break;

  case btfparse::BTFKind::Ptr:
    stream << "PTR";
    break;

  case btfparse::BTFKind::Array:
    stream << "ARRAY";
    break;

  case btfparse::BTFKind::Struct:
    stream << "STRUCT";
    break;

  case btfparse::BTFKind::Union:
    stream << "UNION";
    break;

  case btfparse::BTFKind::Enum:
    stream << "ENUM";
    break;

  case btfparse::BTFKind::Fwd:
    stream << "FWD";
    break;

  case btfparse::BTFKind::Typedef:
    stream << "TYPEDEF";
    break;

  case btfparse::BTFKind::Volatile:
    stream << "VOLATILE";
    break;

  case btfparse::BTFKind::Const:
    stream << "CONST";
    break;

  case btfparse::BTFKind::Restrict:
    stream << "RESTRICT";
    break;

  case btfparse::BTFKind::Func:
    stream << "FUNC";
    break;

  case btfparse::BTFKind::FuncProto:
    stream << "FUNC_PROTO";
    break;

  case btfparse::BTFKind::Var:
    stream << "VAR";
    break;

  case btfparse::BTFKind::DataSec:
    stream << "DATASEC";
    break;

  case btfparse::BTFKind::Float:
    stream << "FLOAT";
    break;

  default:
    stream << "UNKNOWN";
    break;
  }

  return stream;
}

std::ostream &operator<<(std::ostream &stream, const btfparse::BTFType &type) {
  switch (btfparse::IBTF::getBTFTypeKind(type)) {
  case btfparse::BTFKind::Void:
    break;

  case btfparse::BTFKind::Int:
    stream << std::get<btfparse::IntBTFType>(type);
    break;

  case btfparse::BTFKind::Ptr:
    stream << std::get<btfparse::PtrBTFType>(type);
    break;

  case btfparse::BTFKind::Array:
    stream << std::get<btfparse::ArrayBTFType>(type);
    break;

  case btfparse::BTFKind::Struct:
    stream << std::get<btfparse::StructBTFType>(type);
    break;

  case btfparse::BTFKind::Union:
    stream << std::get<btfparse::UnionBTFType>(type);
    break;

  case btfparse::BTFKind::Enum:
    stream << std::get<btfparse::EnumBTFType>(type);
    break;

  case btfparse::BTFKind::Fwd:
    stream << std::get<btfparse::FwdBTFType>(type);
    break;

  case btfparse::BTFKind::Typedef:
    stream << std::get<btfparse::TypedefBTFType>(type);
    break;

  case btfparse::BTFKind::Volatile:
    stream << std::get<btfparse::VolatileBTFType>(type);
    break;

  case btfparse::BTFKind::Const:
    stream << std::get<btfparse::ConstBTFType>(type);
    break;

  case btfparse::BTFKind::Restrict:
    stream << std::get<btfparse::RestrictBTFType>(type);
    break;

  case btfparse::BTFKind::Func:
    stream << std::get<btfparse::FuncBTFType>(type);
    break;

  case btfparse::BTFKind::FuncProto:
    stream << std::get<btfparse::FuncProtoBTFType>(type);
    break;

  case btfparse::BTFKind::Var:
    stream << std::get<btfparse::VarBTFType>(type);
    break;

  case btfparse::BTFKind::DataSec:
    stream << std::get<btfparse::DataSecBTFType>(type);
    break;

  case btfparse::BTFKind::Float:
    stream << std::get<btfparse::FloatBTFType>(type);
    break;

  default:
    stream << "Unknown";
    break;
  }

  return stream;
}