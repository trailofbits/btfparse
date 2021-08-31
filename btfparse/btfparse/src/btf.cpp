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

struct BTF::PrivateData final {
  BTFTypeList btf_type_list;
};

BTF::~BTF() {}

BTF::BTF(const std::filesystem::path &path) : d(new PrivateData) {
  auto file_reader_res = IFileReader::open(path);
  if (file_reader_res.failed()) {
    throw convertFileReaderError(file_reader_res.takeError());
  }

  auto file_reader = file_reader_res.takeValue();

  bool little_endian{false};
  auto opt_error = detectEndianness(little_endian, *file_reader.get());
  if (opt_error.has_value()) {
    throw opt_error.value();
  }

  file_reader->setEndianness(little_endian);

  auto btf_header_res = readBTFHeader(*file_reader.get());
  if (btf_header_res.failed()) {
    throw btf_header_res.takeError();
  }

  auto btf_header = btf_header_res.takeValue();

  auto btf_type_list_res = parseTypeSection(btf_header, *file_reader.get());
  if (btf_type_list_res.failed()) {
    throw btf_type_list_res.takeError();
  }

  d->btf_type_list = btf_type_list_res.takeValue();
}

BTFError BTF::convertFileReaderError(const FileReaderError &error) {
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

std::optional<BTFError> BTF::detectEndianness(bool &little_endian,
                                              IFileReader &file_reader) {
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

Result<BTFHeader, BTFError> BTF::readBTFHeader(IFileReader &file_reader) {
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

Result<BTF::BTFTypeList, BTFError>
BTF::parseTypeSection(const BTFHeader &btf_header, IFileReader &file_reader) {

  try {
    BTFTypeList btf_type_list;

    auto type_section_start_offset = btf_header.hdr_len + btf_header.type_off;
    auto type_section_end_offset =
        type_section_start_offset + btf_header.type_len;

    file_reader.seek(type_section_start_offset);

    for (;;) {
      auto current_offset = file_reader.offset();
      if (current_offset >= type_section_end_offset) {
        break;
      }

      auto btf_type_header_res = parseBTFTypeHeader(file_reader);
      if (btf_type_header_res.failed()) {
        return btf_type_header_res.takeError();
      }

      auto btf_type_header = btf_type_header_res.takeValue();

      BTFType btf_type{};
      if (btf_type_header.kind == BTFKind_Int) {
        auto btf_type_int_res =
            parseBTFTypeIntData(btf_header, btf_type_header, file_reader);

        if (btf_type_int_res.failed()) {
          return btf_type_int_res.takeError();
        }

        btf_type = btf_type_int_res.takeValue();

      } else {
        BTFErrorInformation::FileRange file_range{current_offset,
                                                  kBTFTypeHeaderSize};

        return BTFError{
            BTFErrorInformation{BTFErrorInformation::Code::InvalidBTFKind,
                                file_range},
        };
      }

      btf_type_list.push_back(std::move(btf_type));
    }

    return btf_type_list;

  } catch (const FileReaderError &error) {
    return convertFileReaderError(error);
  }
}

Result<BTFTypeHeader, BTFError>
BTF::parseBTFTypeHeader(IFileReader &file_reader) {

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

Result<BTFTypeIntData, BTFError>
BTF::parseBTFTypeIntData(const BTFHeader &btf_header,
                         const BTFTypeHeader &btf_type_header,
                         IFileReader &file_reader) {

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
    auto name_offset = btf_header.str_off + btf_type_header.name_off;

    auto name_res = parseString(file_reader, name_offset);
    if (name_res.failed()) {
      return name_res.takeError();
    }

    BTFTypeIntData btf_type_data;
    btf_type_data.name = name_res.takeValue();

    auto integer_info = file_reader.u32();

    auto encoding = (integer_info & 0x0F000000UL) >> 24;
    btf_type_data.is_signed = (encoding & 1) != 0;
    btf_type_data.is_char = (encoding & 2) != 0;
    btf_type_data.is_bool = (encoding & 4) != 0;

    std::size_t encoding_flag_count{0U};
    if (btf_type_data.is_signed) {
      ++encoding_flag_count;
    }

    if (btf_type_data.is_char) {
      ++encoding_flag_count;
    }

    if (btf_type_data.is_bool) {
      ++encoding_flag_count;
    }

    if (encoding_flag_count > 1U) {
      return BTFError{
          BTFErrorInformation{
              BTFErrorInformation::Code::InvalidIntBTFTypeEncoding,
              file_range,
          },
      };
    }

    btf_type_data.bits = integer_info & 0x000000ff;
    if (btf_type_data.bits > 128 ||
        btf_type_data.bits > btf_type_header.size_or_type * 8) {
      return BTFError{
          BTFErrorInformation{
              BTFErrorInformation::Code::InvalidIntBTFTypeEncoding,
              file_range,
          },
      };
    }

    btf_type_data.offset = (integer_info & 0x00ff0000) >> 16;
    if (btf_type_data.offset + btf_type_data.bits >
        btf_type_header.size_or_type * 8) {
      return BTFError{
          BTFErrorInformation{
              BTFErrorInformation::Code::InvalidIntBTFTypeEncoding,
              file_range,
          },
      };
    }

    return btf_type_data;

  } catch (const FileReaderError &error) {
    return convertFileReaderError(error);
  }
}

Result<std::string, BTFError> BTF::parseString(IFileReader &file_reader,
                                               std::uint64_t offset) {

  Result<std::string, BTFError> output;
  auto original_offset = file_reader.offset();

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
    output = convertFileReaderError(error);
  }

  file_reader.seek(original_offset);
  return output;
}

} // namespace btfparse
