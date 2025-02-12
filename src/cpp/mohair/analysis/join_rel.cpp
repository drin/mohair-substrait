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

#include "mohair/analysis/join_rel.hpp"


// ------------------------------
// Functions

namespace mohair {

  unique_ptr<SubstraitSchema>
  SchemaFromJoinRel(const JoinRel& rel_op, unique_ptr<SubstraitSchema>&& input_schema) {
    if (not rel_op.has_common()) { return input_schema; }
    return SchemaFromEmit(rel_op.common(), std::move(input_schema));
  }

} // namespace: mohair
