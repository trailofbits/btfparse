//
// Copyright (c) 2021-present, Trail of Bits, Inc.
// All rights reserved.
//
// This source code is licensed in accordance with the terms specified in
// the LICENSE file found in the root directory of this source tree.
//

#pragma once

#include <btfparse/ifilereader.h>

namespace btfparse {

class FileReader final : public IFileReader {
public:
  static Result<IFileReader::Ptr, FileReaderError>
  create(IStream::Ptr stream) noexcept;

  virtual ~FileReader() override;

  virtual void setEndianness(bool little_endian) override;

  virtual void seek(std::uint64_t offset) override;
  virtual std::uint64_t offset() const override;

  virtual void read(std::uint8_t *buffer, std::size_t size) override;
  virtual std::uint8_t u8() override;
  virtual std::uint16_t u16() override;
  virtual std::uint32_t u32() override;
  virtual std::uint64_t u64() override;

private:
  struct PrivateData;
  std::unique_ptr<PrivateData> d;

  FileReader(IStream::Ptr stream);

public:
  struct Context final {
    IStream::Ptr stream;
    bool little_endian{true};
  };

  static void setEndianness(Context &context, bool little_endian);

  static void seek(Context &context, std::uint64_t offset);
  static std::uint64_t offset(Context &context);

  static void read(Context &context, std::uint8_t *buffer, std::size_t size);
  static std::uint8_t u8(Context &context);
  static std::uint16_t u16(Context &context);
  static std::uint32_t u32(Context &context);
  static std::uint64_t u64(Context &context);

  friend class IFileReader;
};

} // namespace btfparse
