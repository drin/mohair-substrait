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

// >> Implementations for SubstraitOpImpl<ReferenceRel>

namespace mohair {

  //! Copy the internal Rel and RelOp without recursing into inputs
  template <>
  unique_ptr<Rel> SubstraitOpImpl<ReferenceRel>::CopyRelOp() const {
    unique_ptr<Rel> rel_copy { std::make_unique<Rel>() };

    rel_copy->mutable_reference()->set_subtree_ordinal(rel_op->subtree_ordinal());
    rel_copy->mutable_reference()->set_subtree_reference(rel_op->subtree_reference());

    return rel_copy;
  }

  //! A visitor function to build plan pipelines with this operator as the final sink
  template <>
  void
  SubstraitOpImpl<ReferenceRel>::AddToPipeline( PlanPipeline&  plan_pipe
                                               ,PipelineStage& pipe_stage
                                               ,OpPipeline&    pipeline) {
    // Use this operator as the source of the specified pipeline
    pipeline.SetSource(this);

    // Track pipelines that have an origin operator
    plan_pipe->origin_pipelines.push_back(&pipeline);

    // TODO: figure out how to specify an execution plan that reads from the
    //       referenced result
  }

  //! Replace the internal Rel with new_srel and return pointer to the old Rel
  template <>
  Rel* SubstraitOpImpl<ReferenceRel>::MoveToRel(Rel* new_srel) {
    // Cache the pointer to the old rel
    Rel* old_srel = rel;
    
    // Move rel_op into the new rel and refresh rel_op (in case its invalidated)
    new_srel->set_allocated_reference(old_srel->release_reference());
    rel    = new_srel;
    rel_op = new_srel->mutable_reference();

    // Return pointer to the old rel
    return old_srel;
  }

} // namespace: mohair

