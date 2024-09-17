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
libabsl_dep     = dependency('absl'
  ,static : false
  ,modules: [
      'absl::log_internal_check_op'
     ,'absl::log_internal_message'
     ,'absl::status'
     ,'absl::time_zone'
   ]
)


# ------------------------------
# Reference paths

mohair_srcdir    = 'mohair'
substrait_srcdir = 'substrait'


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
libmohair_substrait_headers = (
    [
       'mohair_substrait.hpp'
      ,'apidep_standard.hpp'
      ,'apidep_substrait.hpp'
    ]
  + proto_headers
)

libmohair_substrait_sources = (
    [ 'mohair_substrait.cpp' ]
  + proto_sources
)


# ------------------------------
# Include directories
libmohair_substrait_inc = include_directories('.')


# ------------------------------
# Build artifacts (libraries and dependencies)

libmohair_substrait = library('mohair-substrait'
  ,libmohair_substrait_sources
  ,include_directories: libmohair_substrait_inc
  ,dependencies       : [libcorefoundation_dep, libprotobuf_dep, libabsl_dep ]
  ,pic                : true
  ,install            : true
)

libmohair_substrait_dep = declare_dependency(
   include_directories: libmohair_substrait_inc
  ,dependencies       : [libcorefoundation_dep, libprotobuf_dep, libabsl_dep ]
  ,link_with          : [libmohair_substrait]
)


# ------------------------------
# Installation

# NOTE: preserve_path is useful here, and it avoids adding the `src/cpp` prefix because of
# where this build file is located (prefix of the relative path is preserved).
install_headers(
   libmohair_substrait_headers
  ,subdir       : 'mohair-substrait'
  ,preserve_path: true
)

