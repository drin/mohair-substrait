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

#include "mohair/analysis/aggregate_rel.hpp"


// ------------------------------
// Aliases

namespace mohair {


  using AggregateFn = skyproto::substrait::AggregateFunction;


} // namespace: mohair


// ------------------------------
// Functions

namespace mohair {

  // >> Analysis of aggregate-specific entities
  void
  AnalyzeAggrFunction(SubstraitSchema* input_schema, const AggregateFn& aggregate_fn) {
    if (not aggregate_fn.has_output_type()) {
      throw std::runtime_error("AggregateFunction is missing output type");
    }

    SubstraitType* new_type = input_schema->mutable_struct_()->add_types();
    new_type->CopyFrom(aggregate_fn.output_type());
  }

  //! Analyze grouping expressions in AggregateRel message to extract schema
  //  information.
  //NOTE: "shared" grouping expressions are preferred over "local".
  void
  TypesFromSharedGroupingExprs( const AggregateRel& rel_op
                               ,SubstraitSchema*    input_schema
                               ,SubstraitSchema*    aggr_schema) {
    // Accumulate types for each grouping expr into input_schema
    int sgexpr_startndx = input_schema->struct_().types_size();
    for (const auto& grp_expr : rel_op.grouping_expressions()) {
      AnalyzeSubstraitExpr(input_schema, grp_expr);
    }

    // Accumulate types for each referenced grouping expr into aggr_schema
    for (const auto& grp_set : rel_op.groupings()) {
      for (const uint32_t sgexpr_ndx : grp_set.expression_references()) {
        SubstraitType* new_type = aggr_schema->mutable_struct_()->add_types();
        new_type->CopyFrom(
          input_schema->struct_().types(sgexpr_startndx + sgexpr_ndx)
        );
      }
    }
  }

  //! Analyze grouping expressions in AggregateRel::Grouping messages to extract
  //  schema information.
  //NOTE: "local" grouping expressions are deprecated.
  void
  TypesFromLocalGroupingExprs( const AggregateRel& rel_op
                              ,SubstraitSchema*    input_schema
                              ,SubstraitSchema*    aggr_schema) {
    // Create a temporary schema for each grouping set
    for (const auto& grp_set : rel_op.groupings()) {
      SubstraitSchema gexpr_schema;
      gexpr_schema.CopyFrom(*input_schema);

      // Accumulate types for each grouping expression into aggr_schema
      for (const auto& grp_expr : grp_set.grouping_expressions()) {
        SubstraitType* gexpr_type = AnalyzeSubstraitExpr(&gexpr_schema, grp_expr);

        SubstraitType* new_type = aggr_schema->mutable_struct_()->add_types();
        new_type->CopyFrom(*gexpr_type);
      }
    }
  }

  // >> Translation helpers
  bool HasSharedGroupExprs(const AggregateRel& rel_op) {
    return not rel_op.grouping_expressions().empty();
  }

  bool HasLocalGroupExprs(const AggregateRel& rel_op) {
    return (
          not rel_op.groupings().empty()
      and not rel_op.groupings(0).grouping_expressions().empty()
    );
  }

  //! Modify `input_schema` based on the expressions contained in `rel_op`.
  //  Returns true if modified, false if not.
  unique_ptr<SubstraitSchema>
  SchemaFromAggregateRel( const AggregateRel&           rel_op
                         ,unique_ptr<SubstraitSchema>&& input_schema) {
    // AggregateRel's ouput schema is grouping expressions + measure expressions
    auto aggr_schema = std::make_unique<SubstraitSchema>();

    // Gather types for grouping expressions into aggr_schema
    if (HasSharedGroupExprs(rel_op)) {
      TypesFromSharedGroupingExprs(rel_op, input_schema.get(), aggr_schema.get());
    }
    else if (HasLocalGroupExprs(rel_op)) {
      TypesFromLocalGroupingExprs(rel_op, input_schema.get(), aggr_schema.get());
    }

    // then, gather types for measure expressions into aggr_schema
    for (const auto& aggr_measure : rel_op.measures()) {
      AnalyzeAggrFunction(aggr_schema.get(), aggr_measure.measure());
    }

    if (not rel_op.has_common()) { return aggr_schema; }
    return SchemaFromEmit(rel_op.common(), std::move(aggr_schema));
  }

} // namespace: mohair
