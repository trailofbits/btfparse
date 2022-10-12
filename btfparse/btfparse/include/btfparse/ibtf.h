//
// Copyright (c) 2021-present, Trail of Bits, Inc.
// All rights reserved.
//
// This source code is licensed in accordance with the terms specified in
// the LICENSE file found in the root directory of this source tree.
//

#pragma once

#include <btfparse/error.h>
#include <btfparse/result.h>

#include <filesystem>
#include <memory>
#include <optional>
#include <sstream>
#include <unordered_map>
#include <vector>

namespace btfparse {

struct BTFErrorInformation final {
  enum class Code {
    Unknown,
    MemoryAllocationFailure,
    FileNotFound,
    IOError,
    InvalidMagicValue,
    InvalidBTFKind,
    UnsupportedBTFKind,
    InvalidIntBTFTypeEncoding,
    InvalidPtrBTFTypeEncoding,
    InvalidArrayBTFTypeEncoding,
    InvalidTypedefBTFTypeEncoding,
    InvalidEnumBTFTypeEncoding,
    InvalidFuncProtoBTFTypeEncoding,
    InvalidVolatileBTFTypeEncoding,
    InvalidFwdBTFTypeEncoding,
    InvalidFuncBTFTypeEncoding,
    InvalidFloatBTFTypeEncoding,
    InvalidRestrictBTFTypeEncoding,
    InvalidVarBTFTypeEncoding,
    InvalidDataSecBTFTypeEncoding,
    InvalidStringOffset,
  };

  struct FileRange final {
    std::uint64_t offset{};
    std::size_t size{};
  };

  using OptionalFileRange = std::optional<FileRange>;

  Code code{Code::Unknown};
  OptionalFileRange opt_file_range;
};

struct BTFErrorInformationPrinter final {
  std::string operator()(const BTFErrorInformation &error_information) const {
    std::stringstream buffer;
    buffer << "Error: '";

    switch (error_information.code) {
    case BTFErrorInformation::Code::Unknown:
      buffer << "Unknown error";
      break;

    case BTFErrorInformation::Code::MemoryAllocationFailure:
      buffer << "Memory allocation failure";
      break;

    case BTFErrorInformation::Code::FileNotFound:
      buffer << "File not found";
      break;

    case BTFErrorInformation::Code::IOError:
      buffer << "IO error";
      break;

    case BTFErrorInformation::Code::InvalidMagicValue:
      buffer << "Invalid magic value";
      break;

    case BTFErrorInformation::Code::InvalidBTFKind:
      buffer << "Invalid BTF kind";
      break;

    case BTFErrorInformation::Code::UnsupportedBTFKind:
      buffer << "Unsupported BTF kind";
      break;

    case BTFErrorInformation::Code::InvalidIntBTFTypeEncoding:
      buffer << "Invalid encoding for `Int` BTFType";
      break;

    case BTFErrorInformation::Code::InvalidPtrBTFTypeEncoding:
      buffer << "Invalid encoding for `Ptr` BTFType";
      break;

    case BTFErrorInformation::Code::InvalidArrayBTFTypeEncoding:
      buffer << "Invalid encoding for `Array` BTFType";
      break;

    case BTFErrorInformation::Code::InvalidTypedefBTFTypeEncoding:
      buffer << "Invalid encoding for `Typedef` BTFType";
      break;

    case BTFErrorInformation::Code::InvalidEnumBTFTypeEncoding:
      buffer << "Invalid encoding for `Enum` BTFType";
      break;

    case BTFErrorInformation::Code::InvalidFuncProtoBTFTypeEncoding:
      buffer << "Invalid encoding for `FuncProto` BTFType";
      break;

    case BTFErrorInformation::Code::InvalidVolatileBTFTypeEncoding:
      buffer << "Invalid encoding for `Volatile` BTFType";
      break;

    case BTFErrorInformation::Code::InvalidFwdBTFTypeEncoding:
      buffer << "Invalid encoding for `Fwd` BTFType";
      break;

    case BTFErrorInformation::Code::InvalidFuncBTFTypeEncoding:
      buffer << "Invalid encoding for `Func` BTFType";
      break;

    case BTFErrorInformation::Code::InvalidFloatBTFTypeEncoding:
      buffer << "Invalid encoding for `Float` BTFType";
      break;

    case BTFErrorInformation::Code::InvalidRestrictBTFTypeEncoding:
      buffer << "Invalid encoding for `Restrict` BTFType";
      break;

    case BTFErrorInformation::Code::InvalidVarBTFTypeEncoding:
      buffer << "Invalid encoding for `Var` BTFType";
      break;

    case BTFErrorInformation::Code::InvalidDataSecBTFTypeEncoding:
      buffer << "Invalid encoding for `DataSec` BTFType";
      break;

    case BTFErrorInformation::Code::InvalidStringOffset:
      buffer << "Invalid string offset";
      break;
    }

    buffer << "'";

    if (error_information.opt_file_range.has_value()) {
      const auto &file_range = error_information.opt_file_range.value();
      buffer << ", File range: " << file_range.offset << " - "
             << (file_range.offset + file_range.size);
    }

    return buffer.str();
  }
};

using BTFError = Error<BTFErrorInformation, BTFErrorInformationPrinter>;

struct IntBTFType final {
  enum class Encoding {
    None,
    Signed,
    Char,
    Bool,
  };

  std::string name;
  std::uint32_t size{};
  Encoding encoding{Encoding::None};

  std::uint8_t offset{};
  std::uint8_t bits{};
};

struct PtrBTFType final {
  std::uint32_t type{};
};

struct ConstBTFType final {
  std::uint32_t type{};
};

struct ArrayBTFType final {
  std::uint32_t type{};
  std::uint32_t index_type{};
  std::uint32_t nelems{};
};

struct TypedefBTFType final {
  std::string name;
  std::uint32_t type{};
};

struct EnumBTFType final {
  struct Value final {
    std::string name;
    std::int32_t val{};
  };

  using ValueList = std::vector<Value>;

  std::optional<std::string> opt_name;
  std::uint32_t size{};
  ValueList value_list;
};

struct FuncProtoBTFType final {
  struct Param final {
    std::optional<std::string> opt_name;
    std::uint32_t type{};
  };

  using ParamList = std::vector<Param>;

  std::uint32_t return_type{};
  ParamList param_list;
  bool is_variadic{false};
};

struct VolatileBTFType final {
  std::uint32_t type{};
};

struct StructBTFType final {
  struct Member final {
    std::optional<std::string> opt_name;
    std::uint32_t type{};
    std::uint32_t offset{};
    std::optional<std::uint8_t> opt_bitfield_size;
  };

  using MemberList = std::vector<Member>;

  std::optional<std::string> opt_name;
  std::uint32_t size{};
  MemberList member_list;
};

struct UnionBTFType final {
  struct Member final {
    std::optional<std::string> opt_name;
    std::uint32_t type{};
    std::uint32_t offset{};
    std::optional<std::uint8_t> opt_bitfield_size;
  };

  using MemberList = std::vector<Member>;

  std::optional<std::string> opt_name;
  std::uint32_t size{};
  MemberList member_list;
};

struct FwdBTFType final {
  std::string name;
  bool is_union{false};
};

struct FuncBTFType final {
  enum class Linkage {
    Static = 0,
    Global = 1,
    Extern = 2,
  };

  std::string name;
  std::uint32_t type{};
  Linkage linkage{Linkage::Static};
};

struct FloatBTFType final {
  std::string name;
  std::uint32_t size{};
};

struct RestrictBTFType final {
  std::uint32_t type{};
};

struct VarBTFType final {
  std::string name;
  std::uint32_t type{};
  std::uint32_t linkage{};
};

struct DataSecBTFType final {
  struct Variable final {
    std::uint32_t type{};
    std::uint32_t offset{};
    std::uint32_t size{};
  };

  using VariableList = std::vector<Variable>;

  std::string name;
  std::uint32_t size{};
  VariableList variable_list;
};

enum class BTFKind {
  Void = 0,
  Int = 1,
  Ptr = 2,
  Array = 3,
  Struct = 4,
  Union = 5,
  Enum = 6,
  Fwd = 7,
  Typedef = 8,
  Volatile = 9,
  Const = 10,
  Restrict = 11,
  Func = 12,
  FuncProto = 13,
  Var = 14,
  DataSec = 15,
  Float = 16,
};

using BTFType =
    std::variant<std::monostate, IntBTFType, PtrBTFType, ArrayBTFType,
                 StructBTFType, UnionBTFType, EnumBTFType, FwdBTFType,
                 TypedefBTFType, VolatileBTFType, ConstBTFType, RestrictBTFType,
                 FuncBTFType, FuncProtoBTFType, VarBTFType, DataSecBTFType,
                 FloatBTFType>;

using BTFTypeMap = std::unordered_map<std::uint32_t, BTFType>;
using PathList = std::vector<std::filesystem::path>;

class IBTF {
public:
  using Ptr = std::unique_ptr<IBTF>;

  static Result<Ptr, BTFError>
  createFromPath(const std::filesystem::path &path) noexcept;

  static Result<Ptr, BTFError>
  createFromPathList(const PathList &path_list) noexcept;

  virtual std::optional<BTFType> getType(std::uint32_t id) const noexcept = 0;
  virtual std::optional<BTFKind> getKind(std::uint32_t id) const noexcept = 0;

  virtual std::uint32_t count() const noexcept = 0;
  virtual BTFTypeMap getAll() const noexcept = 0;

  static BTFKind getBTFTypeKind(const BTFType &btf_type) noexcept;

  IBTF() = default;
  virtual ~IBTF() = default;

  IBTF(const IBTF &) = delete;
  IBTF &operator=(const IBTF &) = delete;
};

} // namespace btfparse
