//
// Copyright (c) 2021-present, Trail of Bits, Inc.
// All rights reserved.
//
// This source code is licensed in accordance with the terms specified in
// the LICENSE file found in the root directory of this source tree.
//

#include "filereader.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <array>

namespace btfparse {

class MockedStream final : public IStream {
public:
  MockedStream() = default;
  virtual ~MockedStream() override = default;

  virtual bool seek(std::uint64_t offset) override {
    if (fail_seeks) {
      return false;
    }

    current_offset = offset;
    return true;
  }

  virtual std::uint64_t offset() const override { return current_offset; }

  virtual bool read(std::uint8_t *buffer, std::size_t size) override {
    if (fail_reads) {
      return false;
    }

    current_offset += size;

    buffer[0] = 0xFF;
    for (std::size_t i = 1; i < size; ++i) {
      buffer[i] = 0;
    }

    return true;
  }

  bool fail_seeks{false};
  bool fail_reads{false};
  std::uint64_t current_offset{};
};

TEST_CASE("FileReader::setEndianness()") {
  FileReader::Context context;
  context.little_endian = true;

  FileReader::setEndianness(context, false);
  CHECK(context.little_endian == false);

  FileReader::setEndianness(context, true);
  CHECK(context.little_endian == true);
}

TEST_CASE("FileReader::seek(), FileReader::offset()") {
  FileReader::Context context;
  context.stream = std::make_unique<MockedStream>();

  auto &mocked_stream = *static_cast<MockedStream *>(context.stream.get());
  mocked_stream.current_offset = 0;

  FileReader::seek(context, 10);
  CHECK(mocked_stream.current_offset == 10);
  CHECK(FileReader::offset(context) == 10);

  FileReader::seek(context, 20);
  CHECK(mocked_stream.current_offset == 20);
  CHECK(FileReader::offset(context) == 20);

  mocked_stream.fail_seeks = true;
  std::optional<FileReaderError> opt_file_reader_error;

  try {
    FileReader::seek(context, 10);
  } catch (FileReaderError error) {
    opt_file_reader_error = std::move(error);
  }

  REQUIRE(opt_file_reader_error.has_value());

  const auto &error_information = opt_file_reader_error.value().get();
  CHECK(error_information.code == FileReaderErrorInformation::Code::IOError);
}

TEST_CASE("FileReader::read()") {
  FileReader::Context context;
  context.stream = std::make_unique<MockedStream>();

  auto &mocked_stream = *static_cast<MockedStream *>(context.stream.get());
  mocked_stream.current_offset = 0;

  std::array<std::uint8_t, 4> read_buffer;

  FileReader::read(context, read_buffer.data(), 1);
  CHECK(FileReader::offset(context) == 1);

  FileReader::read(context, read_buffer.data(), 4);
  CHECK(FileReader::offset(context) == 5);

  mocked_stream.fail_reads = true;
  std::optional<FileReaderError> opt_file_reader_error;

  try {
    FileReader::read(context, read_buffer.data(), 1);
  } catch (FileReaderError error) {
    opt_file_reader_error = std::move(error);
  }

  REQUIRE(opt_file_reader_error.has_value());

  const auto &error_information = opt_file_reader_error.value().get();
  CHECK(error_information.code == FileReaderErrorInformation::Code::IOError);

  REQUIRE(error_information.opt_read_operation.has_value());
  const auto &read_operation = error_information.opt_read_operation.value();

  CHECK(read_operation.offset == 5);
  CHECK(read_operation.size == 1);
}

TEST_CASE("FileReader::u8()") {
  FileReader::Context context;
  context.stream = std::make_unique<MockedStream>();

  auto value = FileReader::u8(context);
  CHECK(value == 0xFF);
}

TEST_CASE("FileReader::u16()") {
  FileReader::Context context;
  context.stream = std::make_unique<MockedStream>();

  FileReader::setEndianness(context, true);
  auto value = FileReader::u16(context);
  CHECK(value == 0xFF);

  FileReader::setEndianness(context, false);
  value = FileReader::u16(context);
  CHECK(value == 0xFF00);
}

TEST_CASE("FileReader::u32()") {
  FileReader::Context context;
  context.stream = std::make_unique<MockedStream>();

  FileReader::setEndianness(context, true);
  auto value = FileReader::u32(context);
  CHECK(value == 0xFF);

  FileReader::setEndianness(context, false);
  value = FileReader::u32(context);
  CHECK(value == 0xFF000000);
}

TEST_CASE("FileReader::u64()") {
  FileReader::Context context;
  context.stream = std::make_unique<MockedStream>();

  FileReader::setEndianness(context, true);
  auto value = FileReader::u64(context);
  CHECK(value == 0xFF);

  FileReader::setEndianness(context, false);
  value = FileReader::u64(context);
  CHECK(value == 0xFF00000000000000ULL);
}

} // namespace btfparse
