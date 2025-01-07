# ------------------------------
# Headers and sources

core_headers = files(
   'api.hpp'
  ,'apidep_standard.hpp'
  ,'apidep_substrait.hpp'
  ,'plans.hpp'
  ,'operators.hpp'
)

libmohair_headers += core_headers
libmohair_sources += files(
   'plans.cpp'
  ,'operators.cpp'
)


# ------------------------------
# Installation

install_headers(core_headers
  ,subdir       : 'skytether' / 'mohair'
  ,preserve_path: true
)
