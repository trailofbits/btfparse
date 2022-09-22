//
// Copyright (c) 2021-present, Trail of Bits, Inc.
// All rights reserved.
//
// This source code is licensed in accordance with the terms specified in
// the LICENSE file found in the root directory of this source tree.
//

#include "memoryfileadapter.h"

#include <cstring>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <btfparse/ifilereader.h>

namespace btfparse {
MemoryFileAdapter::MemoryFileAdapter(std::unique_ptr<char[]> buffer,
                                     std::size_t size)
    : file_buffer(std::move(buffer)), file_pos(0), file_buffer_size(size) {}

MemoryFileAdapter::~MemoryFileAdapter() {}

IStream::Ptr MemoryFileAdapter::create(const std::filesystem::path &path) {
  try {
    auto fd = open(path.string().c_str(), O_RDONLY);

    if (fd < 0) {
      throw FileReaderError(FileReaderErrorInformation{
          FileReaderErrorInformation::Code::FileNotFound});
    }

    struct stat stat_data {};
    auto stat_res = fstat(fd, &stat_data);

    if (stat_res < 0) {
      throw FileReaderError(FileReaderErrorInformation{
          FileReaderErrorInformation::Code::IOError});
    }

    auto file_size = static_cast<std::size_t>(stat_data.st_size);

    if (file_size >= 1024 * 1024 * 10) {
      throw FileReaderError(FileReaderErrorInformation{
          FileReaderErrorInformation::Code::MemoryAllocationFailure,
      });
    }

    auto file_buffer = std::unique_ptr<char[]>(new char[file_size]);

    ssize_t read_res = 0;
    std::size_t pos = 0;

    do {
      auto read_size = std::min(file_size - pos, 4096UL);

      read_res = ::read(fd, &file_buffer[pos], read_size);

      if (read_res > 0) {
        pos += static_cast<std::size_t>(read_res);
      }
    } while (read_res > 0 && pos < file_size);

    if (read_res < 0 || (pos + 1) < file_size) {
      throw FileReaderError(FileReaderErrorInformation{
          FileReaderErrorInformation::Code::IOError});
    }

    return Ptr(new MemoryFileAdapter(std::move(file_buffer), file_size));

  } catch (const std::bad_alloc &) {
    throw FileReaderError(FileReaderErrorInformation{
        FileReaderErrorInformation::Code::MemoryAllocationFailure,
    });
  }
}

bool MemoryFileAdapter::seek(std::uint64_t offset) {
  if (offset >= file_buffer_size) {
    return false;
  }

  file_pos = offset;

  return true;
}

std::uint64_t MemoryFileAdapter::offset() const {
  return static_cast<std::uint64_t>(file_pos);
}

bool MemoryFileAdapter::read(std::uint8_t *buffer, std::size_t size) {
  if ((file_pos + size) > file_buffer_size) {
    return false;
  }

  std::memcpy(buffer, &file_buffer[file_pos], size);

  file_pos += size;

  return true;
}

} // namespace btfparse
