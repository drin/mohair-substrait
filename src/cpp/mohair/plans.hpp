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
  struct SystemPlan;

  // >> Query operators
  struct MohairOp {
    Rel* substrait_rel;

    virtual ~MohairOp() = default;

    MohairOp(Rel* rel): substrait_rel(rel) {}

    virtual const string ToString();
    virtual const string ViewStr();

    virtual bool   IsSink();
    virtual bool   IsOrigin();
    virtual size_t GetOpArity();

    virtual unique_ptr<MohairOp>* GetOpInputs();

    virtual void SimplifyMessage(Rel* rel);
  };

  struct SourceOp : public MohairOp {
    string table_name;

    SourceOp(Rel* rel, string tname): MohairOp(rel), table_name(tname) {}

    bool IsOrigin() override { return true; }
  };

  // >> Operator Pipelines

  // Type forwards
  struct OpPipeline;
  struct PipelineStage;

  using PipelineVector = vector<unique_ptr<OpPipeline>>;
  using StageVector    = vector<unique_ptr<PipelineStage>>;

  // NOTE: deciding what stage to put a pipeline into can be: early/late materialization
  struct PipelineStage {
    MohairOp*      sink;
    PipelineStage* next;
    PipelineVector pipelines;
    vector<string> origin_names;
    size_t         length;
    size_t         width;

    PipelineStage(MohairOp* dest, PipelineStage* next_stage, size_t stage_width)
      : sink(dest), next(next_stage), length(0), width(stage_width) {
      pipelines.reserve(stage_width);
    }

    const string ToString(string prefix);
    const string ViewStr() { return this->ToString(""); }

    OpPipeline&  CreatePipeline(MohairOp* sink_next);
  };

  struct OpPipeline {
    MohairOp*         sink;
    MohairOp*         next;
    MohairOp*         source;
    vector<MohairOp*> pipe_ops;

    OpPipeline(MohairOp* dest, MohairOp* dest_next)
      : sink(dest), next(dest_next), source(nullptr) {}

    const string ToString(string prefix);
    const string ViewStr() { return this->ToString(""); }

    size_t       Size();
    void         AddOp(MohairOp* pipe_op);
    void         SetSource(MohairOp* src);
  };

  // >> Query plans

  //! A query plan managed by the cooperative decomposition system.
  struct SystemPlan {
    unique_ptr<PlanMessage>         plan_msg;
    unique_ptr<MohairOp>            plan_root;
    size_t                          breadth;
    size_t                          depth;

    StageVector                     pipeline_stages;
    vector<OpPipeline*>             origin_pipelines;

    unordered_map<uint64_t, string> fn_anchors;

    virtual ~SystemPlan() {}

    SystemPlan(unique_ptr<PlanMessage>&& plan, unique_ptr<MohairOp>&& op)
      : plan_msg(std::move(plan)), plan_root(std::move(op)) {}

    // >> Accessors
    //! Access the function name associated with the given anchor
    string         FnNameForAnchor(uint64_t anchor_id);
    const RelRoot& RootRelation() const;

    // >> Convenience methods

    //! Create a PipelineStage (ending with `sink`) and associate it with the downstream
    //  PipelineStage (where output of the new stage will flow to).
    PipelineStage& CreatePipelineStage(MohairOp* sink, PipelineStage* next, size_t width);

    //! Builds pipelines from plan operators and discovers plan characteristics
    void BuildPipelines();
    void PrintPipelines();

    //! Create mapping of function anchors in the query plan
    void RegisterExtensionFunctions();
  };

  // >> Cooperative Query Decomposition
  enum DecomposeAlg {
     LongPipelineLeaf // Leaf pipeline breaker with longest pipeline
    ,LongPipelineHead // Internal pipeline breaker with longest pipeline
    ,TallJoinLeaf     // Leaf join operation with tallest plan height
    ,WideJoinHead     // Internal join operation with largest plan width
  };

  /**
   * A class that points to a super-plan and an anchor operator.
   *
   * The anchor is an operator whose input is on the cut of the plan. This means that the
   * anchor is a leaf in the super-plan and a parent of each sub-plan root.
   */
  struct PlanSplit {
    PipelineStage* stage;
    MohairOp*      merge_rel;

    //! Constructs a plan split from the given pipeline stage.
    //  If the stage has many pipelines, the anchor is the stage's sink.
    //  Otherwise, the anchor is the sink's downstream operator (where output flows to).
    PlanSplit(PipelineStage* split_stage): stage(split_stage) {
      if (split_stage->width > 1) { merge_rel = split_stage->sink; }
      else { merge_rel = split_stage->pipelines[0]->next; }
    }

    //! Finds a candidate split given a decision algorithm
    static unique_ptr<PlanSplit>
    FindSplit(SystemPlan* sys_plan, DecomposeAlg method = DecomposeAlg::LongPipelineLeaf);

    vector<unique_ptr<PlanMessage>>
    SubplansFor(PlanMessage* plan_msg);
  };

} // namespace: mohair


// ------------------------------
// Functions

namespace mohair {

  // >> Translation Functions
  unique_ptr<MohairOp>   MohairFrom(Rel *rel_msg);
  unique_ptr<SystemPlan> SystemPlanFrom(const string&             serialized_msg);
  unique_ptr<SystemPlan> SystemPlanFrom(unique_ptr<PlanMessage>&& plan_msg);

  unique_ptr<SuperPlan>  SuperPlanFrom(MohairOp* mohair_op);

  /*
   * vector<unique_ptr<PlanMessage>>
   * SubplansFromSplit(PlanMessage* plan_msg, PlanSplit& split);
   */

} // namespace: mohair
