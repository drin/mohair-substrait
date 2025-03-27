# ------------------------------
# Headers and sources

op_headers = files(
   'op_traits.hpp'
)

libmohair_headers += op_headers
libmohair_sources += files(
   'project_rel.cpp'
  ,'filter_rel.cpp'
  ,'fetch_rel.cpp'
  ,'sort_rel.cpp'
  ,'aggregate_rel.cpp'
  ,'join_rel.cpp'
  ,'cross_rel.cpp'
  ,'hashjoin_rel.cpp'
  ,'mergejoin_rel.cpp'
  ,'extensionleaf_rel.cpp'
  ,'reference_rel.cpp'
)


# ------------------------------
# Installation

install_headers(analysis_headers
  ,subdir       : 'skytether' / 'mohair' / 'analysis'
  ,preserve_path: true
)
