# ------------------------------
# Headers and sources

core_headers = files(
   'api.hpp'
  ,'apidep_standard.hpp'
  ,'apidep_protobuf.hpp'
  ,'apidep_substrait.hpp'
  ,'operator_traits.hpp'
  ,'operator_specializations.hpp'
  ,'operators.hpp'
  ,'plans.hpp'
)

libmohair_headers += core_headers
libmohair_sources += files(
   'plans.cpp'
  ,'operators.cpp'
)


# >> From subdirectories
subdir('adapters')


# ------------------------------
# Artifacts


# ------------------------------
# Installation

install_headers(core_headers
  ,subdir       : 'skytether' / 'mohair'
  ,preserve_path: true
)
