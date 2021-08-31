//
// Copyright (c) 2021-present, Trail of Bits, Inc.
// All rights reserved.
//
// This source code is licensed in accordance with the terms specified in
// the LICENSE file found in the root directory of this source tree.
//

#include "filereader.h"
#include "fstreamadapter.h"

#include <btfparse/ifilereader.h>

namespace btfparse {

Result<IFileReader::Ptr, FileReaderError>
IFileReader::open(const std::filesystem::path &path) {
  return FileReader::create(FstreamAdapter::create(path));
}

Result<IFileReader::Ptr, FileReaderError>
IFileReader::createFromStream(IStream::Ptr stream) {
  return FileReader::create(std::move(stream));
}

} // namespace btfparse
