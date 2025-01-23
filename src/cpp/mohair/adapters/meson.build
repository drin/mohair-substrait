# ------------------------------
# Headers and sources

adapter_headers = files(
   'adapter_standard.hpp'
  ,'adapter_protobuf.hpp'
  ,'adapter_substrait.hpp'
)

libmohair_headers += adapter_headers
libmohair_sources += files(
   'adapter_standard.cpp'
  ,'adapter_protobuf.cpp'
)


# ------------------------------
# Installation

install_headers(adapter_headers
  ,subdir       : 'skytether' / 'mohair' / 'adapters'
  ,preserve_path: true
)
