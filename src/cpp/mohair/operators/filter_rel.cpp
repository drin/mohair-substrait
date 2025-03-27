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


// ------------------------------
// Dependencies

#include "mohair/operators.hpp"


// ------------------------------
// Functions

// >> Implementations for SubstraitOpImpl<FilterRel>

namespace mohair {

  //! Copy the internal Rel and RelOp without recursing into inputs
  template <>
  unique_ptr<Rel> SubstraitOpImpl<FilterRel>::CopyRelOp() {
    unique_ptr<Rel> rel_copy  { std::make_unique<Rel>() };
    FilterRel&      sel_copy = *(rel_copy->mutable_filter());

    sel_copy.mutable_common()->CopyFrom(rel_op->common());
    sel_copy.mutable_advanced_extension()->CopyFrom(rel_op->advanced_extension());

    sel_copy.mutable_condition()->CopyFrom(rel_op->condition());

    return rel_copy;
  }

  //! A visitor function to add this operator to the specified OpPipeline
  template <>
  void
  SubstraitOpImpl<ProjectRel>::AddToPipeline( PlanPipeline&  plan_pipe
                                             ,PipelineStage& pipe_stage
                                             ,OpPipeline&    pipeline) {
    pipeline.AddOp(this);
    input_ops[0]->AddToPipeline(plan_pipe, pipe_stage, pipeline);
  }

  //! Replace the internal Rel with new_srel and return pointer to the old Rel
  template <>
  Rel* SubstraitOpImpl<FilterRel>::MoveToRel(Rel* new_srel) {
    // Cache the pointer to the old rel
    Rel* old_srel = rel;
    
    // Move rel_op into the new rel and refresh rel_op (in case its invalidated)
    new_srel->set_allocated_filter(old_srel->release_filter());
    rel    = new_srel;
    rel_op = new_srel->mutable_filter();

    // Return pointer to the old rel
    return old_srel;
  }

} // namespace: mohair

