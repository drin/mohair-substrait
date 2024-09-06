# ------------------------------
# Project attributes
cmake_minimum_required(VERSION 3.5)

# Convenience variables
set(MOHAIR_EXPORT_SET "MohairSubstraitExports")

# Project definition
project(MohairSubstrait)

# Project configuration
set(CMAKE_CXX_STANDARD              17)
set(CMAKE_CXX_STANDARD_REQUIRED     ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(VERSION_STR    "0.1.3")
set(VERSION_MAJOR  "0")
set(VERSION_MINOR  "1")
set(VERSION_PATCH  "3")
# set(BUILD_IS_DEBUG $<CONFIG:Debug>)
set(BUILD_IS_DEBUG 0)

configure_file(${CMAKE_CURRENT_LIST_DIR}/src/cpp/mohair-substrait-config.hpp.in
               ${CMAKE_CURRENT_LIST_DIR}/src/cpp/mohair-substrait-config.hpp)



# ------------------------------
# Locate Dependencies

# >> Third-party

# Protobuf
find_package(Protobuf REQUIRED)

# conditionally, depend on absl
if (Protobuf_VERSION VERSION_GREATER_EQUAL 4)
  find_package(absl REQUIRED)
endif()


# library for mohair-substrait
find_library(MohairSubstrait_LIBRARIES libmohair-substrait.a REQUIRED)

# headers for mohair-substrait
find_path(MohairSubstrait_INCLUDE_DIRS mohair-substrait REQUIRED)


# ------------------------------
# Source files

# Adapter and substrait protocol sources
set(ADAPTER_SOURCES  src/cpp/mohair_substrait.cpp)
set(PROTOCOL_SOURCES src/cpp/mohair-substrait/mohair/algebra.pb.cc
                     src/cpp/mohair-substrait/mohair/topology.pb.cc
                     src/cpp/mohair-substrait/substrait/type.pb.cc
                     src/cpp/mohair-substrait/substrait/function.pb.cc
                     src/cpp/mohair-substrait/substrait/plan.pb.cc
                     src/cpp/mohair-substrait/substrait/algebra.pb.cc
                     src/cpp/mohair-substrait/substrait/capabilities.pb.cc
                     src/cpp/mohair-substrait/substrait/parameterized_types.pb.cc
                     src/cpp/mohair-substrait/substrait/type_expressions.pb.cc
                     src/cpp/mohair-substrait/substrait/extended_expression.pb.cc
                     src/cpp/mohair-substrait/substrait/extensions/extensions.pb.cc)

# Executable sources
set(TOOL_READSUB_SOURCES src/cpp/toolbox/read-substrait.cpp)


# ------------------------------
# Library targets

# >> The mohair-substrait library that we want to produce
set(PROJ_LIB_NAME mohair-substrait)
add_library(${PROJ_LIB_NAME} STATIC
            ${ADAPTER_SOURCES}
            ${PROTOCOL_SOURCES})

set_target_properties(${PROJ_LIB_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)


# ------------------------------
# Library dependencies and properties

# >> mohair-substrait library

# link the protobuf library (the framework itself)
target_link_libraries(${PROJ_LIB_NAME} PUBLIC protobuf::libprotobuf)

# conditionally, link against absl
if (Protobuf_VERSION VERSION_GREATER_EQUAL 4)
  # this is needed for generated protobuf libs that use ABSL_DCHECK*()
  target_link_libraries(${PROJ_LIB_NAME} PUBLIC absl::status
                                            absl::log_internal_check_op)
endif()


# ------------------------------
# Library headers

# >> Internal headers (in-project) common across library targets
include_directories(src/cpp)

# >> External headers
target_include_directories(${PROJ_LIB_NAME} PUBLIC ${Protobuf_INCLUDE_DIRS})


# ------------------------------
# Build artifacts

install(
  TARGETS ${PROJ_LIB_NAME}
  EXPORT  ${MOHAIR_EXPORT_SET}
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/mohair-substrait
  LIBRARY       DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE       DESTINATION ${CMAKE_INSTALL_LIBDIR})

