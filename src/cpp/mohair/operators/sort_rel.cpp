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

// >> Implementations for SubstraitOpImpl<SortRel>

namespace mohair {

  //! Copy the internal Rel and RelOp without recursing into inputs
  template <>
  unique_ptr<Rel> SubstraitOpImpl<SortRel>::CopyRelOp() const {
    unique_ptr<Rel> rel_copy  { std::make_unique<Rel>() };
    SortRel&        sort_copy = *(rel_copy->mutable_sort());

    sort_copy.mutable_common()->CopyFrom(rel_op->common());
    sort_copy.mutable_advanced_extension()->CopyFrom(rel_op->advanced_extension());

    sort_copy.mutable_sorts()->CopyFrom(rel_op->sorts());

    return rel_copy;
  }

  //! A visitor function to build plan pipelines with this operator as the final sink
  template <>
  void
  SubstraitOpImpl<SortRel>::AddToPipeline( PlanPipeline&  plan_pipe
                                          ,PipelineStage& pipe_stage
                                          ,OpPipeline&    pipeline) {
    // Use this operator as the source of the specified pipeline
    pipeline.SetSource(this);

    // Create a new stage, pipeline, and recurse on input
    // NOTE: "upstream" means closer to the __origin__ of data flow (source table)
    PipelineStage& upstream_stage = plan_pipe->CreatePipelineStage(this, &pipe_stage, 1);
    OpPipeline& upstream_pipe = upstream_stage.CreatePipeline(pipeline.GetSourceNext());

    input_ops[0]->AddToPipeline(plan_pipe, upstream_stage, upstream_pipe);

    // When a pipeline is fully built, see if it's the longest for the stage
    if (upstream_pipe.Size() > upstream_stage.length) {
      upstream_stage.length = upstream_pipe.Size();
    }
  }

  //! Replace the internal Rel with new_srel and return pointer to the old Rel
  template <>
  Rel* SubstraitOpImpl<SortRel>::MoveToRel(Rel* new_srel) {
    // Cache the pointer to the old rel
    Rel* old_srel = rel;
    
    // Move rel_op into the new rel and refresh rel_op (in case its invalidated)
    new_srel->set_allocated_sort(old_srel->release_sort());
    rel    = new_srel;
    rel_op = new_srel->mutable_sort();

    // Return pointer to the old rel
    return old_srel;
  }

} // namespace: mohair

