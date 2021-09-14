//
// Copyright (c) 2021-present, Trail of Bits, Inc.
// All rights reserved.
//
// This source code is licensed in accordance with the terms specified in
// the LICENSE file found in the root directory of this source tree.
//

#include "btf.h"

#include <btfparse/ibtf.h>

namespace btfparse {

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
