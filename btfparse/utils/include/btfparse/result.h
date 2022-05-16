//
// Copyright (c) 2021-present, Trail of Bits, Inc.
// All rights reserved.
//
// This source code is licensed in accordance with the terms specified in
// the LICENSE file found in the root directory of this source tree.
//

#pragma once

#include <iostream>
#include <variant>
#include <utility>

#ifdef __linux__
#include <csignal>
#else
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

namespace btfparse {

template <typename Value, typename Error, bool use_exceptions = false>
class Result final {
  mutable bool checked{false};
  std::variant<std::monostate, Value, Error> data;

public:
  Result() = default;

  Result(const Value &value) : data(value) {}
  Result(const Error &error) : data(error) {}

  Result(Value &&value) noexcept : data(std::move(value)) {}
  Result(Error &&error) noexcept : data(std::move(error)) {}

  ~Result() {
    if (std::holds_alternative<std::monostate>(data)) {
      return;
    }

    if (!checked) {
      raiseError(ErrorCode::NotChecked);
    }
  }

  bool failed() const {
    verifyInitialized();

    checked = true;
    return std::holds_alternative<Error>(data);
  }

  const Value &value() const {
    verifyAccess(true);
    return std::get<Value>(data);
  }

  const Error &error() const {
    verifyAccess(false);
    return std::get<Error>(data);
  }

  Value takeValue() { return take<Value>(); }
  Error takeError() { return take<Error>(); }

  Result(Result &&other) noexcept {
    data = std::exchange(other.data, std::monostate());
    checked = std::exchange(other.checked, false);
  }

  Result &operator=(Result &&other) noexcept {
    if (this != &other) {
      data = std::exchange(other.data, std::monostate());
      checked = std::exchange(other.checked, false);
    }

    return *this;
  }

  Result(const Result &) = delete;
  Result &operator=(const Result &) = delete;

private:
  enum class ErrorCode {
    NotInitialized,
    NotChecked,
    NotAnError,
    NotAValue,
  };

  void raiseError(ErrorCode error_code) const {
    const char *message{nullptr};
    switch (error_code) {
    case ErrorCode::NotInitialized:
      message = "This Result object was not initialized";
      break;

    case ErrorCode::NotChecked:
      message = "This Result object was not checked for failure";
      break;

    case ErrorCode::NotAnError:
      message = "This Result object does not contain an error";
      break;

    case ErrorCode::NotAValue:
      message = "This Result object does not contain a value";
      break;
    }

    if (use_exceptions) {
      throw std::logic_error(message);

    } else {
      std::cerr << message << "\n";

#ifdef __linux__
      raise(SIGTRAP);
#else
      DebugBreak();
#endif
    }
  }

  void verifyInitialized() const {
    if (std::holds_alternative<std::monostate>(data)) {
      raiseError(ErrorCode::NotInitialized);
    }
  }

  void verifyAccess(bool as_value) const {
    verifyInitialized();

    if (as_value) {
      if (!checked) {
        raiseError(ErrorCode::NotChecked);
      }

      if (failed()) {
        raiseError(ErrorCode::NotAValue);
      }

    } else {
      if (!checked) {
        raiseError(ErrorCode::NotChecked);
      }

      if (!failed()) {
        raiseError(ErrorCode::NotAnError);
      }
    }
  }

  template <typename Type> Type take() {
    static_assert(std::is_same_v<Type, Value> || std::is_same_v<Type, Error>,
                  "Invalid type passed to the Result<Type>::take() method");

    verifyAccess(std::is_same_v<Type, Value>);
    auto output = std::move(std::get<Type>(data));

    checked = false;
    data = std::monostate();

    return output;
  }
};

} // namespace btfparse
