# ------------------------------
# Project Definition

# >> project info
project('libmohair'
  ,'cpp'
  ,version         : '3.1.4'
  ,meson_version   : '>=1.6.0'
  ,license         : 'Apache-2.0'
  ,license_files   : 'LICENSE'
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
# Build targets

# >> Configuration data and header for project
configure_file(
   input        : cpp_srcdir / 'mohair-config.hpp.in'
  ,output       : 'mohair-config.hpp'
  ,configuration: mohair_cfgdata
  ,install_dir  : get_option('includedir') / 'skytether'
)


# >> Include targets
config_inc = include_directories('.')


# >> Targets from subdirectories
subdir(cpp_srcdir)


# ------------------------------
# Package configurations

# pkg-config
module_pkgcfg = import('pkgconfig')
module_pkgcfg.generate(libmohair, subdirs: 'skytether')


# ------------------------------
# Build artifacts (binaries)

# >> Reader for substrait files
readsubstrait_sources = [ cpp_tooldir  / 'read-substrait.cpp' ]

readsubstrait_bin = executable('read-substrait'
  ,readsubstrait_sources
  ,dependencies : [ libmohair_dep ]
  ,install      : true
)

# >> Interface to cooperative query decomposition for substrait files
decompose_sources = [ cpp_tooldir  / 'decompose-substrait.cpp' ]

executable('decompose-substrait'
  ,decompose_sources
  ,dependencies : [ libmohair_dep ]
  ,install      : true
)


# ------------------------------
# Build tests

test_readsubstrait_fpath = 'resources' / 'test' / 'test-subplan.substrait'
test('Read sample substrait plan'
  ,readsubstrait_bin
  ,args: [test_readsubstrait_fpath, '>test-readsubstrait.out']
)

