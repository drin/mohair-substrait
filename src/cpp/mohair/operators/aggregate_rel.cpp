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
// Overview
//
// AggregateRel has 3 operation-specific attributes:
//  1. Grouping Sets
//  2. Measures
//  3. Grouping Expressions
//
//  1. A grouping set identifies a set of "tuple groups" for which each measure
//  will be calculated over. For example, an "empty" grouping set corresponds to
//  a single group of all tuples (aka "*"). A grouping set that specifies an
//  attribute, A, of the input schema corresponds to N groups of tuples, where N
//  is the count of distinct values of A (e.g. if attribute A contains US
//  states, then N must be <= 50).
//
//  2. A measure is a function to calculate an aggregate value--a value that
//  summarizes some information across all tuples of a grouping set. For
//  example, a sum (or summation) is a measure. If an attribute A contains US
//  states, an attribute B contains US cities, and an attribute C contains the
//  population of a US city, then a sum would be a measure that could be
//  computed by summing values of attribute C for a variety of grouping sets.
//  For the empty grouping set, the sum would measure the population of all US
//  cities in the active domain. For the grouping set specifying A directly, the
//  sum would measure the population of all US cities by state.
//
//  3. A grouping expression is an expression used to identify the value of a
//  grouping set. A direct reference uses the values of an attribute as-is, in
//  the case of attribute A, it would be US states in the active domain. Another
//  expression might specify values of attribute A with a string length over 5,
//  which would be US states in the active domain excluding states such as Utah
//  and Idaho. Another expression might specify a prefix of values of
//  attribute A so that some states, such as "New York" and "New Hampshire", are
//  identified as the same grouping set. Grouping expressions allows flexibility
//  in reducing, multiplying, or omitting groups used for an aggregate beyond a
//  simple direct reference.


// ------------------------------
// Dependencies

#include "mohair/operators.hpp"


// ------------------------------
// Functions

// >> Implementations for SubstraitOpImpl<AggregateRel>

namespace mohair {

  //! Copy the internal Rel and RelOp without recursing into inputs
  template <>
  unique_ptr<Rel> SubstraitOpImpl<AggregateRel>::CopyRelOp() {
    unique_ptr<Rel> rel_copy  { std::make_unique<Rel>() };
    AggregateRel&   aggr_copy = *(rel_copy->mutable_aggregate());

    aggr_copy.mutable_common()->CopyFrom(rel_type->common());
    aggr_copy.mutable_advanced_extension()->CopyFrom(rel_type->advanced_extension());

    aggr_copy.mutable_groupings()->CopyFrom(rel_type->groupings());
    aggr_copy.mutable_measures()->CopyFrom(rel_type->measures());
    aggr_copy.mutable_grouping_expressions()->CopyFrom(rel_type->grouping_expressions());

    return rel_copy;
  }

  //! A visitor function to build plan pipelines with this operator as the final sink
  template <>
  void
  SubstraitOpImpl<AggregateRel>::AddToPipeline( PlanPipeline&  plan_pipe
                                               ,PipelineStage& pipe_stage
                                               ,OpPipeline&    pipeline) {
    // Use this operator as the source of the specified pipeline
    pipeline.SetSource(this);

    // Create a pipeline stage with this operator as a sink
    PipelineStage& upstream_stage = plan_pipe->CreatePipelineStage(this, &pipe_stage, 1);

    // Create a pipeline for the input operator
    OpPipeline& upstream_pipe = upstream_stage.CreatePipeline(pipeline.GetSourceNext());
    input_ops[0]->AddToPipeline(plan_pipe, upstream_stage, upstream_pipe);

    // When a pipeline is fully built, see if it's the longest for the stage
    if (upstream_pipe.Size() > upstream_stage.length) {
      upstream_stage.length = upstream_pipe.Size();
    }
  }

  //! Replace the internal Rel with new_srel and return pointer to the old Rel
  template <>
  Rel* SubstraitOpImpl<AggregateRel>::MoveToRel(Rel* new_srel) {
    // Cache the pointer to the old rel
    Rel* old_srel = rel;
    
    // Move rel_op into the new rel and refresh rel_op (in case its invalidated)
    new_srel->set_allocated_aggregate(old_srel->release_aggregate());
    rel    = new_srel;
    rel_op = new_srel->mutable_aggregate();

    // Return pointer to the old rel
    return old_srel;
  }

} // namespace: mohair
