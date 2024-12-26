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

mohair_hdrdir = 'mohair'

proto_mohair_srcdir    = 'skyproto' / 'mohair'
proto_substrait_srcdir = 'skyproto' / 'substrait'


# ------------------------------
# Headers and sources

query_headers = [
   mohair_hdrdir / 'operators.hpp'
  ,mohair_hdrdir / 'plans.hpp'
]

proto_headers = [
   proto_mohair_srcdir    / 'algebra.pb.h'
  ,proto_mohair_srcdir    / 'topology.pb.h'
  ,proto_substrait_srcdir / 'type.pb.h'
  ,proto_substrait_srcdir / 'function.pb.h'
  ,proto_substrait_srcdir / 'plan.pb.h'
  ,proto_substrait_srcdir / 'algebra.pb.h'
  ,proto_substrait_srcdir / 'capabilities.pb.h'
  ,proto_substrait_srcdir / 'parameterized_types.pb.h'
  ,proto_substrait_srcdir / 'type_expressions.pb.h'
  ,proto_substrait_srcdir / 'extended_expression.pb.h'
  ,proto_substrait_srcdir / 'extensions' / 'extensions.pb.h'
]


query_sources = [
   mohair_hdrdir / 'operators.cpp'
  ,mohair_hdrdir / 'plans.cpp'
]

proto_sources = [
   proto_mohair_srcdir    / 'algebra.pb.cc'
  ,proto_mohair_srcdir    / 'topology.pb.cc'
  ,proto_substrait_srcdir / 'type.pb.cc'
  ,proto_substrait_srcdir / 'function.pb.cc'
  ,proto_substrait_srcdir / 'plan.pb.cc'
  ,proto_substrait_srcdir / 'algebra.pb.cc'
  ,proto_substrait_srcdir / 'capabilities.pb.cc'
  ,proto_substrait_srcdir / 'parameterized_types.pb.cc'
  ,proto_substrait_srcdir / 'type_expressions.pb.cc'
  ,proto_substrait_srcdir / 'extended_expression.pb.cc'
  ,proto_substrait_srcdir / 'extensions' / 'extensions.pb.cc'
]

# >> Composed lists for convenience

libmohair_headers = [
   'mohair.hpp'
  ,proto_headers
  ,mohair_hdrdir / 'apidep_standard.hpp'
  ,mohair_hdrdir / 'apidep_substrait.hpp'
  ,query_headers
]

libmohair_sources = [
   'mohair.cpp'
  ,proto_sources
  ,query_sources
]


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

