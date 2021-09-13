//
// Copyright (c) 2021-present, Trail of Bits, Inc.
// All rights reserved.
//
// This source code is licensed in accordance with the terms specified in
// the LICENSE file found in the root directory of this source tree.
//

#pragma once

#include <btfparse/ibtf.h>

#include <iostream>

std::ostream &operator<<(std::ostream &stream, btfparse::BTFKind kind);
std::ostream &operator<<(std::ostream &stream, const btfparse::BTFType &type);
