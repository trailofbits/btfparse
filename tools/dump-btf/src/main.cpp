//
// Copyright (c) 2021-present, Trail of Bits, Inc.
// All rights reserved.
//
// This source code is licensed in accordance with the terms specified in
// the LICENSE file found in the root directory of this source tree.
//

#include "utils.h"

#include <cstring>
#include <iostream>

namespace {

void showHelp() {
  std::cerr << "Usage:\n"
            << "\tdump-btf /sys/kernel/btf/vmlinux\n";
}

} // namespace

int main(int argc, char *argv[]) {
  if (argc != 2 || std::strcmp(argv[1], "--help") == 0) {
    showHelp();
    return 0;
  }

  const char *input_path = argv[1];

  auto btf_res = btfparse::IBTF::createFromPath(input_path);
  if (btf_res.failed()) {
    std::cerr << "Failed to open the BTF file: " << btf_res.takeError() << "\n";
    return 1;
  }

  auto btf = btf_res.takeValue();
  if (btf->count() == 0) {
    std::cout << "No types were found!\n";
    return 1;
  }

  for (std::uint32_t id = 0; id < btf->count(); ++id) {
    auto type = btf->getType(id).value();

    std::cout << "[" << id << "] " << btfparse::IBTF::getBTFTypeKind(type)
              << " " << printType(*btf.get(), type) << "\n";
  }

  return 0;
}
