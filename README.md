# Overview

This repository depends on two external repositories as submodules:
  1. [Substrait][repo-substrait] -- This defines the main substrait protocol and is my
     fork of the [main Substrait repo][repo-substrait-io].
  2. [Mohair-protocol][repo-mohair] -- This defines my own extensions to substrait for
     cooperative query decomposition.

The purpose of this repository is to build a library, `libmohair-substrait`. Projects can
link against this library to access my interface to accessing and manipulating substrait
plans. Additionally, the build for this library is defined such that any number of
libraries that link against it can also link against each other without running into
duplicate protobuf descriptor issues or other similar issues.

# Organization

The namespace for symbols in this library is prefixed with `skytether`, e.g.
`::skytether::substrait::Plan` or `::skytether::mohair::SkyRel`. This is to address issues
where a library may transitively link against this library and transitively link against
other generated substrait sources (e.g. provided by `libarrow-substrait`). Although, this
means that substrait sources provided by other libraries also exist in some other
namespace, `::substrait` by default (e.g. `::substrait::Plan`).


# Usage

The most straight-forward way to use this repository is to simply use the homebrew formula
I have written: [drin/hatchery/mohair-substrait.rb][brew-mohair-substrait]. Instructions
for tapping my cask or simply installing the formula are provided in the
[README.md][readme-hatchery] in that repo:
```bash
# To install a formula directly
brew install drin/hatchery/<formula>

# To tap the cask and then install the formula
brew tap drin/hatchery
brew install <formula>
```

Otherwise, the formula itself should provide a fairly readable form of the build
instructions:
```bash
build_dirname=build-dir

meson setup "${build_dirname}"

# Optionally, change build options
# meson configure -Dprefix=<option> <...> "${build_dirname}"

meson compile -C "${build_dirname}"

# Optionally, install
# meson install -C "${build_dirname}"
```

Installed header files should be available in `${include_dir}/skytether-mohair`, where
`include_dir` is something like `/opt/homebrew/include` or wherever you set your prefix
to.


<!-- resources -->
[repo-substrait]:        https://github.com/drin/substrait
[repo-mohair]:           https://github.com/drin/mohair-protocol

[repo-substrait-io]:     https://github.com/substrait-io/substrait

[brew-mohair-substrait]: https://github.com/drin/homebrew-hatchery/blob/mainline/Formula/mohair-substrait.rb

[readme-hatchery]:       https://github.com/drin/homebrew-hatchery/blob/mainline/README.md
