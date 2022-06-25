//
// Copyright (c) 2021-present, Trail of Bits, Inc.
// All rights reserved.
//
// This source code is licensed in accordance with the terms specified in
// the LICENSE file found in the root directory of this source tree.
//

#pragma once

#include <btfparse/ibtf.h>

namespace btfparse {

class IBTFHeaderGenerator {
public:
  using Ptr = std::unique_ptr<IBTFHeaderGenerator>;
  static Ptr create();

  IBTFHeaderGenerator() = default;
  virtual ~IBTFHeaderGenerator() = default;

  virtual bool generate(std::string &header, const IBTF::Ptr &btf) const = 0;

  IBTFHeaderGenerator(const IBTFHeaderGenerator &) = delete;
  IBTFHeaderGenerator &operator=(const IBTFHeaderGenerator &) = delete;
};

} // namespace btfparse
