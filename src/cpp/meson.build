# ------------------------------
# Overview

# This builds the mohair-substrait library and by defining a meson build rooted in this
# dir, it's easy for this repository to be polyglot (other langauges can be in other
# subdirectories). Additionally, it requires fewer special cases.

# ------------------------------
# Dependencies

# >> System-specific
# for macosx
libcorefoundation_dep = dependency('CoreFoundation', required: false)


# >> Library dependencies
# Protobuf
libprotobuf_dep = dependency('protobuf', static: true)

# Abseil
# NOTE: This approach, instead of cmake modules, seems more robust
libabsl_dep = [
   dependency('absl_log_internal_check_op', static: true)
  ,dependency('absl_log_internal_message' , static: true)
  ,dependency('absl_status'               , static: true)
  ,dependency('absl_time_zone'            , static: true)
]


# ------------------------------
# Headers and sources

# >> From top-level directory
base_header = files('mohair.hpp')

libmohair_headers = [ base_header  ]
libmohair_sources = [ 'mohair.cpp' ]



# >> From subdirectories
subdir('skyproto')
subdir('mohair')


# ------------------------------
# Build targets

# >> Include targets
libmohair_inc = [
   config_inc
  ,include_directories('.')
]

# >> Library targets
libmohair = library('mohair'
  ,libmohair_sources
  ,include_directories: libmohair_inc
  ,dependencies       : [libcorefoundation_dep, libprotobuf_dep, libabsl_dep]
  ,pic                : true
  ,install            : true
)

# >> Dependency targets
libmohair_dep = declare_dependency(
   include_directories: libmohair_inc
  ,dependencies       : [libcorefoundation_dep, libprotobuf_dep, libabsl_dep]
  ,link_with          : [libmohair]
)


# ------------------------------
# Installation

# NOTE: preserve_path is useful here, and it avoids adding the `src/cpp` prefix because of
# where this build file is located (prefix of the relative path is preserved).
install_headers(base_header, subdir: 'skytether', preserve_path: true)

