//
// Copyright (c) 2021-present, Trail of Bits, Inc.
// All rights reserved.
//
// This source code is licensed in accordance with the terms specified in
// the LICENSE file found in the root directory of this source tree.
//

#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <variant>

namespace btfparse {

const std::uint32_t kLittleEndianMagicValue{0xEB9F};
const std::uint32_t kBigEndianMagicValue{0x9FEB};
const std::size_t kBTFTypeHeaderSize{12U};
const std::size_t kIntBTFTypeSize{4U};

struct BTFHeader final {
  std::uint16_t magic{};
  std::uint8_t version{};
  std::uint8_t flags{};
  std::uint32_t hdr_len{};
  std::uint32_t type_off{};
  std::uint32_t type_len{};
  std::uint32_t str_off{};
  std::uint32_t str_len{};
};

struct BTFTypeHeader final {
  std::uint32_t name_off{};
  std::uint16_t vlen{};
  std::uint8_t kind{};
  bool kind_flag{false};
  std::uint32_t size_or_type{};
};

enum BTFKind {
  BTFKind_Void = 0,
  BTFKind_Int = 1,
  BTFKind_Ptr = 2,
  BTFKind_Array = 3,
  BTFKind_Struct = 4,
  BTFKind_Union = 5,
  BTFKind_Enum = 6,
  BTFKind_Fwd = 7,
  BTFKind_Typedef = 8,
  BTFKind_Volatile = 9,
  BTFKind_Const = 10,
  BTFKind_Restrict = 11,
  BTFKind_Func = 12,
  BTFKind_FuncProto = 13,
  BTFKind_Var = 14,
  BTFKind_DataSec = 15,
  BTFKind_Float = 16,
};

} // namespace btfparse
