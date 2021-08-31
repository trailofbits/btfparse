//
// Copyright (c) 2021-present, Trail of Bits, Inc.
// All rights reserved.
//
// This source code is licensed in accordance with the terms specified in
// the LICENSE file found in the root directory of this source tree.
//

#pragma once

#include <string>

namespace btfparse {

template <typename Enum> struct DefaultErrorCodePrinter final {
  static_assert(std::is_enum<Enum>::value,
                "The template parameter must be an enum");

  std::string operator()(const Enum &enum_value) const {
    return std::to_string(static_cast<std::uint64_t>(enum_value));
  }
};

template <typename ErrorType,
          typename ErrorPrinter = DefaultErrorCodePrinter<ErrorType>>
class Error final {
  std::string string_error;
  ErrorType data;

public:
  Error(const ErrorType &error)
      : string_error(getStringError(error)), data(error) {}

  Error(ErrorType &&error) noexcept
      : string_error(getStringError(error)), data(std::move(error)) {}

  const ErrorType &get() const { return data; }

  const std::string &toString() const { return string_error; }
  operator const char *() const { return toString().c_str(); }

private:
  static std::string getStringError(const ErrorType &error) {
    ErrorPrinter error_printer;
    return error_printer(error);
  }
};

} // namespace btfparse
