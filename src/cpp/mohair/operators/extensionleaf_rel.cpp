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

// >> Implementations for SubstraitOpImpl<ExtensionLeafRel>

namespace mohair {

  //! Copy the internal Rel and RelOp without recursing into inputs
  template <>
  unique_ptr<Rel> SubstraitOpImpl<ExtensionLeafRel>::CopyRelOp() const {
    unique_ptr<Rel>   rel_copy  { std::make_unique<Rel>() };
    ExtensionLeafRel& leaf_copy = *(rel_copy->mutable_extension_leaf());

    leaf_copy->mutable_common()->CopyFrom(rel_op->common());
    leaf_copy->mutable_detail()->CopyFrom(rel_op->detail());

    return rel_copy;
  }

  //! A visitor function to build plan pipelines with this operator as the final sink
  template <>
  void
  SubstraitOpImpl<ExtensionLeafRel>::AddToPipeline( PlanPipeline&  plan_pipe
                                                   ,PipelineStage& pipe_stage
                                                   ,OpPipeline&    pipeline) {
    // Use this operator as the source of the specified pipeline
    // TODO: figure out if the source should be this op or a custom op
    pipeline.SetSource(this);

    // Track pipelines that have an origin operator
    plan_pipe->origin_pipelines.push_back(&pipeline);

    // Update pipeline and downstream pipelines with the source name
    pipe_stage.origin_names.push_back(source_name);
    while (pipe_stage.next != nullptr) {
      pipe_stage = *(pipe_stage->next);
      pipe_stage.origin_names.push_back(source_name);
    }
  }

  //! Replace the internal Rel with new_srel and return pointer to the old Rel
  template <>
  Rel* SubstraitOpImpl<ExtensionLeafRel>::MoveToRel(Rel* new_srel) {
    // Cache the pointer to the old rel
    Rel* old_srel = rel;
    
    // Move rel_op into the new rel and refresh rel_op (in case its invalidated)
    new_srel->set_allocated_extension_leaf(old_srel->release_extension_leaf());
    rel    = new_srel;
    rel_op = new_srel->mutable_extension_leaf();

    // Return pointer to the old rel
    return old_srel;
  }

} // namespace: mohair

