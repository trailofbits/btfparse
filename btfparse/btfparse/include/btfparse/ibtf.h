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

namespace btfparse {

struct BTFErrorInformation final {
  enum class Code {
    Unknown,
    MemoryAllocationFailure,
    FileNotFound,
    IOError,
    InvalidMagicValue,
    InvalidBTFKind,
    InvalidIntBTFTypeEncoding,
    InvalidPtrBTFTypeEncoding,
    InvalidArrayBTFTypeEncoding,
    InvalidTypedefBTFTypeEncoding,
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

    case BTFErrorInformation::Code::InvalidIntBTFTypeEncoding:
      buffer << "Invalid encoding of `Int` BTFType";
      break;

    case BTFErrorInformation::Code::InvalidPtrBTFTypeEncoding:
      buffer << "Invalid encoding of `Ptr` BTFType";
      break;

    case BTFErrorInformation::Code::InvalidArrayBTFTypeEncoding:
      buffer << "Invalid encoding of `Array` BTFType";
      break;

    case BTFErrorInformation::Code::InvalidTypedefBTFTypeEncoding:
      buffer << "Invalid encoding of `Typedef` BTFType";
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
  std::string name;

  bool is_signed{false};
  bool is_char{false};
  bool is_bool{false};

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

using BTFType = std::variant<std::monostate, IntBTFType, PtrBTFType,
                             ConstBTFType, ArrayBTFType, TypedefBTFType>;

class IBTF {
public:
  using Ptr = std::unique_ptr<IBTF>;

  static Result<Ptr, BTFError>
  createFromPath(const std::filesystem::path &path);

  IBTF() = default;
  virtual ~IBTF() = default;

  IBTF(const IBTF &) = delete;
  IBTF &operator=(const IBTF &) = delete;
};

} // namespace btfparse
