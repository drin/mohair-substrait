# ------------------------------
# Project Definition

# >> project info
project('libmohair-substrait'
  ,'cpp'
  ,version         : '0.1.7'
  ,default_options : ['warning_level=3','cpp_std=c++17']
)

version_str    = meson.project_version()
version_fields = version_str.split('.')

mohair_cfgdata = configuration_data({
   'VERSION_STR'      : version_str
  ,'VERSION_MAJOR'    : version_fields[0]
  ,'VERSION_MINOR'    : version_fields[1]
  ,'VERSION_PATCH'    : version_fields[2]
  ,'BUILD_IS_DEBUG'   : get_option('buildtype') == 'debug' ? 1 : 0
  ,'SUBSTRAIT_VERSION': get_option('substrait_version')
  ,'MOHAIR_VERSION'   : get_option('mohair_version')
})


# ------------------------------
# Reference paths

cpp_srcdir  = 'src' / 'cpp'
cpp_tooldir = cpp_srcdir / 'toolbox'


# ------------------------------
# Build definitions

# >> Write configuration data
# write `mohair-substrait-config.hpp` for target libmohair_substrait
mohair_cfgfile = configure_file(
   input        : cpp_srcdir / 'mohair-substrait-config.hpp.in'
  ,output       : 'mohair-substrait-config.hpp'
  ,configuration: mohair_cfgdata
  ,install_dir  : get_option('includedir') / 'mohair-substrait'
)

# >> Invoke build from subdirectories
# create target 'libmohair_substrait' (and related targets)
subdir(cpp_srcdir)


# ------------------------------
# Package configurations

# pkg-config
module_pkgcfg = import('pkgconfig')
module_pkgcfg.generate(libmohair_substrait)

# cmake (NOTE: this is mostly a placeholder)
module_cmake = import('cmake')
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
# Build artifacts (binaries)

# >> Reader for substrait files
readsubstrait_sources = [ cpp_tooldir  / 'read-substrait.cpp' ]

readsubstrait_bin = executable('read-substrait'
  ,readsubstrait_sources
  ,dependencies : [ libmohair_substrait_dep ]
  ,install      : true
)


# ------------------------------
# Build tests

test_readsubstrait_fpath = 'resources' / 'test' / 'test-subplan.substrait'
test('Read sample substrait plan'
  ,readsubstrait_bin
  ,args: [test_readsubstrait_fpath, '>test-readsubstrait.out']
)

