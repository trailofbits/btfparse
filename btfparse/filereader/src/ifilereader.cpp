//
// Copyright (c) 2021-present, Trail of Bits, Inc.
// All rights reserved.
//
// This source code is licensed in accordance with the terms specified in
// the LICENSE file found in the root directory of this source tree.
//

#include "filereader.h"
#include "memoryfileadapter.h"

#include <btfparse/ifilereader.h>

namespace btfparse {

Result<IFileReader::Ptr, FileReaderError>
IFileReader::open(const std::filesystem::path &path) noexcept {
  MemoryFileAdapter::Ptr memoryfile_ptr;

  try {
    memoryfile_ptr = MemoryFileAdapter::create(path);

  } catch (const std::bad_alloc &) {
    return FileReaderError(FileReaderErrorInformation{
        FileReaderErrorInformation::Code::MemoryAllocationFailure,
    });

  } catch (const FileReaderError &e) {
    return e;
  }

  return FileReader::create(std::move(memoryfile_ptr));
}

Result<IFileReader::Ptr, FileReaderError>
IFileReader::createFromStream(IStream::Ptr stream) noexcept {
  return FileReader::create(std::move(stream));
}

} // namespace btfparse
