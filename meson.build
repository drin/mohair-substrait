# ------------------------------
# Project Definition

# >> configuration
project('libmohair-substrait'
  ,'cpp'
  ,version         : '0.1.3'
  ,default_options : ['warning_level=3','cpp_std=c++17']
)


# ------------------------------
# Build variables (convenience)

# >> Path strings
# base directory for C++ sources
cpp_srcdir    = 'src' / 'cpp'

# dir for generated protobuf code
cpp_proto_substrait = cpp_srcdir / 'substrait'
cpp_proto_mohair    = cpp_srcdir / 'mohair'

# dir for binary sources (1-1 with binaries)
cpp_tooldir   = cpp_srcdir / 'toolbox'


# >> Include dir objects
inc_mohairsubstrait = include_directories(cpp_srcdir)


# ------------------------------
# Meson dependencies (modules)

# use pkg-config to generate library build info
module_pkgcfg = import('pkgconfig')
module_cmake  = import('cmake')


# >> Required dependencies

# Protobuf 
dep_proto  = dependency('protobuf')


# >> Make configuration data available to source files
version_str    = meson.project_version()
version_fields = version_str.split('.')
is_debug       = get_option('buildtype') == 'debug' ? 1 : 0

mohair_cfgdata = configuration_data({
   'VERSION_STR'      : meson.project_version()
  ,'VERSION_MAJOR'    : version_fields[0]
  ,'VERSION_MINOR'    : version_fields[1]
  ,'VERSION_PATCH'    : version_fields[2]
  ,'BUILD_IS_DEBUG'   : is_debug
  ,'SUBSTRAIT_VERSION': get_option('substrait_version')
  ,'MOHAIR_VERSION'   : get_option('mohair_version')
})

mohair_cfgfile = configure_file(
   input        : cpp_srcdir / 'mohair-substrait-config.hpp.in'
  ,output       : 'mohair-substrait-config.hpp'
  ,configuration: mohair_cfgdata
)


# ------------------------------
# Composable lists of headers

# >> For internal implementations for the library
adapter_hdrlist = [
   cpp_srcdir / 'mohair_substrait.hpp'
  ,cpp_srcdir / 'apidep_standard.hpp'
  ,cpp_srcdir / 'apidep_substrait.hpp'
]

# >> For generated substrait and mohair definitions
substrait_hdrlist = [
   cpp_proto_substrait / 'type.pb.h'
  ,cpp_proto_substrait / 'function.pb.h'
  ,cpp_proto_substrait / 'plan.pb.h'
  ,cpp_proto_substrait / 'algebra.pb.h'
  ,cpp_proto_substrait / 'capabilities.pb.h'
  ,cpp_proto_substrait / 'parameterized_types.pb.h'
  ,cpp_proto_substrait / 'type_expressions.pb.h'
  ,cpp_proto_substrait / 'extended_expression.pb.h'
  ,cpp_proto_substrait / 'extensions' / 'extensions.pb.h'
]

mohair_hdrlist = [
   cpp_proto_mohair    / 'algebra.pb.h'
  ,cpp_proto_mohair    / 'topology.pb.h'
]


# ------------------------------
# Composable lists of sources

# >> For internal implementations for the library
adapter_srclist = [
  cpp_srcdir / 'mohair_substrait.cpp'
]

# >> For generated substrait and mohair implementations
protocol_srclist = [
   cpp_proto_mohair    / 'algebra.pb.cc'
  ,cpp_proto_mohair    / 'topology.pb.cc'
  ,cpp_proto_substrait / 'type.pb.cc'
  ,cpp_proto_substrait / 'function.pb.cc'
  ,cpp_proto_substrait / 'plan.pb.cc'
  ,cpp_proto_substrait / 'algebra.pb.cc'
  ,cpp_proto_substrait / 'capabilities.pb.cc'
  ,cpp_proto_substrait / 'parameterized_types.pb.cc'
  ,cpp_proto_substrait / 'type_expressions.pb.cc'
  ,cpp_proto_substrait / 'extended_expression.pb.cc'
  ,cpp_proto_substrait / 'extensions' / 'extensions.pb.cc'
]


# ------------------------------
# Composed header and source lists (organized by library/binary)

# >> Library for mohair substrait interface
library_hdrlist = adapter_hdrlist + substrait_hdrlist + mohair_hdrlist
library_srclist = adapter_srclist + protocol_srclist


# ------------------------------
# Library definitions

lib_mohairsubstrait = library('mohair-substrait'
  ,library_srclist
  ,include_directories: inc_mohairsubstrait
  ,dependencies       : dep_proto
  ,install            : true
)

# Generate pkgconfig and cmake config files
module_pkgcfg.generate(lib_mohairsubstrait)
module_cmake.write_basic_package_version_file(
   name   : 'MohairSubstrait'
  ,version: meson.project_version()
)
module_cmake.configure_package_config_file(
   name         : 'MohairSubstrait'
  ,input        : 'MohairSubstraitConfig.cmake.in'
  ,configuration: mohair_cfgdata
)


# ------------------------------
# Installation

install_headers(adapter_hdrlist  , subdir: 'mohair-substrait')
install_headers(substrait_hdrlist, subdir: 'mohair-substrait' / 'substrait')
install_headers(mohair_hdrlist   , subdir: 'mohair-substrait' / 'mohair')

dep_mohairsubstrait = declare_dependency(
   include_directories: inc_mohairsubstrait
  ,dependencies       : [dep_proto]
  ,link_with          : lib_mohairsubstrait
)


# ------------------------------
# Binaries

# >> Executables we always create

#   |> simple reader for Arrow files
bin_readsubstrait_srclist = [ cpp_tooldir  / 'read-substrait.cpp' ]

bin_readsubstrait = executable('read-substrait'
  ,bin_readsubstrait_srclist
  # ,include_directories: inc_mohairsubstrait
  ,dependencies       : [ dep_mohairsubstrait, dep_proto ]
  ,install            : true
)


# ------------------------------
# Build tests

test_readsubstrait_fpath = 'resources' / 'test' / 'test-subplan.substrait'
test('Read sample substrait plan'
  ,bin_readsubstrait
  ,args: [test_readsubstrait_fpath, '>test-readsubstrait.out']
)
