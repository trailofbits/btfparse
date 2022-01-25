//
// Copyright (c) 2021-present, Trail of Bits, Inc.
// All rights reserved.
//
// This source code is licensed in accordance with the terms specified in
// the LICENSE file found in the root directory of this source tree.
//

#include <doctest/doctest.h>

#include <btfparse/result.h>

#include <array>
#include <string>

namespace btfparse {

namespace {

enum class InitializationType {
  Default,
  Value,
  Error,
};

static const std::array<InitializationType, 3> kInitializationType{
    InitializationType::Default, InitializationType::Value,
    InitializationType::Error};

enum class GetterTestType {
  Error,
  Value,
};

static const std::array<GetterTestType, 2> kGetterTestType{
    GetterTestType::Error,
    GetterTestType::Value,
};

using ResultType = Result<std::string, int, true>;

ResultType createResult(InitializationType init_type) {
  switch (init_type) {
  case InitializationType::Default:
    return ResultType();

  case InitializationType::Value:
    return std::string();

  case InitializationType::Error:
    return 1;

  default:
    __builtin_unreachable();
  }
}

} // namespace

TEST_SUITE("Result class") {
  TEST_CASE("Simple move") {
    auto result = createResult(InitializationType::Default);

    {
      auto temp = createResult(InitializationType::Value);
      result = std::move(temp);
    }

    auto check_succeeded = !result.failed();
    CHECK(check_succeeded);

    { auto temp = std::move(result); }

    check_succeeded = false;

    try {
      result.failed();
    } catch (const std::exception &) {
      check_succeeded = true;
    }

    CHECK(check_succeeded);
  }

  TEST_CASE("Value and error getters") {
    for (const auto &init_type : kInitializationType) {
      auto result = createResult(init_type);
      bool check_succeeded{false};

      if (init_type == InitializationType::Default) {
        try {
          result.failed();
        } catch (const std::exception &) {
          check_succeeded = true;
        }

      } else {
        check_succeeded =
            (init_type == InitializationType::Value && !result.failed()) ||
            (init_type == InitializationType::Error && result.failed());
      }

      CHECK(check_succeeded);

      for (const auto &getter_test_type : kGetterTestType) {
        for (auto take : {false, true}) {
          check_succeeded = false;

          try {
            switch (getter_test_type) {
            case GetterTestType::Error:
              if (take) {
                result.takeError();
              } else {
                result.error();
              }

              check_succeeded = init_type == InitializationType::Error;
              break;

            case GetterTestType::Value:
              if (take) {
                result.takeValue();
              } else {
                result.value();
              }

              check_succeeded = init_type == InitializationType::Value;
              break;

            default:
              __builtin_unreachable();
            }

          } catch (const std::exception &) {
            // clang-format off
            check_succeeded = (init_type == InitializationType::Default) ||
                              (init_type == InitializationType::Error && getter_test_type != GetterTestType::Error) ||
                              (init_type == InitializationType::Value && getter_test_type != GetterTestType::Value);
            // clang-format on
          }

          CHECK(check_succeeded);
        }
      }
    }
  }
}

} // namespace btfparse
