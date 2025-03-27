// ------------------------------
// License
//
// Copyright 2024 Aldrin Montana
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


// ------------------------------
// Base classes for query operators and plans

namespace mohair {

  // >> Type forwards
  struct OpPipeline;
  struct PipelineStage;

  // >> Operator Pipelines
  using PipelineVector = vector<unique_ptr<OpPipeline>>;
  using StageVector    = vector<unique_ptr<PipelineStage>>;

  struct OpPipeline {
    SubstraitOp*         sink;
    SubstraitOp*         next;
    SubstraitOp*         source;
    vector<SubstraitOp*> pipe_ops;

    OpPipeline(SubstraitOp* dest, SubstraitOp* dest_next)
      : sink(dest), next(dest_next), source(nullptr) {}

    const string ToString(string prefix);
    const string ViewStr() { return this->ToString(""); }

    size_t Size();
    void   AddOp(SubstraitOp* pipe_op);
    void   SetSource(SubstraitOp* src);

    SubstraitOp* GetSourceNext();
  };

  struct PipelineStage {
    SubstraitOp*   sink;
    PipelineStage* next;
    PipelineVector pipelines;
    vector<string> origin_names;
    size_t         length;
    size_t         width;

    PipelineStage(SubstraitOp* dest, PipelineStage* next_stage, size_t stage_width)
      : sink(dest), next(next_stage), length(0), width(stage_width) {
      pipelines.reserve(stage_width);
    }

    const string ToString(string prefix);
    const string ViewStr() { return this->ToString(""); }

    OpPipeline& CreatePipeline(SubstraitOp* sink_next);
  };


  // >> Query plans
  //! A query plan managed by the cooperative decomposition system.
  struct PlanPipeline {
    SubstraitPlan*      plan;
    size_t              breadth;
    size_t              depth;
    StageVector         stages;
    vector<OpPipeline*> origin_pipelines;

    PlanPipeline(SubstraitPlan* src_plan): plan(src_plan) {}

    //! Create a PipelineStage (ending with `sink`) and associate it with the downstream
    //  PipelineStage (where output of the new stage will flow to).
    PipelineStage& CreatePipelineStage(SubstraitOp* sink, PipelineStage* next, size_t width);

    //! Print pipelines stringified to stdout
    void PrintPipelines();

    //! Builds pipelines from plan operators and discovers plan characteristics
    static PlanPipeline Build(SubstraitPlan* src_plan);
  };


  // >> Cooperative Query Decomposition
  // TODO: a subplan UUID should be derived from a plan UUID
  struct SplitAnchor {
    SubstraitPlan* plan;
    int32_t        anchor_relndx;
    size_t         merge_relndx;
    SubstraitOp*   anchor_op;
    bool           is_merged;

    SplitAnchor(SubstraitPlan* splan, int32_t ndx_arel, size_t ndx_mrel, SubstraitOp* sop)
      :  plan(splan)
        ,anchor_relndx(ndx_arel)
        ,merge_relndx(ndx_mrel)
        ,anchor_op(sop)
        ,is_merged(false) {}

    void MergeSubplan();
  };

  /**
   * A class that points to a super-plan and an anchor operator.
   *
   * The anchor is an operator whose input is on the cut of the plan. This means that the
   * anchor is a leaf in the super-plan and a parent of each sub-plan root.
   */
  struct PlanSplit {
    SystemPlan*       sys_plan;
    Plan*             super_plan;
    PipelineStage*    stage;
    size_t            stage_ndx;
    SubstraitOp*         superplan_mergerel;
    vector<SubstraitOp*> subplan_rootrels;

    //! Constructs a plan split from the given pipeline stage.
    //  If the stage has many pipelines, the anchor is the stage's sink.
    //  Otherwise, the anchor is the sink's downstream operator (where output flows to).
    PlanSplit(SystemPlan* plan, Plan* superplan, PipelineStage* split_stage, size_t ndx)
      : sys_plan(plan), super_plan(superplan), stage(split_stage), stage_ndx(ndx) {
      // The whole plan will be propagated
      if (split_stage == nullptr) {
        superplan_mergerel = sys_plan->plan_root.get();
      }

      // We propagate subplans
      else {
        // Assume stage sink is the merge relation, unless it's unary
        // IDEA: if sink is join, then it reduces memory pressure to make it the merge relation
        //       otherwise, it reduces data movement across the network to push it down
        superplan_mergerel = split_stage->sink;
        if (split_stage->width == 1 and split_stage->pipelines[0]->next != nullptr) {
          superplan_mergerel = split_stage->pipelines[0]->next;
        }

        size_t count_inputs = superplan_mergerel->GetOpArity();
        subplan_rootrels.reserve(count_inputs);
        for (size_t input_ndx = 0; input_ndx < count_inputs; ++input_ndx) {
          subplan_rootrels.push_back(
            superplan_mergerel->GetOpInputs()[input_ndx].get()
          );
        }
      }
    }

    //! Finds a candidate split given a decision algorithm
    static unique_ptr<PlanSplit>
    FindSplit(SystemPlan* sys_plan, const DecomposeAlg& method = DecomposeAlg::None);

    bool CanSplit();
    bool MergeResultRel(Rel* result_rel);
    bool MergeSubplan(SubstraitPlan* subplan_msg);

    unique_ptr<SubstraitPlan>         ExtractExecSubplan();
    vector<unique_ptr<SubstraitPlan>> ExtractSubplans();
  };

} // namespace: mohair


// ------------------------------
// Functions

namespace mohair {

  // >> Translation Functions
  //! Walks the operator tree starting at `rel_msg` and returns a `SubstraitOp` tree
  unique_ptr<SubstraitOp> ParseSubstraitPlan(Plan* plan_msg);
  unique_ptr<SubstraitOp> ParseSubstraitPlan(Plan* plan_msg, Rel* rel_msg);

  //! Constructs a `SystemPlan` from the given deserialized substrait `Plan`
  unique_ptr<SystemPlan> SystemPlanFrom(unique_ptr<SubstraitPlan>&& plan_msg);

  //! Constructs a `SystemPlan` from the given serialized substrait `Plan`
  unique_ptr<SystemPlan> SystemPlanFrom(const string& serialized_msg);

  //! Constructs a `SuperPlan` message for the given operator
  unique_ptr<SuperPlan>  SuperPlanFrom(SubstraitOp* mohair_op);

} // namespace: mohair
