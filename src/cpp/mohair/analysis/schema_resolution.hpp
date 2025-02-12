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

#include "mohair/analysis/expressions.hpp"
#include "mohair/analysis/rel_common.hpp"
#include "mohair/analysis/project_rel.hpp"
#include "mohair/analysis/aggregate_rel.hpp"
#include "mohair/analysis/join_rel.hpp"


// ------------------------------
// Functions

namespace mohair {

  //! Modifies an input schema given a relational operator
  template <typename RelType>
  unique_ptr<SubstraitSchema>
  SchemaFromRel( [[maybe_unused]] const RelType&                rel_op
                ,[[maybe_unused]] unique_ptr<SubstraitSchema>&& input_schema) {
    // Return an error if we don't find a specialization
    std::cerr << "Unsupported type for schema analysis." << std::endl;
    return nullptr;
  }

  //! Specialization of `SchemaFromRel` for ProjectRel
  template <>
  unique_ptr<SubstraitSchema>
  SchemaFromRel(const ProjectRel& rel_op, unique_ptr<SubstraitSchema>&& input_schema) {
    return SchemaFromProjectRel(rel_op, std::move(input_schema));
  }

  //! Specialization of `SchemaFromRel` for AggregateRel
  template <>
  unique_ptr<SubstraitSchema>
  SchemaFromRel(const AggregateRel& rel_op, unique_ptr<SubstraitSchema>&& input_schema) {
    return SchemaFromAggregateRel(rel_op, std::move(input_schema));
  }

  //! Specialization of `SchemaFromRel` for JoinRel
  template <>
  unique_ptr<SubstraitSchema>
  SchemaFromRel(const JoinRel& rel_op, unique_ptr<SubstraitSchema>&& input_schema) {
    return SchemaFromJoinRel(rel_op, std::move(input_schema));
  }

} // namespace: mohair
