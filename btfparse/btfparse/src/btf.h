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

struct BTFFile final {
  BTFHeader btf_header;
  IFileReader::Ptr file_reader;
};

using BTFFileList = std::vector<BTFFile>;

using BTFTypeParser = Result<BTFType, BTFError> (*)(const BTFFileList &,
                                                    const BTFTypeHeader &,
                                                    IFileReader &);

class BTF final : public IBTF {
public:
  virtual ~BTF() override;

  virtual std::optional<BTFType>
  getType(std::uint32_t id) const noexcept override;

  virtual std::optional<BTFKind>
  getKind(std::uint32_t id) const noexcept override;

  virtual std::uint32_t count() const noexcept override;
  virtual BTFTypeMap getAll() const noexcept override;

private:
  struct PrivateData;
  std::unique_ptr<PrivateData> d;

  BTF(const PathList &path_list);

public:
  static BTFError convertFileReaderError(const FileReaderError &error) noexcept;

  static std::optional<BTFError>
  detectEndianness(bool &little_endian, IFileReader &file_reader) noexcept;

  static Result<BTFHeader, BTFError>
  readBTFHeader(IFileReader &file_reader) noexcept;

  static Result<BTFTypeMap, BTFError>
  parseTypeSections(const BTFFileList &btf_file_list) noexcept;

  static Result<BTFTypeHeader, BTFError>
  parseTypeHeader(IFileReader &file_reader) noexcept;

  static Result<BTFType, BTFError>
  parseIntData(const BTFFileList &btf_file_list,
               const BTFTypeHeader &btf_type_header,
               IFileReader &file_reader) noexcept;

  static Result<BTFType, BTFError>
  parsePtrData(const BTFFileList &, const BTFTypeHeader &btf_type_header,
               IFileReader &file_reader) noexcept;

  static Result<BTFType, BTFError>
  parseConstData(const BTFFileList &, const BTFTypeHeader &btf_type_header,
                 IFileReader &file_reader) noexcept;

  static Result<BTFType, BTFError>
  parseArrayData(const BTFFileList &, const BTFTypeHeader &btf_type_header,
                 IFileReader &file_reader) noexcept;

  static Result<BTFType, BTFError>
  parseTypedefData(const BTFFileList &btf_file_list,
                   const BTFTypeHeader &btf_type_header,
                   IFileReader &file_reader) noexcept;

  static Result<BTFType, BTFError>
  parseEnumData(const BTFFileList &btf_file_list,
                const BTFTypeHeader &btf_type_header,
                IFileReader &file_reader) noexcept;

  static Result<BTFType, BTFError>
  parseFuncProtoData(const BTFFileList &btf_file_list,
                     const BTFTypeHeader &btf_type_header,
                     IFileReader &file_reader) noexcept;

  static Result<BTFType, BTFError>
  parseVolatileData(const BTFFileList &, const BTFTypeHeader &btf_type_header,
                    IFileReader &file_reader) noexcept;

  static Result<BTFType, BTFError>
  parseStructData(const BTFFileList &btf_file_list,
                  const BTFTypeHeader &btf_type_header,
                  IFileReader &file_reader) noexcept;

  static Result<BTFType, BTFError>
  parseUnionData(const BTFFileList &btf_file_list,
                 const BTFTypeHeader &btf_type_header,
                 IFileReader &file_reader) noexcept;

  static Result<BTFType, BTFError>
  parseFwdData(const BTFFileList &btf_file_list,
               const BTFTypeHeader &btf_type_header,
               IFileReader &file_reader) noexcept;

  static Result<BTFType, BTFError>
  parseFuncData(const BTFFileList &btf_file_list,
                const BTFTypeHeader &btf_type_header,
                IFileReader &file_reader) noexcept;

  static Result<BTFType, BTFError>
  parseFloatData(const BTFFileList &btf_file_list,
                 const BTFTypeHeader &btf_type_header,
                 IFileReader &file_reader) noexcept;

  static Result<BTFType, BTFError>
  parseRestrictData(const BTFFileList &, const BTFTypeHeader &btf_type_header,
                    IFileReader &file_reader) noexcept;

  static Result<BTFType, BTFError>
  parseVarData(const BTFFileList &btf_file_list,
               const BTFTypeHeader &btf_type_header,
               IFileReader &file_reader) noexcept;

  static Result<BTFType, BTFError>
  parseDataSecData(const BTFFileList &btf_file_list,
                   const BTFTypeHeader &btf_type_header,
                   IFileReader &file_reader) noexcept;

  static Result<std::string, BTFError>
  parseString(const BTFFileList &btf_file_list, std::uint64_t offset) noexcept;

  static Result<std::string, BTFError>
  parseString(IFileReader &file_reader, std::uint64_t offset) noexcept;

  friend class IBTF;
};

} // namespace btfparse
