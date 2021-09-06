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

using BTFTypeParser = Result<BTFType, BTFError> (*)(const BTFHeader &,
                                                    const BTFTypeHeader &,
                                                    IFileReader &);

class BTF final : public IBTF {
public:
  virtual ~BTF() override;

private:
  struct PrivateData;
  std::unique_ptr<PrivateData> d;

  BTF(const std::filesystem::path &path);

public:
  using BTFTypeList = std::vector<BTFType>;

  static BTFError convertFileReaderError(const FileReaderError &error) noexcept;

  static std::optional<BTFError>
  detectEndianness(bool &little_endian, IFileReader &file_reader) noexcept;

  static Result<BTFHeader, BTFError>
  readBTFHeader(IFileReader &file_reader) noexcept;

  static Result<BTFTypeList, BTFError>
  parseTypeSection(const BTFHeader &btf_header,
                   IFileReader &file_reader) noexcept;

  static Result<BTFTypeHeader, BTFError>
  parseTypeHeader(IFileReader &file_reader) noexcept;

  static Result<BTFType, BTFError>
  parseIntData(const BTFHeader &btf_header,
               const BTFTypeHeader &btf_type_header,
               IFileReader &file_reader) noexcept;

  static Result<BTFType, BTFError>
  parsePtrData(const BTFHeader &btf_header,
               const BTFTypeHeader &btf_type_header,
               IFileReader &file_reader) noexcept;

  static Result<BTFType, BTFError>
  parseConstData(const BTFHeader &btf_header,
                 const BTFTypeHeader &btf_type_header,
                 IFileReader &file_reader) noexcept;

  static Result<BTFType, BTFError>
  parseArrayData(const BTFHeader &btf_header,
                 const BTFTypeHeader &btf_type_header,
                 IFileReader &file_reader) noexcept;

  static Result<BTFType, BTFError>
  parseTypedefData(const BTFHeader &btf_header,
                   const BTFTypeHeader &btf_type_header,
                   IFileReader &file_reader) noexcept;

  static Result<BTFType, BTFError>
  parseEnumData(const BTFHeader &btf_header,
                const BTFTypeHeader &btf_type_header,
                IFileReader &file_reader) noexcept;

  static Result<BTFType, BTFError>
  parseFuncProtoData(const BTFHeader &btf_header,
                     const BTFTypeHeader &btf_type_header,
                     IFileReader &file_reader) noexcept;

  static Result<BTFType, BTFError>
  parseVolatileData(const BTFHeader &btf_header,
                    const BTFTypeHeader &btf_type_header,
                    IFileReader &file_reader) noexcept;

  static Result<BTFType, BTFError>
  parseStructData(const BTFHeader &btf_header,
                  const BTFTypeHeader &btf_type_header,
                  IFileReader &file_reader) noexcept;

  static Result<BTFType, BTFError>
  parseUnionData(const BTFHeader &btf_header,
                 const BTFTypeHeader &btf_type_header,
                 IFileReader &file_reader) noexcept;

  static Result<BTFType, BTFError>
  parseFwdData(const BTFHeader &btf_header,
               const BTFTypeHeader &btf_type_header,
               IFileReader &file_reader) noexcept;

  static Result<BTFType, BTFError>
  parseFuncData(const BTFHeader &btf_header,
                const BTFTypeHeader &btf_type_header,
                IFileReader &file_reader) noexcept;

  static Result<BTFType, BTFError>
  parseFloatData(const BTFHeader &btf_header,
                 const BTFTypeHeader &btf_type_header,
                 IFileReader &file_reader) noexcept;

  static Result<std::string, BTFError>
  parseString(IFileReader &file_reader, std::uint64_t offset) noexcept;

  friend class IBTF;
};

} // namespace btfparse
