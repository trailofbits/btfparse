//
// Copyright (c) 2021-present, Trail of Bits, Inc.
// All rights reserved.
//
// This source code is licensed in accordance with the terms specified in
// the LICENSE file found in the root directory of this source tree.
//

#include "btf.h"

#include <unordered_map>

namespace btfparse {

namespace {

const std::unordered_map<BTFKind, BTFTypeParser> kBTFParserMap{
    {BTFKind::Int, BTF::parseIntData},
    {BTFKind::Ptr, BTF::parsePtrData},
    {BTFKind::Const, BTF::parseConstData},
    {BTFKind::Array, BTF::parseArrayData},
    {BTFKind::Typedef, BTF::parseTypedefData},
    {BTFKind::Enum, BTF::parseEnumData},
    {BTFKind::FuncProto, BTF::parseFuncProtoData},
    {BTFKind::Volatile, BTF::parseVolatileData},
    {BTFKind::Struct, BTF::parseStructData},
    {BTFKind::Union, BTF::parseUnionData},
    {BTFKind::Fwd, BTF::parseFwdData},
    {BTFKind::Func, BTF::parseFuncData},
    {BTFKind::Float, BTF::parseFloatData},
    {BTFKind::Restrict, BTF::parseRestrictData},
    {BTFKind::Var, BTF::parseVarData},
    {BTFKind::DataSec, BTF::parseDataSecData}};

template <typename Type>
std::optional<BTFError>
parseStructOrUnionData(Type &output, const BTFFileList &btf_file_list,
                       const BTFTypeHeader &btf_type_header,
                       IFileReader &file_reader) noexcept {

  static_assert(std::is_same<Type, StructBTFType>::value ||
                    std::is_same<Type, UnionBTFType>::value,
                "Type must be either StructBTFType or UnionBTFType");

  try {
    output = {};

    output.size = btf_type_header.size_or_type;

    if (btf_type_header.name_off != 0) {
      auto name_res = BTF::parseString(btf_file_list, btf_type_header.name_off);
      if (name_res.failed()) {
        return name_res.takeError();
      }

      output.opt_name = name_res.takeValue();
    }

    for (std::uint32_t i = 0; i < btf_type_header.vlen; ++i) {
      typename Type::Member member{};

      auto member_name_off = file_reader.u32();
      if (member_name_off != 0) {
        auto member_name_res = BTF::parseString(btf_file_list, member_name_off);
        if (member_name_res.failed()) {
          return member_name_res.takeError();
        }

        member.opt_name = member_name_res.takeValue();
      }

      member.type = file_reader.u32();

      auto offset = file_reader.u32();
      if (btf_type_header.kind_flag) {
        member.offset = offset & 0xFFFFFFUL;
        member.opt_bitfield_size = static_cast<std::uint8_t>(offset >> 24);

      } else {
        member.offset = offset;
      }

      output.member_list.push_back(std::move(member));
    }

    return std::nullopt;

  } catch (const FileReaderError &error) {
    return BTF::convertFileReaderError(error);
  }
}

} // namespace

struct BTF::PrivateData final {
  BTFTypeMap btf_type_map;
};

BTF::~BTF() {}

std::optional<BTFType> BTF::getType(std::uint32_t id) const noexcept {
  auto btf_type_map_it = d->btf_type_map.find(id);
  if (btf_type_map_it == d->btf_type_map.end()) {
    return std::nullopt;
  }

  return btf_type_map_it->second;
}

std::optional<BTFKind> BTF::getKind(std::uint32_t id) const noexcept {
  auto btf_type_map_it = d->btf_type_map.find(id);
  if (btf_type_map_it == d->btf_type_map.end()) {
    return std::nullopt;
  }

  const auto &btf_type = btf_type_map_it->second;
  return getBTFTypeKind(btf_type);
}

std::uint32_t BTF::count() const noexcept {
  return static_cast<std::uint32_t>(d->btf_type_map.size());
}

BTFTypeMap BTF::getAll() const noexcept { return d->btf_type_map; }

BTF::BTF(const PathList &path_list) : d(new PrivateData) {
  BTFFileList btf_file_list;

  for (const auto &path : path_list) {
    auto file_reader_res = IFileReader::open(path);
    if (file_reader_res.failed()) {
      throw convertFileReaderError(file_reader_res.takeError());
    }

    BTFFile btf_file;
    btf_file.file_reader = file_reader_res.takeValue();

    auto &file_reader = *btf_file.file_reader.get();

    bool little_endian{false};
    auto opt_error = detectEndianness(little_endian, file_reader);
    if (opt_error.has_value()) {
      throw opt_error.value();
    }

    file_reader.setEndianness(little_endian);

    auto btf_header_res = readBTFHeader(file_reader);
    if (btf_header_res.failed()) {
      throw btf_header_res.takeError();
    }

    btf_file.btf_header = btf_header_res.takeValue();
    btf_file_list.push_back(std::move(btf_file));
  }

  auto btf_type_map_res = parseTypeSections(btf_file_list);
  if (btf_type_map_res.failed()) {
    throw btf_type_map_res.takeError();
  }

  d->btf_type_map = btf_type_map_res.takeValue();
}

BTFError BTF::convertFileReaderError(const FileReaderError &error) noexcept {
  const auto &file_reader_error_info = error.get();

  BTFErrorInformation::Code error_code{BTFErrorInformation::Code::Unknown};
  switch (file_reader_error_info.code) {
  case FileReaderErrorInformation::Code::Unknown:
    error_code = BTFErrorInformation::Code::Unknown;
    break;

  case FileReaderErrorInformation::Code::MemoryAllocationFailure:
    error_code = BTFErrorInformation::Code::MemoryAllocationFailure;
    break;

  case FileReaderErrorInformation::Code::FileNotFound:
    error_code = BTFErrorInformation::Code::FileNotFound;
    break;

  case FileReaderErrorInformation::Code::IOError:
    error_code = BTFErrorInformation::Code::IOError;
    break;
  }

  std::optional<BTFErrorInformation::FileRange> opt_file_range;
  if (file_reader_error_info.opt_read_operation.has_value()) {
    const auto &read_operation =
        file_reader_error_info.opt_read_operation.value();

    opt_file_range = BTFErrorInformation::FileRange{
        read_operation.offset,
        read_operation.size,
    };
  }

  return BTFError{
      BTFErrorInformation{
          error_code,
          opt_file_range,
      },
  };
}

std::optional<BTFError>
BTF::detectEndianness(bool &little_endian, IFileReader &file_reader) noexcept {
  try {
    file_reader.seek(0);
    file_reader.setEndianness(true);

    auto magic = file_reader.u16();
    if (magic == kLittleEndianMagicValue) {
      little_endian = true;

    } else if (magic == kBigEndianMagicValue) {
      little_endian = false;

    } else {
      return BTFError{
          BTFErrorInformation{
              BTFErrorInformation::Code::InvalidMagicValue,
          },
      };
    }

    return std::nullopt;

  } catch (const FileReaderError &error) {
    return convertFileReaderError(error);
  }
}

Result<BTFHeader, BTFError>
BTF::readBTFHeader(IFileReader &file_reader) noexcept {
  try {
    file_reader.seek(0);

    BTFHeader btf_header{};
    btf_header.magic = file_reader.u16();
    btf_header.version = file_reader.u8();
    btf_header.flags = file_reader.u8();
    btf_header.hdr_len = file_reader.u32();
    btf_header.type_off = file_reader.u32();
    btf_header.type_len = file_reader.u32();
    btf_header.str_off = file_reader.u32();
    btf_header.str_len = file_reader.u32();

    return btf_header;

  } catch (const FileReaderError &error) {
    return convertFileReaderError(error);
  }
}

Result<BTFTypeMap, BTFError>
BTF::parseTypeSections(const BTFFileList &btf_file_list) noexcept {
  BTFTypeMap btf_type_map;

  std::uint32_t type_id{1U};

  try {
    for (auto &btf_file : btf_file_list) {
      const auto &btf_header = btf_file.btf_header;
      auto &file_reader = *btf_file.file_reader.get();

      auto type_section_start_offset = btf_header.hdr_len + btf_header.type_off;
      auto type_section_end_offset =
          type_section_start_offset + btf_header.type_len;

      file_reader.seek(type_section_start_offset);

      for (;;) {
        auto current_offset = file_reader.offset();
        if (current_offset >= type_section_end_offset) {
          break;
        }

        auto btf_type_header_res = parseTypeHeader(file_reader);
        if (btf_type_header_res.failed()) {
          return btf_type_header_res.takeError();
        }

        auto btf_type_header = btf_type_header_res.takeValue();

        BTFErrorInformation::FileRange file_range{current_offset,
                                                  kBTFTypeHeaderSize};

        if (btf_type_header.kind > static_cast<std::uint8_t>(BTFKind::Float)) {
          return BTFError{
              BTFErrorInformation{BTFErrorInformation::Code::InvalidBTFKind,
                                  file_range},
          };
        }

        auto btf_kind = static_cast<BTFKind>(btf_type_header.kind);

        auto parser_it = kBTFParserMap.find(btf_kind);
        if (parser_it == kBTFParserMap.end()) {
          return BTFError{
              BTFErrorInformation{BTFErrorInformation::Code::UnsupportedBTFKind,
                                  file_range},
          };
        }

        const auto &parser = parser_it->second;

        auto btf_type_res = parser(btf_file_list, btf_type_header, file_reader);
        if (btf_type_res.failed()) {
          return btf_type_res.takeError();
        }

        btf_type_map.insert({type_id, btf_type_res.takeValue()});
        ++type_id;
      }
    }

    return btf_type_map;

  } catch (const FileReaderError &error) {
    return convertFileReaderError(error);
  }
}

Result<BTFTypeHeader, BTFError>
BTF::parseTypeHeader(IFileReader &file_reader) noexcept {

  try {
    BTFTypeHeader btf_type_common;
    btf_type_common.name_off = file_reader.u32();

    auto info = file_reader.u32();
    btf_type_common.vlen = info & 0xFFFFUL;
    btf_type_common.kind = (info & 0x1F000000UL) >> 24UL;
    btf_type_common.kind_flag = (info & 0x80000000UL) != 0;

    btf_type_common.size_or_type = file_reader.u32();

    return btf_type_common;

  } catch (const FileReaderError &error) {
    return convertFileReaderError(error);
  }
}

Result<BTFType, BTFError>
BTF::parseIntData(const BTFFileList &btf_file_list,
                  const BTFTypeHeader &btf_type_header,
                  IFileReader &file_reader) noexcept {

  BTFErrorInformation::FileRange file_range{
      file_reader.offset() - kBTFTypeHeaderSize,
      kBTFTypeHeaderSize + kIntBTFTypeSize};

  if (btf_type_header.kind_flag || btf_type_header.vlen != 0) {
    return BTFError{
        BTFErrorInformation{
            BTFErrorInformation::Code::InvalidIntBTFTypeEncoding,
            file_range,
        },
    };
  }

  switch (btf_type_header.size_or_type) {
  case 1:
  case 2:
  case 4:
  case 8:
  case 16:
    break;

  default: {
    return BTFError{
        BTFErrorInformation{
            BTFErrorInformation::Code::InvalidIntBTFTypeEncoding,
            file_range,
        },
    };
  }
  }

  try {
    auto name_res = parseString(btf_file_list, btf_type_header.name_off);
    if (name_res.failed()) {
      return name_res.takeError();
    }

    IntBTFType output;
    output.name = name_res.takeValue();
    output.size = btf_type_header.size_or_type;

    auto integer_info = file_reader.u32();

    auto encoding = (integer_info & 0x0F000000UL) >> 24;

    int is_signed = (encoding & 1) != 0;
    int is_char = (encoding & 2) != 0;
    int is_bool = (encoding & 4) != 0;

    if (is_signed + is_char + is_bool > 1) {
      return BTFError{
          BTFErrorInformation{
              BTFErrorInformation::Code::InvalidIntBTFTypeEncoding,
              file_range,
          },
      };
    }

    if (is_signed != 0) {
      output.encoding = IntBTFType::Encoding::Signed;

    } else if (is_char != 0) {
      output.encoding = IntBTFType::Encoding::Char;

    } else if (is_bool != 0) {
      output.encoding = IntBTFType::Encoding::Bool;

    } else {
      output.encoding = IntBTFType::Encoding::None;
    }

    output.bits = integer_info & 0x000000ff;
    if (output.bits > 128 || output.bits > btf_type_header.size_or_type * 8) {

      return BTFError{
          BTFErrorInformation{
              BTFErrorInformation::Code::InvalidIntBTFTypeEncoding,
              file_range,
          },
      };
    }

    output.offset = (integer_info & 0x00ff0000) >> 16;
    if (output.offset + output.bits > btf_type_header.size_or_type * 8) {

      return BTFError{
          BTFErrorInformation{
              BTFErrorInformation::Code::InvalidIntBTFTypeEncoding,
              file_range,
          },
      };
    }

    return BTFType{output};

  } catch (const FileReaderError &error) {
    return convertFileReaderError(error);
  }
}

Result<BTFType, BTFError>
BTF::parsePtrData(const BTFFileList &, const BTFTypeHeader &btf_type_header,
                  IFileReader &file_reader) noexcept {

  BTFErrorInformation::FileRange file_range{
      file_reader.offset() - kBTFTypeHeaderSize, kBTFTypeHeaderSize};

  if (btf_type_header.name_off != 0 || btf_type_header.kind_flag ||
      btf_type_header.vlen != 0) {

    return BTFError{
        BTFErrorInformation{
            BTFErrorInformation::Code::InvalidPtrBTFTypeEncoding,
            file_range,
        },
    };
  }

  PtrBTFType output;
  output.type = btf_type_header.size_or_type;

  return BTFType{output};
}

Result<BTFType, BTFError>
BTF::parseConstData(const BTFFileList &, const BTFTypeHeader &btf_type_header,
                    IFileReader &file_reader) noexcept {

  BTFErrorInformation::FileRange file_range{
      file_reader.offset() - kBTFTypeHeaderSize, kBTFTypeHeaderSize};

  if (btf_type_header.name_off != 0 || btf_type_header.kind_flag ||
      btf_type_header.vlen != 0) {

    return BTFError{
        BTFErrorInformation{
            BTFErrorInformation::Code::InvalidPtrBTFTypeEncoding,
            file_range,
        },
    };
  }

  ConstBTFType output;
  output.type = btf_type_header.size_or_type;

  return BTFType{output};
}

Result<BTFType, BTFError>
BTF::parseArrayData(const BTFFileList &, const BTFTypeHeader &btf_type_header,
                    IFileReader &file_reader) noexcept {

  BTFErrorInformation::FileRange file_range{
      file_reader.offset() - kBTFTypeHeaderSize,
      kBTFTypeHeaderSize + kArrayBTFTypeSize};

  if (btf_type_header.name_off != 0 || btf_type_header.kind_flag ||
      btf_type_header.vlen != 0 || btf_type_header.size_or_type != 0) {

    return BTFError{
        BTFErrorInformation{
            BTFErrorInformation::Code::InvalidArrayBTFTypeEncoding,
            file_range,
        },
    };
  }

  try {
    ArrayBTFType output;
    output.type = file_reader.u32();
    output.index_type = file_reader.u32();
    output.nelems = file_reader.u32();

    return BTFType{output};

  } catch (const FileReaderError &error) {
    return convertFileReaderError(error);
  }
}

Result<BTFType, BTFError>
BTF::parseTypedefData(const BTFFileList &btf_file_list,
                      const BTFTypeHeader &btf_type_header,
                      IFileReader &file_reader) noexcept {

  BTFErrorInformation::FileRange file_range{
      file_reader.offset() - kBTFTypeHeaderSize, kBTFTypeHeaderSize};

  if (btf_type_header.name_off == 0 || btf_type_header.kind_flag ||
      btf_type_header.vlen != 0) {

    return BTFError{
        BTFErrorInformation{
            BTFErrorInformation::Code::InvalidTypedefBTFTypeEncoding,
            file_range,
        },
    };
  }

  auto name_res = parseString(btf_file_list, btf_type_header.name_off);
  if (name_res.failed()) {
    return name_res.takeError();
  }

  TypedefBTFType output;
  output.name = name_res.takeValue();
  output.type = btf_type_header.size_or_type;

  return BTFType{output};
}

Result<BTFType, BTFError>
BTF::parseEnumData(const BTFFileList &btf_file_list,
                   const BTFTypeHeader &btf_type_header,
                   IFileReader &file_reader) noexcept {

  BTFErrorInformation::FileRange file_range{
      file_reader.offset() - kBTFTypeHeaderSize,
      kBTFTypeHeaderSize + (btf_type_header.vlen * kEnumValueBTFTypeSize)};

  if (btf_type_header.kind_flag) {
    return BTFError{
        BTFErrorInformation{
            BTFErrorInformation::Code::InvalidEnumBTFTypeEncoding,
            file_range,
        },
    };
  }

  switch (btf_type_header.size_or_type) {
  case 1:
  case 2:
  case 4:
  case 8:
    break;

  default:
    return BTFError{
        BTFErrorInformation{
            BTFErrorInformation::Code::InvalidEnumBTFTypeEncoding,
            file_range,
        },
    };
  }

  try {
    EnumBTFType output;
    output.size = btf_type_header.size_or_type;

    if (btf_type_header.name_off != 0) {
      auto name_res = parseString(btf_file_list, btf_type_header.name_off);
      if (name_res.failed()) {
        return name_res.takeError();
      }

      output.opt_name = name_res.takeValue();
    }

    for (std::uint32_t i = 0; i < btf_type_header.vlen; ++i) {
      auto value_name_off = file_reader.u32();
      if (value_name_off == 0) {
        return BTFError{
            BTFErrorInformation{
                BTFErrorInformation::Code::InvalidEnumBTFTypeEncoding,
                file_range,
            },
        };
      }

      auto value_name_res = parseString(btf_file_list, value_name_off);
      if (value_name_res.failed()) {
        return value_name_res.takeError();
      }

      EnumBTFType::Value enum_value{};
      enum_value.name = value_name_res.takeValue();
      enum_value.val = static_cast<std::int32_t>(file_reader.u32());

      output.value_list.push_back(std::move(enum_value));
    }

    return BTFType{output};

  } catch (const FileReaderError &error) {
    return convertFileReaderError(error);
  }
}

Result<BTFType, BTFError>
BTF::parseFuncProtoData(const BTFFileList &btf_file_list,
                        const BTFTypeHeader &btf_type_header,
                        IFileReader &file_reader) noexcept {

  BTFErrorInformation::FileRange file_range{
      file_reader.offset() - kBTFTypeHeaderSize, kBTFTypeHeaderSize};

  if (btf_type_header.name_off != 0 || btf_type_header.kind_flag) {
    return BTFError{
        BTFErrorInformation{
            BTFErrorInformation::Code::InvalidFuncProtoBTFTypeEncoding,
            file_range,
        },
    };
  }

  try {
    FuncProtoBTFType output;
    output.return_type = btf_type_header.size_or_type;

    for (std::uint32_t i = 0; i < btf_type_header.vlen; ++i) {
      FuncProtoBTFType::Param param{};

      auto param_name_off = file_reader.u32();
      if (param_name_off != 0) {
        auto param_name_res = parseString(btf_file_list, param_name_off);
        if (param_name_res.failed()) {
          return param_name_res.takeError();
        }

        param.opt_name = param_name_res.takeValue();
      }

      param.type = file_reader.u32();

      output.param_list.push_back(std::move(param));
    }

    if (!output.param_list.empty()) {
      const auto &last_element = output.param_list.back();

      if (!last_element.opt_name.has_value() && last_element.type == 0) {
        output.param_list.pop_back();
        output.is_variadic = true;
      }
    }

    return BTFType{output};

  } catch (const FileReaderError &error) {
    return convertFileReaderError(error);
  }
}

Result<BTFType, BTFError>
BTF::parseVolatileData(const BTFFileList &,
                       const BTFTypeHeader &btf_type_header,
                       IFileReader &file_reader) noexcept {

  BTFErrorInformation::FileRange file_range{
      file_reader.offset() - kBTFTypeHeaderSize, kBTFTypeHeaderSize};

  if (btf_type_header.name_off != 0 || btf_type_header.kind_flag ||
      btf_type_header.vlen != 0) {

    return BTFError{
        BTFErrorInformation{
            BTFErrorInformation::Code::InvalidVolatileBTFTypeEncoding,
            file_range,
        },
    };
  }

  VolatileBTFType output;
  output.type = btf_type_header.size_or_type;

  return BTFType{output};
}

Result<BTFType, BTFError>
BTF::parseStructData(const BTFFileList &btf_file_list,
                     const BTFTypeHeader &btf_type_header,
                     IFileReader &file_reader) noexcept {

  StructBTFType output;
  auto opt_error = parseStructOrUnionData(output, btf_file_list,
                                          btf_type_header, file_reader);

  if (opt_error.has_value()) {
    return opt_error.value();
  }

  return BTFType{output};
}

Result<BTFType, BTFError>
BTF::parseUnionData(const BTFFileList &btf_file_list,
                    const BTFTypeHeader &btf_type_header,
                    IFileReader &file_reader) noexcept {

  UnionBTFType output;
  auto opt_error = parseStructOrUnionData(output, btf_file_list,
                                          btf_type_header, file_reader);

  if (opt_error.has_value()) {
    return opt_error.value();
  }

  return BTFType{output};
}

Result<BTFType, BTFError>
BTF::parseFwdData(const BTFFileList &btf_file_list,
                  const BTFTypeHeader &btf_type_header,
                  IFileReader &file_reader) noexcept {

  BTFErrorInformation::FileRange file_range{
      file_reader.offset() - kBTFTypeHeaderSize, kBTFTypeHeaderSize};

  if (btf_type_header.name_off == 0 || btf_type_header.vlen != 0 ||
      btf_type_header.size_or_type != 0) {

    return BTFError{
        BTFErrorInformation{
            BTFErrorInformation::Code::InvalidFwdBTFTypeEncoding,
            file_range,
        },
    };
  }

  auto name_res = parseString(btf_file_list, btf_type_header.name_off);
  if (name_res.failed()) {
    return name_res.takeError();
  }

  FwdBTFType output;
  output.name = name_res.takeValue();
  output.is_union = btf_type_header.kind_flag;

  return BTFType{output};
}

Result<BTFType, BTFError>
BTF::parseFuncData(const BTFFileList &btf_file_list,
                   const BTFTypeHeader &btf_type_header,
                   IFileReader &file_reader) noexcept {

  BTFErrorInformation::FileRange file_range{
      file_reader.offset() - kBTFTypeHeaderSize, kBTFTypeHeaderSize};

  if (btf_type_header.name_off == 0 || btf_type_header.kind_flag ||
      btf_type_header.vlen >= 3) {

    return BTFError{
        BTFErrorInformation{
            BTFErrorInformation::Code::InvalidFuncBTFTypeEncoding,
            file_range,
        },
    };
  }

  auto name_res = parseString(btf_file_list, btf_type_header.name_off);
  if (name_res.failed()) {
    return name_res.takeError();
  }

  FuncBTFType output;
  output.name = name_res.takeValue();
  output.type = btf_type_header.size_or_type;
  output.linkage = static_cast<FuncBTFType::Linkage>(btf_type_header.vlen);

  return BTFType{output};
}

Result<BTFType, BTFError>
BTF::parseFloatData(const BTFFileList &btf_file_list,
                    const BTFTypeHeader &btf_type_header,
                    IFileReader &file_reader) noexcept {

  BTFErrorInformation::FileRange file_range{
      file_reader.offset() - kBTFTypeHeaderSize, kBTFTypeHeaderSize};

  if (btf_type_header.name_off == 0 || btf_type_header.kind_flag ||
      btf_type_header.vlen != 0) {

    return BTFError{
        BTFErrorInformation{
            BTFErrorInformation::Code::InvalidFloatBTFTypeEncoding,
            file_range,
        },
    };
  }

  switch (btf_type_header.size_or_type) {
  case 2:
  case 4:
  case 8:
  case 12:
  case 16:
    break;

  default:
    return BTFError{
        BTFErrorInformation{
            BTFErrorInformation::Code::InvalidFloatBTFTypeEncoding,
            file_range,
        },
    };
  }

  auto name_res = parseString(btf_file_list, btf_type_header.name_off);
  if (name_res.failed()) {
    return name_res.takeError();
  }

  FloatBTFType output;
  output.name = name_res.takeValue();
  output.size = btf_type_header.size_or_type;

  return BTFType{output};
}

Result<BTFType, BTFError>
BTF::parseRestrictData(const BTFFileList &,
                       const BTFTypeHeader &btf_type_header,
                       IFileReader &file_reader) noexcept {

  BTFErrorInformation::FileRange file_range{
      file_reader.offset() - kBTFTypeHeaderSize, kBTFTypeHeaderSize};

  if (btf_type_header.name_off != 0 || btf_type_header.kind_flag ||
      btf_type_header.vlen != 0) {

    return BTFError{
        BTFErrorInformation{
            BTFErrorInformation::Code::InvalidRestrictBTFTypeEncoding,
            file_range,
        },
    };
  }

  RestrictBTFType output;
  output.type = btf_type_header.size_or_type;

  return BTFType{output};
}

Result<BTFType, BTFError>
BTF::parseVarData(const BTFFileList &btf_file_list,
                  const BTFTypeHeader &btf_type_header,
                  IFileReader &file_reader) noexcept {

  BTFErrorInformation::FileRange file_range{file_reader.offset() -
                                                kBTFTypeHeaderSize,
                                            kBTFTypeHeaderSize + kVarDataSize};

  if (btf_type_header.name_off == 0 || btf_type_header.kind_flag ||
      btf_type_header.vlen != 0) {

    return BTFError{
        BTFErrorInformation{
            BTFErrorInformation::Code::InvalidVarBTFTypeEncoding,
            file_range,
        },
    };
  }

  auto name_res = parseString(btf_file_list, btf_type_header.name_off);
  if (name_res.failed()) {
    return name_res.takeError();
  }

  VarBTFType output;
  output.name = name_res.takeValue();
  output.type = btf_type_header.size_or_type;

  try {
    output.linkage = file_reader.u32();

    return BTFType{output};

  } catch (const FileReaderError &error) {
    return convertFileReaderError(error);
  }
}

Result<BTFType, BTFError>
BTF::parseDataSecData(const BTFFileList &btf_file_list,
                      const BTFTypeHeader &btf_type_header,
                      IFileReader &file_reader) noexcept {

  BTFErrorInformation::FileRange file_range{
      file_reader.offset() - kBTFTypeHeaderSize,
      kBTFTypeHeaderSize + (btf_type_header.vlen * kVarSecInfoSize)};

  if (btf_type_header.name_off == 0 || btf_type_header.kind_flag) {
    return BTFError{
        BTFErrorInformation{
            BTFErrorInformation::Code::InvalidDataSecBTFTypeEncoding,
            file_range,
        },
    };
  }

  auto name_res = parseString(btf_file_list, btf_type_header.name_off);
  if (name_res.failed()) {
    return name_res.takeError();
  }

  DataSecBTFType output;
  output.name = name_res.takeValue();
  output.size = btf_type_header.size_or_type;

  try {
    for (std::uint32_t i = 0; i < btf_type_header.vlen; ++i) {
      DataSecBTFType::Variable variable{};
      variable.type = file_reader.u32();
      variable.offset = file_reader.u32();
      variable.size = file_reader.u32();

      output.variable_list.push_back(std::move(variable));
    }

    return BTFType{output};

  } catch (const FileReaderError &error) {
    return convertFileReaderError(error);
  }
}

Result<std::string, BTFError> BTF::parseString(const BTFFileList &btf_file_list,
                                               std::uint64_t offset) noexcept {
  std::uint32_t start_offset{};

  for (const auto &btf_file : btf_file_list) {
    auto end_offset = start_offset + btf_file.btf_header.str_len;
    auto relative_offset = offset - start_offset;

    if (relative_offset < end_offset) {
      auto &file_reader = *btf_file.file_reader.get();

      auto absolute_offset = btf_file.btf_header.hdr_len +
                             btf_file.btf_header.str_off + relative_offset;

      return parseString(file_reader, absolute_offset);
    }

    start_offset = end_offset;
  }

  return BTFError{
      BTFErrorInformation{
          BTFErrorInformation::Code::InvalidStringOffset,
          btfparse::BTFErrorInformation::FileRange{offset, 0},
      },
  };
}

Result<std::string, BTFError> BTF::parseString(IFileReader &file_reader,
                                               std::uint64_t offset) noexcept {

  Result<std::string, BTFError> output;

  auto original_offset{file_reader.offset()};

  try {
    file_reader.seek(offset);
    std::string buffer;

    for (;;) {
      auto ch = static_cast<char>(file_reader.u8());
      if (ch == 0) {
        break;
      }

      buffer.push_back(ch);
    }

    output = buffer;

  } catch (const FileReaderError &error) {
    return convertFileReaderError(error);
  }

  try {
    file_reader.seek(original_offset);
  } catch (const FileReaderError &error) {
    output = convertFileReaderError(error);
  }

  return output;
}

} // namespace btfparse
