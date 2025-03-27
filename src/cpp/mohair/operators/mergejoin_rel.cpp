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

// >> Implementations for SubstraitOpImpl<MergeJoinRel>

namespace mohair {

  //! Copy the internal Rel and RelOp without recursing into inputs
  template <>
  unique_ptr<Rel> SubstraitOpImpl<MergeJoinRel>::CopyRelOp() {
    unique_ptr<Rel> rel_copy   { std::make_unique<Rel>() };
    MergeJoinRel&   mjoin_copy = *(rel_copy->mutable_merge_join());

    mjoin_copy.mutable_common()->CopyFrom(rel_op->common());
    mjoin_copy.mutable_advanced_extension()->CopyFrom(rel_op->advanced_extension());

    mjoin_copy.mutable_post_join_filter->CopyFrom(rel_op->post_join_filter());
    mjoin_copy.set_type(rel_op->type());

    if (mjoin_copy.has_left_keys()) {
      mjoin_copy.mutable_left_keys()->CopyFrom(rel_op->left_keys());
    }

    if (mjoin_copy.has_right_keys()) {
      mjoin_copy.mutable_right_keys()->CopyFrom(rel_op->right_keys());
    }

    if (mjoin_copy.has_keys()) { mjoin_copy.mutable_keys()->CopyFrom(rel_op->keys()); }

    return rel_copy;
  }

  //! A visitor function to build plan pipelines with this operator as the final sink
  template <>
  void
  SubstraitOpImpl<MergeJoinRel>::AddToPipeline( PlanPipeline&  plan_pipe
                                               ,PipelineStage& pipe_stage
                                               ,OpPipeline&    pipeline) {
    // Use this operator as the source of the specified pipeline
    pipeline.SetSource(this);

    // Create a pipeline stage with this operator as a sink
    PipelineStage& upstream_stage = plan->CreatePipelineStage(
      this, &pipe_stage, OpTraits::Arity
    );

    // Create a pipeline for each input operator
    SubstraitOp* source_next = pipeline.GetSourceNext();
    for (size_t ndx_input = 0; ndx_input < OpTraits::Arity; ++ndx_input) {
      OpPipeline& upstream_pipe = upstream_stage.CreatePipeline(source_next);

      input_ops[ndx_input]->AddToPipeline(plan_pipe, upstream_stage, upstream_pipe);

      // Maintain which pipeline is longest for the stage
      if (upstream_pipe.Size() > upstream_stage.length) {
        upstream_stage.length = upstream_pipe.Size();
      }
    }
  }

  //! Replace the internal Rel with new_srel and return pointer to the old Rel
  template <>
  Rel* SubstraitOpImpl<MergeJoinRel>::MoveToRel(Rel* new_srel) {
    // Cache the pointer to the old rel
    Rel* old_srel = rel;
    
    // Move rel_op into the new rel and refresh rel_op (in case its invalidated)
    new_srel->set_allocated_merge_join(old_srel->release_merge_join());
    rel    = new_srel;
    rel_op = new_srel->mutable_merge_join();

    // Return pointer to the old rel
    return old_srel;
  }

} // namespace: mohair

