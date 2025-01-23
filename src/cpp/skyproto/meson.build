# ------------------------------
# Reference paths

mohair_protodir    = 'mohair'
substrait_protodir = 'substrait'


# ------------------------------
# Headers and sources

protocol_headers = files(
   mohair_protodir    / 'algebra.pb.h'
  ,mohair_protodir    / 'topology.pb.h'
  ,substrait_protodir / 'type.pb.h'
  ,substrait_protodir / 'function.pb.h'
  ,substrait_protodir / 'plan.pb.h'
  ,substrait_protodir / 'algebra.pb.h'
  ,substrait_protodir / 'capabilities.pb.h'
  ,substrait_protodir / 'parameterized_types.pb.h'
  ,substrait_protodir / 'type_expressions.pb.h'
  ,substrait_protodir / 'extended_expression.pb.h'
  ,substrait_protodir / 'extensions' / 'extensions.pb.h'
)


libmohair_headers += protocol_headers
libmohair_sources += files(
   mohair_protodir    / 'algebra.pb.cc'
  ,mohair_protodir    / 'topology.pb.cc'
  ,substrait_protodir / 'type.pb.cc'
  ,substrait_protodir / 'function.pb.cc'
  ,substrait_protodir / 'plan.pb.cc'
  ,substrait_protodir / 'algebra.pb.cc'
  ,substrait_protodir / 'capabilities.pb.cc'
  ,substrait_protodir / 'parameterized_types.pb.cc'
  ,substrait_protodir / 'type_expressions.pb.cc'
  ,substrait_protodir / 'extended_expression.pb.cc'
  ,substrait_protodir / 'extensions' / 'extensions.pb.cc'
)


# ------------------------------
# Installation

install_headers(protocol_headers
  ,subdir       : 'skytether' / 'skyproto'
  ,preserve_path: true
)

# ------------------------------
# Archived

# We don't need this implementation yet, but we'll keep it in pocket for the future.
# extensions_dir = 'extensions'
# extension_headers = files(
#   extensions_dir / 'functions.hpp'
# )
# libmohair_headers += [extension_headers, protocol_headers]
# libmohair_sources += files(
#    extensions_dir     / 'functions.cpp'
#   ,extensions_dir     / 'functions_generated.cpp'
# )
# 
# install_headers(extension_headers
#   ,subdir       : 'skytether' / 'skyproto'
#   ,preserve_path: true
# )
