# ------------------------------
# Headers and sources

analysis_headers = files(
   'schema_resolution.hpp'
  ,'expressions.hpp'
  ,'rel_common.hpp'
  ,'project_rel.hpp'
  ,'aggregate_rel.hpp'
  ,'join_rel.hpp'
)

libmohair_headers += analysis_headers
libmohair_sources += files(
   'expressions.cpp'
  ,'rel_common.cpp'
  ,'project_rel.cpp'
  ,'aggregate_rel.cpp'
  ,'join_rel.cpp'
)


# ------------------------------
# Installation

install_headers(analysis_headers
  ,subdir       : 'skytether' / 'mohair' / 'analysis'
  ,preserve_path: true
)
