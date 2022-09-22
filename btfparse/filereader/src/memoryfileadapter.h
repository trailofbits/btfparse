//
// Copyright (c) 2021-present, Trail of Bits, Inc.
// All rights reserved.
//
// This source code is licensed in accordance with the terms specified in
// the LICENSE file found in the root directory of this source tree.
//

#pragma once

#include <btfparse/istream.h>

#include <filesystem>
#include <vector>

namespace btfparse {

class MemoryFileAdapter final : public IStream {
private:
  std::unique_ptr<char[]> file_buffer;
  std::size_t file_pos;
  const std::size_t file_buffer_size;

public:
  MemoryFileAdapter() = delete;
  static Ptr create(const std::filesystem::path &path);
  virtual ~MemoryFileAdapter() override;

  virtual bool seek(std::uint64_t offset) override;
  virtual std::uint64_t offset() const override;
  virtual bool read(std::uint8_t *buffer, std::size_t size) override;

private:
  MemoryFileAdapter(std::unique_ptr<char[]> buffer, std::size_t size);
};

} // namespace btfparse
