//
// Copyright (c) 2021-present, Trail of Bits, Inc.
// All rights reserved.
//
// This source code is licensed in accordance with the terms specified in
// the LICENSE file found in the root directory of this source tree.
//

#include <btfparse/istream.h>

#include <cstdint>
#include <filesystem>
#include <fstream>

namespace btfparse {

class FstreamAdapter final : public IStream {
  mutable std::ifstream input_stream;

public:
  static Ptr create(const std::filesystem::path &path);
  virtual ~FstreamAdapter() override;

  virtual bool seek(std::uint64_t offset) override;
  virtual std::uint64_t offset() const override;
  virtual bool read(std::uint8_t *buffer, std::size_t size) override;

private:
  FstreamAdapter(const std::filesystem::path &path);
};

} // namespace btfparse
