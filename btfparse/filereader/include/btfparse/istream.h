//
// Copyright (c) 2021-present, Trail of Bits, Inc.
// All rights reserved.
//
// This source code is licensed in accordance with the terms specified in
// the LICENSE file found in the root directory of this source tree.
//

#pragma once

#include <cstdint>
#include <memory>

namespace btfparse {

class IStream {
public:
  using Ptr = std::unique_ptr<IStream>;

  IStream() = default;
  virtual ~IStream() = default;

  virtual bool seek(std::uint64_t offset) = 0;
  virtual std::uint64_t offset() const = 0;

  virtual bool read(std::uint8_t *buffer, std::size_t size) = 0;

  IStream(const IStream &) = delete;
  IStream &operator=(const IStream &) = delete;
};

} // namespace btfparse
