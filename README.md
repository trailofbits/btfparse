# btfparse

btfparse is a C++ library that parses kernel debug symbols in BTF format.

# CI

[![Linux](https://github.com/trailofbits/btfparse/actions/workflows/main.yml/badge.svg)](https://github.com/trailofbits/btfparse/actions/workflows/main.yml)

# Building

## Prerequisites

 * A recent C++ compiler, supporting C++17
 * CMake >= 3.16.4

## Steps to build

**Clone the repository**

```bash
git clone https://github.com/trailofbits/btfparse
```

**Configure the project**

```bash
cmake \
  -S btfparse \
  -B btfparse-build \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DBTFPARSE_ENABLE_TOOLS=true \
  -DBTFPARSE_ENABLE_TESTS=true
```

**Build the project**

```bash
cmake \
  --build btfparse-build \
  -j $(nproc)
```

**Running the tests**

```bash
cmake \
  --build btfparse-build \
  --target test
```

# Importing btfparse in your project

This library is meant to be used as a git submodule:

1. Enter your project folder
2. Create the submodule: `git submodule add https://github.com/trailofbits/btfparse`
3. Import the library from your CMakeLists.txt file: `add_subdirectory("btfparse")`
4. Link the btfparse library against your target: `target_link_libraries(your_target PRIVATE btfparse)`

# Examples

## Tool example: dump-btf

The library comes with a **dump-btf** tool that is output-compatible with **bpftool**. In order to build it, make sure to pass `-DBTFPARSE_ENABLE_TOOLS=true` at configure time.

```bash
alessandro@ubuntu2110:~/btfparse-build$ ./tools/dump-btf/dump-btf /sys/kernel/btf/vmlinux | head
[1] INT 'long unsigned int' size=8 bits_offset=0 nr_bits=64 encoding=(none)
[2] CONST '(anon)' type_id=1
[3] ARRAY '(anon)' type_id=1 index_type_id=18 nr_elems=2
[4] PTR '(anon)' type_id=6
[5] INT 'char' size=1 bits_offset=0 nr_bits=8 encoding=(none)
[6] CONST '(anon)' type_id=5
[7] INT 'unsigned int' size=4 bits_offset=0 nr_bits=32 encoding=(none)
[8] CONST '(anon)' type_id=7
[9] INT 'signed char' size=1 bits_offset=0 nr_bits=8 encoding=(none)
[10] TYPEDEF '__u8' type_id=11
```

## Code example

```c++
#include <btfparse/ibtf.h>

bool parseTypes() {
  static const std::vector<std::filesystem::path> kPathList{
      "/sys/kernel/btf/vmlinux"};

  auto btf_res = btfparse::IBTF::createFromPathList(kPathList);
  if (btf_res.failed()) {
    std::cerr << "Failed to open the BTF file: " << btf_res.takeError() << "\n";
    return false;
  }

  auto btf = btf_res.takeValue();
  if (btf->count() == 0) {
    std::cout << "No types were found!\n";
    return false;
  }

  for (const auto &btf_type_map_p : btf->getAll()) {
    const auto &id = btf_type_map_p.first;
    const auto &btf_type = btf_type_map_p.second;

    auto type_kind = btfparse::IBTF::getBTFTypeKind(btf_type);
    if (type_kind != btfparse::BTFKind::Struct) {
      continue;
    }

    const auto &btf_struct = std::get<btfparse::StructBTFType>(btf_type);

    std::cout << std::to_string(id) << ": ";
    if (btf_struct.opt_name.has_value()) {
      std::cout << btf_struct.opt_name.value() << "\n";

    } else {
      std::cout << "unnamed\n";
    }
  }

  return true;
}
```
