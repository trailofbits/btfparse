//
// Copyright (c) 2021-present, Trail of Bits, Inc.
// All rights reserved.
//
// This source code is licensed in accordance with the terms specified in
// the LICENSE file found in the root directory of this source tree.
//

#pragma once

#include <btfparse/error.h>
#include <btfparse/istream.h>
#include <btfparse/result.h>

#include <filesystem>
#include <optional>
#include <sstream>

namespace btfparse {

struct FileReaderErrorInformation final {
  enum class Code {
    Unknown,
    MemoryAllocationFailure,
    FileNotFound,
    IOError,
  };

  struct ReadOperation final {
    std::uint64_t offset{};
    std::size_t size{};
  };

  using OptionalReadOperation = std::optional<ReadOperation>;

  Code code{Code::Unknown};
  OptionalReadOperation opt_read_operation;
};

struct FileReaderErrorInformationPrinter final {
  std::string
  operator()(const FileReaderErrorInformation &error_information) const {
    std::stringstream buffer;
    buffer << "Error: '";

    switch (error_information.code) {
    case FileReaderErrorInformation::Code::Unknown:
      buffer << "Unknown error";
      break;

    case FileReaderErrorInformation::Code::MemoryAllocationFailure:
      buffer << "Memory allocation failure";
      break;

    case FileReaderErrorInformation::Code::FileNotFound:
      buffer << "File not found";
      break;

    case FileReaderErrorInformation::Code::IOError:
      buffer << "IO error";
      break;
    }

    buffer << "'";

    if (error_information.opt_read_operation.has_value()) {
      const auto &read_operation = error_information.opt_read_operation.value();
      buffer << ", Read operation: " << read_operation.size
             << " bytes from offset " << read_operation.offset;
    }

    return buffer.str();
  }
};

using FileReaderError =
    Error<FileReaderErrorInformation, FileReaderErrorInformationPrinter>;

class IFileReader {
public:
  using Ptr = std::unique_ptr<IFileReader>;

  static Result<Ptr, FileReaderError>
  open(const std::filesystem::path &path) noexcept;
  static Result<Ptr, FileReaderError>
  createFromStream(IStream::Ptr stream) noexcept;

  IFileReader() = default;
  virtual ~IFileReader() = default;

  virtual void setEndianness(bool little_endian) = 0;

  virtual void seek(std::uint64_t offset) = 0;
  virtual std::uint64_t offset() const = 0;

  virtual void read(std::uint8_t *buffer, std::size_t size) = 0;
  virtual std::uint8_t u8() = 0;
  virtual std::uint16_t u16() = 0;
  virtual std::uint32_t u32() = 0;
  virtual std::uint64_t u64() = 0;

  IFileReader(const IFileReader &) = delete;
  IFileReader &operator=(const IFileReader &) = delete;
};

} // namespace btfparse
