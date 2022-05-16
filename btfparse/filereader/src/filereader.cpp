//
// Copyright (c) 2021-present, Trail of Bits, Inc.
// All rights reserved.
//
// This source code is licensed in accordance with the terms specified in
// the LICENSE file found in the root directory of this source tree.
//

#include "filereader.h"

#include <array>
#include <cstring>
#include <fstream>

namespace btfparse {

struct FileReader::PrivateData final {
  Context context;
};

Result<IFileReader::Ptr, FileReaderError>
FileReader::create(IStream::Ptr stream) noexcept {
  try {
    return Ptr(new FileReader(std::move(stream)));

  } catch (const std::bad_alloc &) {
    return FileReaderError(FileReaderErrorInformation{
        FileReaderErrorInformation::Code::MemoryAllocationFailure,
    });

  } catch (const FileReaderError &e) {
    return e;
  }
}

FileReader::~FileReader() {}

void FileReader::setEndianness(bool little_endian) {
  setEndianness(d->context, little_endian);
}

void FileReader::seek(std::uint64_t offset) { seek(d->context, offset); }

std::uint64_t FileReader::offset() const { return offset(d->context); }

void FileReader::read(std::uint8_t *buffer, std::size_t size) {
  read(d->context, buffer, size);
}

std::uint8_t FileReader::u8() { return u8(d->context); }

std::uint16_t FileReader::u16() { return u16(d->context); }

std::uint32_t FileReader::u32() { return u32(d->context); }

std::uint64_t FileReader::u64() { return u64(d->context); }

FileReader::FileReader(IStream::Ptr stream) : d(new PrivateData) {
  d->context.stream = std::move(stream);
}

void FileReader::setEndianness(Context &context, bool little_endian) {
  context.little_endian = little_endian;
}

void FileReader::seek(Context &context, std::uint64_t offset) {
  if (!context.stream->seek(offset)) {
    throw FileReaderError(
        {FileReaderErrorInformation::Code::IOError,
         FileReaderErrorInformation::ReadOperation{offset, 0}});
  }
}

std::uint64_t FileReader::offset(Context &context) {
  return context.stream->offset();
}

void FileReader::read(Context &context, std::uint8_t *buffer,
                      std::size_t size) {
  auto read_offset = offset(context);
  if (!context.stream->read(buffer, size)) {
    throw FileReaderError(
        {FileReaderErrorInformation::Code::IOError,
         FileReaderErrorInformation::ReadOperation{read_offset, size}});
  }
}

std::uint8_t FileReader::u8(Context &context) {
  std::uint8_t value{};
  read(context, &value, 1);

  return value;
}

std::uint16_t FileReader::u16(Context &context) {
  std::array<std::uint8_t, 2> read_buffer;
  read(context, read_buffer.data(), read_buffer.size());

  std::uint16_t value{};
  if (context.little_endian) {
    value = static_cast<std::uint16_t>(read_buffer[0] | (read_buffer[1] << 8));
  } else {
    value = static_cast<std::uint16_t>(read_buffer[1] | (read_buffer[0] << 8));
  }

  return value;
}

std::uint32_t FileReader::u32(Context &context) {
  std::array<std::uint8_t, 4> read_buffer;
  read(context, read_buffer.data(), read_buffer.size());

  std::uint32_t value{};
  if (context.little_endian) {
    value = static_cast<std::uint32_t>(read_buffer[0] | (read_buffer[1] << 8) |
                                       (read_buffer[2] << 16) |
                                       (read_buffer[3] << 24));

  } else {
    value = static_cast<std::uint32_t>(read_buffer[3] | (read_buffer[2] << 8) |
                                       (read_buffer[1] << 16) |
                                       (read_buffer[0] << 24));
  }

  return value;
}

std::uint64_t FileReader::u64(Context &context) {
  std::array<std::uint8_t, 8> read_buffer;
  read(context, read_buffer.data(), read_buffer.size());

  auto L_shiftInteger = [](std::uint8_t value,
                           std::size_t shift_count) -> std::uint64_t {
    return static_cast<std::uint64_t>(value) << shift_count;
  };

  std::uint64_t value{};
  if (context.little_endian) {
    value =
        static_cast<std::uint64_t>(read_buffer[0]) |
        L_shiftInteger(read_buffer[1], 8) | L_shiftInteger(read_buffer[2], 16) |
        L_shiftInteger(read_buffer[3], 24) |
        L_shiftInteger(read_buffer[4], 32) |
        L_shiftInteger(read_buffer[5], 40) |
        L_shiftInteger(read_buffer[6], 48) | L_shiftInteger(read_buffer[7], 56);

  } else {
    value =
        static_cast<std::uint64_t>(read_buffer[7]) |
        L_shiftInteger(read_buffer[6], 8) | L_shiftInteger(read_buffer[5], 16) |
        L_shiftInteger(read_buffer[4], 24) |
        L_shiftInteger(read_buffer[3], 32) |
        L_shiftInteger(read_buffer[2], 40) |
        L_shiftInteger(read_buffer[1], 48) | L_shiftInteger(read_buffer[0], 56);
  }

  return value;
}

} // namespace btfparse
