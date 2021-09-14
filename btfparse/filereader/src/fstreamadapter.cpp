//
// Copyright (c) 2021-present, Trail of Bits, Inc.
// All rights reserved.
//
// This source code is licensed in accordance with the terms specified in
// the LICENSE file found in the root directory of this source tree.
//

#include "fstreamadapter.h"

#include <btfparse/ifilereader.h>

#include <cstring>
#include <vector>

namespace btfparse {

IStream::Ptr FstreamAdapter::create(const std::filesystem::path &path) {
  try {
    return Ptr(new FstreamAdapter(path));

  } catch (const std::bad_alloc &) {
    throw FileReaderError(FileReaderErrorInformation{
        FileReaderErrorInformation::Code::MemoryAllocationFailure,
    });
  }
}

FstreamAdapter::~FstreamAdapter() {}

bool FstreamAdapter::seek(std::uint64_t offset) {
  input_stream.clear();

  input_stream.seekg(static_cast<std::streamoff>(offset));
  if (!input_stream) {
    return false;
  }

  return true;
}

std::uint64_t FstreamAdapter::offset() const {
  return static_cast<std::uint64_t>(input_stream.tellg());
}

bool FstreamAdapter::read(std::uint8_t *buffer, std::size_t size) {
  std::vector<char> temp_buffer(size);

  input_stream.read(temp_buffer.data(), static_cast<int>(temp_buffer.size()));
  if (!input_stream) {
    return false;
  }

  std::memcpy(buffer, temp_buffer.data(), temp_buffer.size());
  return true;
}

FstreamAdapter::FstreamAdapter(const std::filesystem::path &path)
    : input_stream(path.c_str(), std::ios::in | std::ios::binary) {
  if (!input_stream) {
    throw FileReaderError(FileReaderErrorInformation{
        FileReaderErrorInformation::Code::FileNotFound,
    });
  }
}

} // namespace btfparse
