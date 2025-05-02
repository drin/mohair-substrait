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
    uint32_t                    PlanOpID;
    Rel*                        substrait_rel;
    unique_ptr<SubstraitSchema> schema;

    virtual ~MohairOp() = default;

    MohairOp(Rel* rel, unique_ptr<SubstraitSchema>&& input_schema)
      : substrait_rel(rel), schema(std::move(input_schema)) {
    }

    virtual const string ToString();
    virtual const string ViewStr();
    virtual const string GetName();

    virtual void   AssignOpID();
    virtual void   UpdateOperatorIDs(vector<MohairOp*>& plan_ops);

    virtual bool   IsSink();
    virtual bool   IsOrigin();
    virtual size_t GetOpArity();

    //! Get input operators (child operators) as a C-style array
    virtual unique_ptr<MohairOp>* GetOpInputs();

    //! Return a simplified copy of the `substrait_rel` attribute.
    virtual unique_ptr<Rel> CopySubstraitRel();
  };

  struct SourceOp : public MohairOp {
    string table_name;

    SourceOp(Rel* rel, string tname, unique_ptr<SubstraitSchema>&& input_schema)
      : MohairOp(rel, std::move(input_schema)), table_name(tname) {}

    const string GetName() override  { return table_name; }
    bool         IsOrigin() override { return true;       }
  };

  // >> Operator Pipelines

  // Type forwards
  struct OpPipeline;
  struct PipelineStage;

  using PipelineVector = vector<unique_ptr<OpPipeline>>;
  using StageVector    = vector<unique_ptr<PipelineStage>>;

  // NOTE: deciding what stage to put a pipeline into can be: early/late materialization
  struct PipelineStage {
    PipelineStage*   next;
    MohairOp*        sink;
    PipelineVector   pipelines;
    vector<uint64_t> origin_hashes;
    size_t           length;
    size_t           width;

    //! Stage descriptor and hash include the sink, thus building on what a downstream
    //  pipeline already constructed
    string           stage_desc;
    uint64_t         stage_hash;

    PipelineStage(PipelineStage* next_stage, MohairOp* dest)
      : next(next_stage), sink(dest), length(0), stage_hash(0) {
      stage_desc = string { sink->ToString() };
      width      = sink->GetOpArity();

      pipelines.reserve(width);
    }

    const string ToString(string prefix);
    const string ViewStr() { return this->ToString(""); }

    //! Default value for upstream_hash is the FNV-1a offset basis
    uint64_t    Hash(uint64_t upstream_hash = 14695981039346656037ULL);
    OpPipeline& CreatePipeline(OpPipeline* next_pipe);

    //! A convenience method to be called when an origin pipeline is built
    void AddOriginPipeline(OpPipeline& origin_pipe);
  };

  struct OpPipeline {
    OpPipeline*       next;
    MohairOp*         sink;
    MohairOp*         source;
    vector<MohairOp*> pipe_ops;

    //! Flow descriptor and hash for a pipeline are exclusive of the sink
    string            flow_desc;
    uint64_t          flow_hash;
    vector<uint64_t>  upstream_hashes;

    OpPipeline(OpPipeline* next_pipe, MohairOp* dest)
      : next(next_pipe), sink(dest), source(nullptr), flow_desc(""), flow_hash(0) {}

    const string ToString(string prefix);
    const string ViewStr() { return this->ToString(""); }

    size_t   Size();
    uint64_t Hash();
    void     AddOp(MohairOp* pipe_op);
    void     SetSource(MohairOp* src);
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
    vector<MohairOp*>               plan_ops;

    unordered_map<uint64_t, string> fn_anchors;

    virtual ~SystemPlan() {}

    SystemPlan(unique_ptr<PlanMessage>&& plan, unique_ptr<MohairOp>&& op)
      : plan_msg(std::move(plan)), plan_root(std::move(op)) {
      constexpr size_t initial_countops { 32 };
      plan_ops.reserve(initial_countops);
    }

    // >> Accessors
    //! Access the function name associated with the given anchor
    string         FnNameForAnchor(uint64_t anchor_id);
    const RelRoot& RootRelation() const;

    uint64_t Hash() const;

    // >> Convenience methods

    //! Create a new upstream PipelineStage and associate it with the given downstream
    //  stage and sink operator.
    PipelineStage& CreatePipelineStage(PipelineStage* next, MohairOp* sink);

    //! Searches for pipeline sink operators that are in the root subtree
    vector<MohairOp*> FindOriginPipelines();

    //! Builds pipelines from plan operators and discovers plan characteristics
    void BuildPipelines();

    //! Create mapping of function anchors in the query plan
    void RegisterExtensionFunctions();

    //! Print pipelines stringified to stdout
    void PrintPipelines();
  };

  // >> Cooperative Query Decomposition

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
    MohairOp*         superplan_mergerel;
    vector<MohairOp*> subplan_rootrels;
    vector<uint64_t>  origin_hashes;

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
        // Default merge relation is the stage sink
        uint32_t anchor_opid = 0;
        superplan_mergerel   = split_stage->sink;

        // TODO: needs validating experiments
        // TODO: if many downstream engines, an operator could be split into 2 partial
        //       operators.
        // hypothesis (one downstream engine):
        //    if sink has many inputs, using it as the merge relation reduces memory
        //    pressure at downstream engines (and is thus preferable).
        //    if sink is unary, it reduces data movement across network to push it down
        if (split_stage->width == 1 and split_stage->pipelines[0]->next != nullptr) {
          OpPipeline* next_pipe = split_stage->pipelines[0]->next;
          MohairOp*   sink_next = next_pipe->sink;
          if (not next_pipe->pipe_ops.empty()) {
            sink_next = next_pipe->pipe_ops.back();
          }

          // Set the merge operator to be downstream of the stage sink
          // (so we can delegate the stage sink)
          superplan_mergerel = sink_next;

          // Set anchor operator ID to the sink operator, so we can find the correct
          // pointer (merge op -> anchor op) to modify
          anchor_opid = GetRelCommon(split_stage->sink->substrait_rel)->operator_id();
        }

        // TODO: clean up
        // Cache this for debugging purposes
        origin_hashes = split_stage->origin_hashes;

        size_t count_inputs = superplan_mergerel->GetOpArity();
        subplan_rootrels.reserve(count_inputs);
        for (size_t input_ndx = 0; input_ndx < count_inputs; ++input_ndx) {
          MohairOp*  subplan_root   = superplan_mergerel->GetOpInputs()[input_ndx].get();
          RelCommon* subplan_common = GetRelCommon(subplan_root->substrait_rel);

          // use operator id to capture the correct anchor rel
          if (anchor_opid == 0 or anchor_opid == subplan_common->operator_id()) {
            subplan_rootrels.push_back(subplan_root);
          }
        }
      }

      // Clear split override annotation to reduce confusion
      superplan_mergerel->substrait_rel->clear_has_splitoverride();
    }

    //! Finds a candidate split given a decision algorithm
    static unique_ptr<PlanSplit>
    FindSplit(SystemPlan* sys_plan, const DecomposeAlg& method = DecomposeAlg::None);

    void PrintSplit();
    bool CanSplit();
    bool MergeResultRel(Rel* result_rel);
    bool MergeSubplan(PlanMessage* subplan_msg);
    bool MergeOriginResult(OpPipeline* origin_pipe, Rel* result_rel);

    vector<OpPipeline*>             RemainingOrigins();
    unique_ptr<PlanMessage>         ExtractOriginMessage(OpPipeline* origin_pipe);
    unique_ptr<PlanMessage>         ExtractExecSubplan();
    vector<unique_ptr<PlanMessage>> ExtractSubplans();
  };

} // namespace: mohair


// ------------------------------
// Functions

namespace mohair {

  // >> Translation Functions
  //! Walks the operator tree starting at `rel_msg` and returns a `MohairOp` tree
  unique_ptr<MohairOp> MohairFrom(Plan* plan_msg);
  unique_ptr<MohairOp> MohairFrom(Plan* plan_msg, Rel* rel_msg);

  //! Constructs a `SystemPlan` from the given deserialized substrait `Plan`
  unique_ptr<SystemPlan> SystemPlanFrom(unique_ptr<PlanMessage>&& plan_msg);

  //! Constructs a `SystemPlan` from the given serialized substrait `Plan`
  unique_ptr<SystemPlan> SystemPlanFrom(const string& serialized_msg);

  //! Constructs a `SuperPlan` message for the given operator
  unique_ptr<SuperPlan>  SuperPlanFrom(MohairOp* mohair_op);

} // namespace: mohair
