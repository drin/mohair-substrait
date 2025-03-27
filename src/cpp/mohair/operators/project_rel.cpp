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

// >> Implementations for SubstraitOpImpl<ProjectRel>

namespace mohair {

  //! Copy the internal Rel and RelOp without recursing into inputs
  template <>
  unique_ptr<Rel> SubstraitOpImpl<ProjectRel>::CopyRelOp() const {
    unique_ptr<Rel> rel_copy  { std::make_unique<Rel>() };
    ProjectRel&     proj_copy = *(rel_copy->mutable_project());

    proj_copy.mutable_common()->CopyFrom(rel_op->common());
    proj_copy.mutable_advanced_extension()->CopyFrom(rel_op->advanced_extension());

    proj_copy.mutable_expressions()->CopyFrom(rel_op->expressions());

    return rel_copy;
  }

  //! A visitor function to build plan pipelines with this operator as the final sink
  template <>
  void
  SubstraitOpImpl<ProjectRel>::AddToPipeline( PlanPipeline&  plan_pipe
                                             ,PipelineStage& pipe_stage
                                             ,OpPipeline&    pipeline) {
    pipeline.AddOp(this);
    input_ops[0]->AddToPipeline(plan_pipe, pipe_stage, pipeline);
  }

    while (not current_op->IsSink() and not current_op->IsOrigin()) {

    // Complete the current pipeline
    current_pipe.SetSource(current_op);

    // Base case: we hit an origin operator (e.g. ReadRel or SkyPartitionRel)
    if (current_op->IsOrigin()) {
      plan->origin_pipelines.push_back(&current_pipe);

      SourceOp* origin_src = dynamic_cast<SourceOp*>(current_op);

      current_stage.origin_names.push_back(origin_src->table_name);
      while (current_stage.next != nullptr) {
        current_stage = *(current_stage->next);
        current_stage.origin_names.push_back(origin_src->table_name);
      }

      return;
    }
  }

  //! Replace the internal Rel with new_srel and return pointer to the old Rel
  template <>
  Rel* SubstraitOpImpl<ProjectRel>::MoveToRel(Rel* new_srel) {
    // Cache the pointer to the old rel
    Rel* old_srel = rel;
    
    // Move rel_op into the new rel and refresh rel_op (in case its invalidated)
    new_srel->set_allocated_project(old_srel->release_project());
    rel    = new_srel;
    rel_op = new_srel->mutable_project();

    // Return pointer to the old rel
    return old_srel;
  }

} // namespace: mohair

