# ------------------------------
# Headers and sources

helper_headers = files(
   'helper_prototypes.hpp'
)

libmohair_headers += helper_headers
libmohair_sources += files(
   'structural.cpp'
  ,'traversal.cpp'
)


# ------------------------------
# Installation

install_headers(helper_headers
  ,subdir       : 'skytether' / 'mohair' / 'adapters' / 'helpers'
  ,preserve_path: true
)
