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

#include "mohair/analysis/rel_common.hpp"


// ------------------------------
// Functions

namespace mohair {

  unique_ptr<SubstraitSchema>
  SchemaFromEmit( const RelCommon&              rel_common
                 ,unique_ptr<SubstraitSchema>&& input_schema) {
    if (rel_common.emit_kind_case() != RelCommon::EmitKindCase::kEmit) {
      if (not rel_common.hint().output_names().empty()) {
        int count_aliases = rel_common.hint().output_names_size();
        for (int alias_ndx = 0; alias_ndx < count_aliases; ++alias_ndx) {
          string* alias;
          if (input_schema->names_size() <= alias_ndx) {
            alias = input_schema->add_names();
          }
          else {
            alias = input_schema->mutable_names(alias_ndx);
          }

          *alias = rel_common.hint().output_names(alias_ndx);
        }
      }

      return input_schema;
    }

    // If the operator emits "selectively", attributes are explicitly propagated
    auto emit_schema = std::make_unique<SubstraitSchema>();
    for (int32_t emit_ndx : rel_common.emit().output_mapping()) {
      SubstraitType* emit_type = emit_schema->mutable_struct_()->add_types();

      // We propagate attributes by index based on the `output_mapping` field
      emit_type->CopyFrom(input_schema->struct_().types(emit_ndx));

      // Propagate the attribute name if it has one
      if (emit_ndx < input_schema->names_size()) {
        emit_schema->add_names(input_schema->names(emit_ndx));
      }
    }

    return emit_schema;
  }

  /* TODO: if we want to emit only the attributes we need
  template <typename T>
  void SortedInsert(vector<T>& sorted_vec, T val) {
    auto sort_ndx = sorted_vec.begin();
    for (; sort_ndx < sorted_vec.end() and val >= *sort_ndx; ++sort_ndx) {}

    sorted_vec.insert(sort_ndx, val);
  }

  std::tuple<unique_ptr<SubstraitSchema>, vector<int>>
  SchemaFromEmit( const RelCommon&              rel_common
                 ,unique_ptr<SubstraitSchema>&& input_schema) {
    vector<int32_t> sorted_emits;

    if (not rel_common.has_direct()) { return std::make_tuple(input_schema, sorted_emits); }

    // If the operator emits "selectively", attributes are explicitly propagated
    auto emit_schema = std::make_unique<SubstraitSchema>();
    for (int32_t emit_ndx : rel_common.emit().output_mapping()) {
      SubstraitType* emit_type = emit_schema->mutable_struct_()->add_types();

      // We propagate attributes by index based on the `output_mapping` field
      emit_type->CopyFrom(input_schema->struct_().types(emit_ndx));

      // Propagate the attribute name if it has one
      if (emit_ndx < input_schema->names_size()) {
        emit_schema->add_names(input_schema->names(emit_ndx));
      }

      // Keep sorted list of emit indices
      SortedInsert(sorted_emits, emit_ndx);
    }

    return std::make_tuple(emit_schema, sorted_emits);
  }
  */

} // namespace: mohair
