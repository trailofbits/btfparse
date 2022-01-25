#!/usr/bin/env bash

#
# Copyright (c) 2021-present, Trail of Bits, Inc.
# All rights reserved.
#
# This source code is licensed in accordance with the terms specified in
# the LICENSE file found in the root directory of this source tree.
#

VMLINUX_BTF_DATA="/sys/kernel/btf/vmlinux"

main() {
  export PATH="$(pwd):${PATH}"

  checkForRequiredDependencies || return 1
  checkForBTFSupport || return 0

  local bpf_output_path="$(mktemp)"
  bpftool btf dump file "${VMLINUX_BTF_DATA}" > "${bpf_output_path}"
  if [[ $? != 0 ]] ; then
    echo "Failed to generate the bpftool data"
    return 1
  fi

  local dumpbtf_output_path="$(mktemp)"
  dump-btf "${VMLINUX_BTF_DATA}" > "${dumpbtf_output_path}"
  if [[ $? != 0 ]] ; then
    echo "Failed to generate the dump-btf data"
    return 1
  fi

  diff -u "${bpf_output_path}" "${dumpbtf_output_path}"
  if [[ $? != 0 ]] ; then
    echo "dump-btf did not correctly generate output that matches bpftool"
    return 1
  fi

  return 0
}

checkForRequiredDependencies() {
  declare -a tool_name_list=("bpftool" \
                             "dump-btf" \
                             "diff")

  for tool_name in "${tool_name_list[@]}" ; do
    which "${tool_name}" > /dev/null 2>&1
    if [[ $? != 0 ]] ; then
      echo "The following tool is missing: ${tool_name}"
      return 1
    fi
  done

  return 0
}

checkForBTFSupport() {
  if [[ ! -f "${VMLINUX_BTF_DATA}" ]] ; then
    echo "The following kernel does not seem to have BTF support: $(uname -a)"
    return 1
  fi

  return 0
}

main $@
exit $?
