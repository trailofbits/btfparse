#
# Copyright (c) 2021-present, Trail of Bits, Inc.
# All rights reserved.
#
# This source code is licensed in accordance with the terms specified in
# the LICENSE file found in the root directory of this source tree.
#

function(generateSettingsTargets)
  add_library(btfparse_common_target_settings INTERFACE)
  target_compile_options(btfparse_common_target_settings INTERFACE
    -Wall
    -pedantic
    -Wconversion
    -Wunused
    -Wshadow
    -fvisibility=hidden
    -Werror
    -Wno-deprecated-declarations
  )

  set_target_properties(btfparse_common_target_settings PROPERTIES
    INTERFACE_POSITION_INDEPENDENT_CODE ON
  )

  if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    list(APPEND compile_option_list -O0)
    list(APPEND compile_definition_list DEBUG)

  else()
    list(APPEND compile_option_list -O2)
    list(APPEND compile_definition_list NDEBUG)
  endif()

  if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug" OR
     "${CMAKE_BUILD_TYPE}" STREQUAL "RelWithDebInfo")

    list(APPEND compile_option_list -g3)
  else()
    list(APPEND compile_option_list -g0)
  endif()

  target_compile_options(btfparse_common_target_settings INTERFACE
    ${compile_option_list}
  )

  target_compile_definitions(btfparse_common_target_settings INTERFACE
    ${compile_definition_list}
  )

  add_library(btfparse_cxx_target_settings INTERFACE)
  target_compile_features(btfparse_cxx_target_settings INTERFACE cxx_std_17)
  target_link_libraries(btfparse_cxx_target_settings INTERFACE
    btfparse_common_target_settings
  )

  add_library(btfparse_c_target_settings INTERFACE)
  target_link_libraries(btfparse_c_target_settings INTERFACE
    btfparse_common_target_settings
  )
endfunction()
