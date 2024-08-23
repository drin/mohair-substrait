#!/bin/bash

# [[ $(is_mac) -eq 1 ]] means we are on a mac
function is_mac() {
  if [[ 'Darwin' == $(uname -s) ]]; then
    echo 1

  else
    echo 0

  fi
}


# Logic to change include filepaths for generated sources
function change_namespace() {
  old_ns="${1:?"Required: existing namespace to modify"}"
  new_ns="${2:?"Required: new namespace prefix to use"}"
  target_dirpath="${3:?"Required: directory path to search"}"

  re_replace_ns="s%include \"${old_ns}/%include \"${new_ns}/%"
  src_fpaths=($(grep -Rl 'include "substrait' "${target_dirpath}"))

  # Mac syntax for sed (`-i` requires some additional argument)
  if [[ $(is_mac) -eq 1 ]]; then
    sed -i '' "${re_replace_ns}" "${src_fpaths[@]}"

  # Linux syntax for sed
  else
    sed -i "${re_replace_ns}" "${src_fpaths[@]}"

  fi
}


# Take input from the user
srcgen_dirpath="${1:?"Please provide a directory path containing generated source files"}"
generated_ns="substrait"
target_ns="mohair-substrait/substrait"

# Call the change_namespace function with some hardcoded values
change_namespace "${generated_ns}" "${target_ns}" "${srcgen_dirpath}"
