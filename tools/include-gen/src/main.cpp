//
// Copyright (c) 2021-present, Trail of Bits, Inc.
// All rights reserved.
//
// This source code is licensed in accordance with the terms specified in
// the LICENSE file found in the root directory of this source tree.
//

#include <algorithm>
#include <cstring>
#include <iostream>
#include <sstream>

#include <btfparse/ibtfheadergenerator.h>

namespace {

void showHelp() {
  std::cerr
      << "Usage:\n"
      << "\tinclude-gen /sys/kernel/btf/vmlinux\n"
      << "\tinclude-gen /sys/kernel/btf/vmlinux [/sys/kernel/btf/btusb]\n";
}

} // namespace

int main(int argc, char *argv[]) {
  if (argc <= 1 || std::strcmp(argv[1], "--help") == 0) {
    showHelp();
    return 0;
  }

  std::vector<std::filesystem::path> path_list;
  for (int i = 1; i < argc; ++i) {
    const char *input_path = argv[i];
    path_list.emplace_back(input_path);
  }

  auto btf_res = btfparse::IBTF::createFromPathList(path_list);
  if (btf_res.failed()) {
    std::cerr << "Failed to open the BTF file: " << btf_res.takeError() << "\n";
    return 1;
  }

  auto btf = btf_res.takeValue();
  if (btf->count() == 0) {
    std::cerr << "No types were found!\n";
    return 1;
  }

  auto header_generator = btfparse::IBTFHeaderGenerator::create();

  std::string header;
  if (!header_generator->generate(header, btf)) {
    std::cerr << "Failed to generate the header\n";
    return 1;
  }

  std::cout << header << "\n";
  return 0;
}
