//
// Copyright (c) 2021-present, Trail of Bits, Inc.
// All rights reserved.
//
// This source code is licensed in accordance with the terms specified in
// the LICENSE file found in the root directory of this source tree.
//

#include <btfparse/ibtf.h>

int main() {
  auto btf_res = btfparse::IBTF::createFromPath("/sys/kernel/btf/vmlinux");
  if (btf_res.failed()) {
    std::cerr << "Failed to open the BTF file: " << btf_res.takeError() << "\n";
    return 1;
  }

  auto btf = btf_res.takeValue();
  static_cast<void>(btf);

  std::cout << "All done!\n";
  return 0;
}
