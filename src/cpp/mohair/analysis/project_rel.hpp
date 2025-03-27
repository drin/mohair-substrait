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

#include "mohair/analysis/expressions.hpp"
#include "mohair/analysis/rel_common.hpp"


// ------------------------------
// Functions

namespace mohair {

  unique_ptr<SubstraitSchema>
  SchemaFromProjectRel( const ProjectRel&             rel_op
                       ,unique_ptr<SubstraitSchema>&& input_schema);

} // namespace: mohair
