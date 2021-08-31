//
// Copyright (c) 2021-present, Trail of Bits, Inc.
// All rights reserved.
//
// This source code is licensed in accordance with the terms specified in
// the LICENSE file found in the root directory of this source tree.
//

#pragma once

#include "btf_types.h"

#include <btfparse/ibtf.h>
#include <btfparse/ifilereader.h>

#include <vector>

namespace btfparse {

class BTF final : public IBTF {
public:
  virtual ~BTF() override;

private:
  struct PrivateData;
  std::unique_ptr<PrivateData> d;

  BTF(const std::filesystem::path &path);

public:
  using BTFTypeList = std::vector<BTFType>;

  static BTFError convertFileReaderError(const FileReaderError &error);

  static std::optional<BTFError> detectEndianness(bool &little_endian,
                                                  IFileReader &file_reader);

  static Result<BTFHeader, BTFError> readBTFHeader(IFileReader &file_reader);

  static Result<BTFTypeList, BTFError>
  parseTypeSection(const BTFHeader &btf_header, IFileReader &file_reader);

  static Result<BTFTypeHeader, BTFError>
  parseBTFTypeHeader(IFileReader &file_reader);

  static Result<BTFTypeIntData, BTFError>
  parseBTFTypeIntData(const BTFHeader &btf_header,
                      const BTFTypeHeader &btf_type_header,
                      IFileReader &file_reader);

  static Result<std::string, BTFError> parseString(IFileReader &file_reader,
                                                   std::uint64_t offset);

  friend class IBTF;
};

} // namespace btfparse
