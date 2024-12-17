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
# Reference paths

mohair_srcdir    = 'skyproto' / 'mohair'
substrait_srcdir = 'skyproto' / 'substrait'


# ------------------------------
# Headers and sources

proto_headers = [
   mohair_srcdir    / 'algebra.pb.h'
  ,mohair_srcdir    / 'topology.pb.h'
  ,substrait_srcdir / 'type.pb.h'
  ,substrait_srcdir / 'function.pb.h'
  ,substrait_srcdir / 'plan.pb.h'
  ,substrait_srcdir / 'algebra.pb.h'
  ,substrait_srcdir / 'capabilities.pb.h'
  ,substrait_srcdir / 'parameterized_types.pb.h'
  ,substrait_srcdir / 'type_expressions.pb.h'
  ,substrait_srcdir / 'extended_expression.pb.h'
  ,substrait_srcdir / 'extensions' / 'extensions.pb.h'
]

proto_sources = [
   mohair_srcdir    / 'algebra.pb.cc'
  ,mohair_srcdir    / 'topology.pb.cc'
  ,substrait_srcdir / 'type.pb.cc'
  ,substrait_srcdir / 'function.pb.cc'
  ,substrait_srcdir / 'plan.pb.cc'
  ,substrait_srcdir / 'algebra.pb.cc'
  ,substrait_srcdir / 'capabilities.pb.cc'
  ,substrait_srcdir / 'parameterized_types.pb.cc'
  ,substrait_srcdir / 'type_expressions.pb.cc'
  ,substrait_srcdir / 'extended_expression.pb.cc'
  ,substrait_srcdir / 'extensions' / 'extensions.pb.cc'
]

# >> Composed lists for convenience
libmohair_headers = (
    [
       'mohair.hpp'
      ,'apidep_standard.hpp'
      ,'apidep_substrait.hpp'
    ]
  + proto_headers
)

libmohair_sources = (
    [ 'mohair.cpp' ]
  + proto_sources
)


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
install_headers(libmohair_headers, subdir: 'skytether', preserve_path: true)

