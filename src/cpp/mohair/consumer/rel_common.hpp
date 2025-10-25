// ------------------------------
// License
//
// Copyright 2025 Aldrin Montana
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


// ------------------------------
// Dependencies
#pragma once

#include "mohair.hpp"
#include "mohair/plans.hpp"


// ------------------------------
// Aliases


namespace mohair {

} // namespace: mohair


// ------------------------------
// Functions

namespace mohair {

  unique_ptr<SubstraitSchema>
  SchemaFromEmit(const RelCommon& rel_common, MohairOp* op) {
    auto rel_schema = std::make_unique<SubstraitSchema>();
    if (rel_common.has_direct()) {
      rel_schema->CopyFrom(*(op->schema));
    }

    // It is possible to emit only certain attributes (like a built-in projection)
    if (rel_common.has_emit()) {
      const auto& input_types = rel_common.types();
      for (int32_t emit_ndx : rel_common.output_mapping()) {
        SubstraitType* emit_type = schema->add_types();
        emit_type->CopyFrom(rel_common.types(emit_ndx));
        schema->add_names(rel_common.names(emit_ndx));
      }
    }

    // Otherwise, we're carrying forward every attribute
    else {
      schema->CopyFrom(input_op->schema);
    }
  }

} // namespace: mohair
