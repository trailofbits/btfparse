//
// Copyright (c) 2021-present, Trail of Bits, Inc.
// All rights reserved.
//
// This source code is licensed in accordance with the terms specified in
// the LICENSE file found in the root directory of this source tree.
//

#include "btf.h"

#include <sstream>
#include <unordered_map>

#include <btfparse/ibtf.h>

namespace btfparse {

namespace {

const std::unordered_map<BTFErrorInformation::Code, std::string>
    kErrorTranslationMap = {
        {BTFErrorInformation::Code::Unknown, "Unknown error"},

        {BTFErrorInformation::Code::MemoryAllocationFailure,
         "Memory allocation failure"},

        {BTFErrorInformation::Code::FileNotFound, "File not found"},
        {BTFErrorInformation::Code::IOError, "IO error"},
        {BTFErrorInformation::Code::InvalidMagicValue, "Invalid magic value"},
        {BTFErrorInformation::Code::InvalidBTFKind, "Invalid BTF kind"},
        {BTFErrorInformation::Code::UnsupportedBTFKind, "Unsupported BTF kind"},

        {BTFErrorInformation::Code::InvalidIntBTFTypeEncoding,
         "Invalid encoding for `Int` BTFType"},

        {BTFErrorInformation::Code::InvalidPtrBTFTypeEncoding,
         "Invalid encoding for `Ptr` BTFType"},

        {BTFErrorInformation::Code::InvalidArrayBTFTypeEncoding,
         "Invalid encoding for `Array` BTFType"},

        {BTFErrorInformation::Code::InvalidTypedefBTFTypeEncoding,
         "Invalid encoding for `Typedef` BTFType"},

        {BTFErrorInformation::Code::InvalidEnumBTFTypeEncoding,
         "Invalid encoding for `Enum` BTFType"},

        {BTFErrorInformation::Code::InvalidFuncProtoBTFTypeEncoding,
         "Invalid encoding for `FuncProto` BTFType"},

        {BTFErrorInformation::Code::InvalidVolatileBTFTypeEncoding,
         "Invalid encoding for `Volatile` BTFType"},

        {BTFErrorInformation::Code::InvalidFwdBTFTypeEncoding,
         "Invalid encoding for `Fwd` BTFType"},

        {BTFErrorInformation::Code::InvalidFuncBTFTypeEncoding,
         "Invalid encoding for `Func` BTFType"},

        {BTFErrorInformation::Code::InvalidFloatBTFTypeEncoding,
         "Invalid encoding for `Float` BTFType"},

        {BTFErrorInformation::Code::InvalidRestrictBTFTypeEncoding,
         "Invalid encoding for `Restrict` BTFType"},

        {BTFErrorInformation::Code::InvalidVarBTFTypeEncoding,
         "Invalid encoding for `Var` BTFType"},

        {BTFErrorInformation::Code::InvalidDataSecBTFTypeEncoding,
         "Invalid encoding for `DataSec` BTFType"},

        {BTFErrorInformation::Code::InvalidStringOffset,
         "Invalid string offset"},
};

}

std::string BTFErrorInformationPrinter::operator()(
    const BTFErrorInformation &error_information) const {
  std::stringstream buffer;
  buffer << "Error: '";

  auto error_it = kErrorTranslationMap.find(error_information.code);
  if (error_it == kErrorTranslationMap.end()) {
    buffer << "Unknown error code " +
                  std::to_string(static_cast<int>(error_information.code));
  } else {
    buffer << error_it->second;
  }

  buffer << "'";

  if (error_information.opt_file_range.has_value()) {
    const auto &file_range = error_information.opt_file_range.value();
    buffer << ", File range: " << file_range.offset << " - "
           << (file_range.offset + file_range.size);
  }

  return buffer.str();
}

Result<IBTF::Ptr, BTFError>
IBTF::createFromPath(const std::filesystem::path &path) noexcept {
  return IBTF::createFromPathList({path});
}

Result<IBTF::Ptr, BTFError>
IBTF::createFromPathList(const PathList &path_list) noexcept {
  try {
    return Ptr(new BTF(path_list));

  } catch (const std::bad_alloc &) {
    return BTFError(BTFErrorInformation{
        BTFErrorInformation::Code::MemoryAllocationFailure,
    });

  } catch (const BTFError &e) {
    return e;
  }
}

BTFKind IBTF::getBTFTypeKind(const BTFType &btf_type) noexcept {
  return static_cast<BTFKind>(btf_type.index());
}

} // namespace btfparse
