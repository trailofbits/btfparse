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
            << "\tdump-btf /sys/kernel/btf/vmlinux\n"
            << "\tdump-btf /sys/kernel/btf/vmlinux [/sys/kernel/btf/btusb]\n";
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
    std::cout << "No types were found!\n";
    return 1;
  }

  for (const auto &btf_type_map_p : btf->getAll()) {
    const auto &id = btf_type_map_p.first;
    const auto &btf_type = btf_type_map_p.second;

    std::cout << "[" << id << "] " << btfparse::IBTF::getBTFTypeKind(btf_type)
              << " " << btf_type << "\n";
  }

  return 0;
}
