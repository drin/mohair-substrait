// ------------------------------
// License
//
// Copyright 2023 Aldrin Montana
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

#include <stdexcept>

#include "mohair/plans.hpp"


// ------------------------------
// Type Aliases

//  >> Protobuf framework types
using AnyMessage = google::protobuf::Any;


// ------------------------------
// Functions

namespace mohair {

  // >> Convenience functions
  Rel* FindMatchingRefRel(Rel* anchor_rel, uint32_t subplan_anchorid) {
    // Gather input rels
    constexpr size_t count_inputrels { 2 };
    vector<Rel*>     reference_rels;
    reference_rels.reserve(count_inputrels);

    switch (anchor_rel->rel_type_case()) {
      case Rel::RelTypeCase::kProject: {
        Rel* input_rel = anchor_rel->mutable_project()->mutable_input();
        if (input_rel->has_reference()) { reference_rels.push_back(input_rel); }

        break;
      }

      case Rel::RelTypeCase::kFilter: {
        Rel* input_rel = anchor_rel->mutable_filter()->mutable_input();
        if (input_rel->has_reference()) { reference_rels.push_back(input_rel); }

        break;
      }

      case Rel::RelTypeCase::kFetch: {
        Rel* input_rel = anchor_rel->mutable_fetch()->mutable_input();
        if (input_rel->has_reference()) { reference_rels.push_back(input_rel); }

        break;
      }


      case Rel::RelTypeCase::kSort: {
        Rel* input_rel = anchor_rel->mutable_sort()->mutable_input();
        if (input_rel->has_reference()) { reference_rels.push_back(input_rel); }

        break;
      }

      case Rel::RelTypeCase::kAggregate: {
        Rel* input_rel = anchor_rel->mutable_aggregate()->mutable_input();
        if (input_rel->has_reference()) { reference_rels.push_back(input_rel); }

        break;
      }


      case Rel::RelTypeCase::kJoin: {
        Rel* input_rel = anchor_rel->mutable_join()->mutable_left();
        if (input_rel->has_reference()) { reference_rels.push_back(input_rel); }

        input_rel = anchor_rel->mutable_join()->mutable_right();
        if (input_rel->has_reference()) { reference_rels.push_back(input_rel); }

        break;
      }

      case Rel::RelTypeCase::kCross: {
        Rel* input_rel = anchor_rel->mutable_cross()->mutable_left();
        if (input_rel->has_reference()) { reference_rels.push_back(input_rel); }

        input_rel = anchor_rel->mutable_join()->mutable_right();
        if (input_rel->has_reference()) { reference_rels.push_back(input_rel); }

        break;
      }

      case Rel::RelTypeCase::kHashJoin: {
        Rel* input_rel = anchor_rel->mutable_hash_join()->mutable_left();
        if (input_rel->has_reference()) { reference_rels.push_back(input_rel); }

        input_rel = anchor_rel->mutable_join()->mutable_right();
        if (input_rel->has_reference()) { reference_rels.push_back(input_rel); }

        break;
      }

      case Rel::RelTypeCase::kMergeJoin: {
        Rel* input_rel = anchor_rel->mutable_merge_join()->mutable_left();
        if (input_rel->has_reference()) { reference_rels.push_back(input_rel); }

        input_rel = anchor_rel->mutable_join()->mutable_right();
        if (input_rel->has_reference()) { reference_rels.push_back(input_rel); }

        break;
      }

      default: {
        return nullptr;
      }
    }

    // Find and return matching ReferenceRel
    for (size_t rel_ndx = 0; rel_ndx < reference_rels.size(); ++rel_ndx) {
      const ReferenceRel& ref_rel = reference_rels[rel_ndx]->reference();
      if (ref_rel.subtree_reference() == subplan_anchorid) {
        return reference_rels[rel_ndx];
      }
    }

    return nullptr;
  }



  // >> Implementations for `OpPipeline` methods

  const string OpPipeline::ToString(string prefix) {
    stringstream pipeline_stream;

    pipeline_stream << prefix << "↩ [ " << sink->ViewStr() << " ]    ";

    for (size_t op_ndx = 0; op_ndx < pipe_ops.size(); ++op_ndx) {
      pipeline_stream << "  ↢ " << pipe_ops[op_ndx]->ViewStr();
    }

    pipeline_stream << "\t⇐ " << source->ViewStr() << std::endl;

    return pipeline_stream.str();
  }

  //! Returns the number of operators in the pipeline plus 2 (for sink and source)
  size_t OpPipeline::Size()                   { return 2 + pipe_ops.size();  }
  void   OpPipeline::AddOp(MohairOp* pipe_op) { pipe_ops.push_back(pipe_op); }
  void   OpPipeline::SetSource(MohairOp* src) { source = src;                }

  SubstraitOp* OpPipeline::GetSourceNext() {
    if (not pipe_ops.empty()) { return pipe_ops.back(); }
    return nullptr;
  }


  // >> Implementations for `PipelineStage` methods
  //!
  const string PipelineStage::ToString(string prefix) {
    stringstream stage_stream;

    // output the source names first
    if (not origin_names.empty()) {
      stage_stream << prefix << "[ " << origin_names[0];
      for (size_t origin_ndx = 1; origin_ndx < origin_names.size(); ++origin_ndx) {
        stage_stream << ", " << origin_names[origin_ndx];
      }
      stage_stream << " ]  ⇤" << std::endl;
    }

    // then output the pipelines
    for (size_t pipe_ndx = 0; pipe_ndx < pipelines.size(); ++pipe_ndx) {
      stage_stream << pipelines[pipe_ndx]->ToString(prefix + "  ") << std::endl;
    }

    return stage_stream.str();
  }

  //! Create a pipeline flowing to the same sink and with a pointer to the sink's
  //  downstream operator (where output flows to); this operator is considered when
  //  splitting the plan.
  OpPipeline& PipelineStage::CreatePipeline(SubstraitOp* sink_next) {
    auto& new_pipeline = pipelines.emplace_back(
      std::make_unique<OpPipeline>(sink, sink_next)
    );

    return *(new_pipeline.get());
  }


  // >> Implementations for `PlanPipeline` methods
  PipelineStage&
  PlanPipeline::CreatePipelineStage(SubstraitOp* sink, PipelineStage* next, size_t width) {
    auto new_stage = (
      stages.emplace_back(std::make_unique<PipelineStage>(sink, next, width))
            .get()
    );

    return *new_stage;
  }

  //! Populates a pipeline if the operator is a streaming operator
  void BuildPipelineWithOp( SystemPlan*    plan
                           ,PipelineStage& current_stage
                           ,OpPipeline&    current_pipe
                           ,SubstraitOp*   current_op) {
    while (not current_op->IsSink() and not current_op->IsOrigin()) {
      current_pipe.AddOp(current_op);
      current_op = (current_op->GetOpInputs()[0]).get();
    }

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

    // Create a new stage and initial pipeline
    // NOTE: "upstream" means closer to the __origin__ of data flow (source table)
    size_t         count_inputs   = current_op->GetOpArity();
    PipelineStage& upstream_stage = plan->CreatePipelineStage(
      current_op, &current_stage, count_inputs
    );

    // For each input operator, create a pipeline and recurse
    for (size_t child_ndx = 0; child_ndx < count_inputs; ++child_ndx) {
      MohairOp* child_op  = (current_op->GetOpInputs()[child_ndx]).get();
      MohairOp* sink_next = nullptr;
      if (not current_pipe.pipe_ops.empty()) { sink_next = current_pipe.pipe_ops.back(); }

      OpPipeline& upstream_pipe = upstream_stage.CreatePipeline(sink_next);
      BuildPipelineWithOp(plan, upstream_stage, upstream_pipe, child_op);

      // When a pipeline is fully built, see if it's the longest for the stage
      if (upstream_pipe.Size() > upstream_stage.length) {
        upstream_stage.length = upstream_pipe.Size();
      }
    }
  }

  //! Creates all pipelines for the plan
  void PlanPipeline PlanPipeline::Build(SubstraitPlan* src_plan) {
    PlanPipeline plan_pipes { src_plan };
    SubstraitOp* plan_root  = src_plan->plan_root.get();

    // Create a pipeline stage with the root operator as a sink.
    size_t         count_inputs = plan_root->GetOpArity().value();
    PipelineStage& final_stage  = CreatePipelineStage(plan_root, nullptr, count_inputs);

    plan_root->BuildPipelines(*this, final_stage);
  }

  void PlanPipeline::PrintPipelines() {
    std::cout << "Plan Pipeline [" << stages.size() << " Stages]" << std::endl;

    for (size_t stage_ndx = 0; stage_ndx < stages.size(); ++stage_ndx) {
      std::cout << "[stage | " << std::to_string(stage_ndx) << "]:" << std::endl;
      std::cout << stages[stage_ndx]->ToString("  ")       << std::endl;
    }
  }

  // Traversal functions for finding candidate plan splits
  optional<size_t> FindSplitOverride(SystemPlan* sys_plan);
  optional<size_t> FindTallJoinLeaf(SystemPlan* sys_plan);
  optional<size_t> FindLongPipelineLeaf(SystemPlan* sys_plan);
  optional<size_t> FindWideJoin(SystemPlan* sys_plan);

  //! Finds a candidate `PlanSplit` given a decomposition algorithm (metric)
  unique_ptr<PlanSplit>
  PlanSplit::FindSplit(SystemPlan* sys_plan, const DecomposeAlg& method) {
    MohairStartTS(FindPlanSplit);
    optional<size_t> stage_ndx { std::nullopt };

    switch (method) {
      // Do not find a split candidate if no algorithm is specified
      case DecomposeAlg::None:
      case DecomposeAlg::Eager: {
        optional<size_t> override_ndx = FindSplitOverride(sys_plan);
        if (override_ndx.has_value()) { stage_ndx = override_ndx.value(); }
        break;
      }

      case DecomposeAlg::TallJoinLeaf: {
        stage_ndx = FindTallJoinLeaf(sys_plan);
        break;
      }

      // LongPipelineLeaf is currently default algorithm
      case DecomposeAlg::LongPipelineLeaf: {
        stage_ndx = FindLongPipelineLeaf(sys_plan);
        break;
      }

      case DecomposeAlg::WideJoinHead: {
        stage_ndx = FindWideJoin(sys_plan);
        break;
      }

      default: {
        std::cerr << "Unknown decomposition method" << std::endl;
        return nullptr;
      }
    }

    PipelineStage* split_stage { nullptr };
    if (stage_ndx.has_value()) {
      split_stage = sys_plan->stages[stage_ndx.value()].get();
    }

    MohairStopTS(FindPlanSplit);
    MohairLogTimestamps(FindPlanSplit);

    return std::make_unique<PlanSplit>(
       sys_plan
      ,sys_plan->plan_msg->payload.get()
      ,split_stage
      ,stage_ndx.value_or(0)
    );
  }

  //! Returns true if this instance can split the given `SystemPlan`
  bool PlanSplit::CanSplit() { return this->stage != nullptr; }

  //! Merge the given `SkyResultRel` into this instance's superplan.
  //  We use this instead of MergeSubplan if we don't need to merge a whole plan.
  bool PlanSplit::MergeResultRel(Rel* result_rel) {
    // This should only be valid if:
    // - we have a single merge rel
    // - inputs to the merge rel has no references
    if (super_plan->relations_size() != 2) {
      std::cerr << "PlanSplit cannot merge result if it does not have only one anchor"
                << std::endl
      ;
      return false;
    }

    int  mergerel_ndx = super_plan->relations_size() - 1;
    Rel* merge_rel    = super_plan->mutable_relations(mergerel_ndx)->mutable_rel();

    const vector<Rel*>& anchor_inputs = GetInputRels(merge_rel);
    for (const Rel* input_rel : anchor_inputs) {
      if (input_rel->has_reference()) {
        std::cerr << "PlanSplit cannot merge result if it has unresolved references"
                  << std::endl
        ;
        return false;
      }
    }

    // Replace the merge rel and move the merge rel back into the root subtree
    MoveRelOp(result_rel, merge_rel);
    MoveReferenceToOp(super_plan, superplan_mergerel->substrait_rel);

    return true;
  }

  //! Merge the given `SubstraitPlan` into this instance's superplan.
  bool PlanSplit::MergeSubplan(SubstraitPlan* subplan_msg) {
    MohairStartTS(MergePlanSplit);

    // >> Recover (and validate) links between the superplan and subplan
    // Search in subplan message for a `SuperPlan` reference
    SuperPlan refrel_superplan;
    bool      has_superplan_ref { false };
    const auto& subplan_planext = subplan_msg->payload->advanced_extensions();
    for (const auto& optimization_msg : subplan_planext.optimization()) {
      if (not optimization_msg.Is<SuperPlan>()) { continue; }

      optimization_msg.UnpackTo(&refrel_superplan);
      has_superplan_ref = true;
    }

    if (not has_superplan_ref) {
      std::cerr << "Failed to find reference [subplan -> superplan]" << std::endl;
      return false;
    }

    // Search superplan relations (in reverse) for a `ReferenceRel` pointing to subplan
    PlanRel* superplan_anchor { nullptr };
    Rel*     refrel_subplan   { nullptr };

    uint32_t subplan_anchorid { subplan_msg->root_relation->subtree_anchor() };
    auto     superplan_rels = super_plan->mutable_relations();
    auto     mergerel_itr   = superplan_rels->end();
    for (--mergerel_itr; mergerel_itr != superplan_rels->begin(); --mergerel_itr) {
      if (mergerel_itr->has_root()) { continue; }

      // This checks the inputs to mergerel_itr for a matching `ReferenceRel`
      refrel_subplan = FindMatchingRefRel(mergerel_itr->mutable_rel(), subplan_anchorid);

      if (refrel_subplan != nullptr) {
        superplan_anchor = &(*mergerel_itr);

        if (refrel_superplan.mergerel_reference() != superplan_anchor->subtree_anchor()) {
          std::cerr << "Expected SuperPlan anchor ID to match. Actual: ["
                    << refrel_superplan.mergerel_reference() << " != "
                    << superplan_anchor->subtree_anchor()    << "]"
                    << std::endl
          ;
          return false;
        }
      }
    }

    if (superplan_anchor == nullptr) {
      std::cerr << "Failed to find reference [superplan -> subplan]" << std::endl;
      PrintSubstraitPlan(subplan_msg->payload.get());
      return false;
    }

    // >> Do the merging

    // Move the root of the subplan
    Rel* subplan_rootrel = subplan_msg->root_relation->mutable_root()->mutable_input();
    MoveRelOp(subplan_rootrel, refrel_subplan);

    // Move the remaining subtrees
    auto subplan_rels = subplan_msg->payload->mutable_relations();
    for (auto rel_itr = subplan_rels->begin(); rel_itr != subplan_rels->end(); ++rel_itr) {
        // Skip the root relation as we've already moved it
        if (rel_itr->has_root()) { continue; }

        // Add each remaining relation to the superplan
        PlanRel* new_rel = super_plan->add_relations();
        new_rel->CopyFrom(*rel_itr);
    }

    // If any input to the anchor rel is a reference, we're done
    const vector<Rel*>& anchor_inputs = GetInputRels(superplan_anchor->mutable_rel());
    for (const Rel* input_rel : anchor_inputs) {
      if (input_rel->has_reference()) {
        MohairStopTS(MergePlanSplit);
        MohairLogTimestamps(MergePlanSplit);

        return true;
      }
    }

    // Otherwise, we can move the anchor back (undo the reference)
    std::cout << "Anchor has had all subplans merged" << std::endl;
    MoveReferenceToOp(super_plan, superplan_mergerel->substrait_rel);

    MohairStopTS(MergePlanSplit);
    MohairLogTimestamps(MergePlanSplit);

    return true;
  }

  //! Create a single substrait message from this split and the given stage
  unique_ptr<SubstraitPlan> PlanSplit::ExtractExecSubplan() {
    MohairStartTS(ExtractPlanSplitExec);

    // This happens when the plan could not be split and we missed it
    MOHAIR_ASSERT("Split failed but we kept going anyway", superplan_mergerel != nullptr);

    // Initialize execution subplan from the superplan (ensures we propagate all context)
    // TODO: make this copy more efficient
    auto exec_plan = std::make_unique<Plan>();
    exec_plan->CopyFrom(*super_plan);

    // Clear output names in the execution subplan, or it'll confuse the engine
    const PlanRel& exec_root = exec_plan->relations(0);
    if (not exec_root.has_root()) {
      throw std::runtime_error("ExecSubplan's first PlanRel is not a root");
    }
    if (not exec_root.root().names().empty()) {
      exec_plan->mutable_relations(0)->mutable_root()->clear_names();
    }

    // Move the op to an anchor (PlanRel) for future merging (modifies the anchor rel)
    PlanRel* superplan_anchor = MoveOpToReference(super_plan, superplan_mergerel->substrait_rel);
    unique_ptr<SuperPlan> refrel_superplan = CreateSuperPlanRel(superplan_anchor);

    // Then, create the execution subplan message to contain the subplan
    unique_ptr<SubstraitPlan> exec_subplan { SubstraitPlan::FromPlan(std::move(exec_plan)) };

    // Move the op tree from the superplan to the subplan
    PlanRel* subplan_planroot = exec_subplan->root_relation;
    Rel*     new_rootrel      = subplan_planroot->mutable_root()->mutable_input();
    Rel*     old_rootrel      = superplan_anchor->mutable_rel();

    MoveRelOp(old_rootrel, new_rootrel);
    CreateReferenceRel(old_rootrel, subplan_planroot);

    // Pack the `SuperPlan` message so we can match pushback plans to the merge rel
    Plan* subplan = exec_subplan->payload.get();

    AdvancedExtension* subplan_planext  = subplan->mutable_advanced_extensions();
    AnyMessage*        optimization_msg = subplan_planext->add_optimization();
    optimization_msg->PackFrom(*refrel_superplan);

    MohairStopTS(ExtractPlanSplitExec);
    MohairLogTimestamps(ExtractPlanSplitExec);

    return exec_subplan;
  }

  //! Create a list of substrait messages given this instance's split information.
  vector<unique_ptr<SubstraitPlan>> PlanSplit::ExtractSubplans() {
    MohairStartTS(ExtractPlanSplit);

    // This happens when the plan could not be split and we missed it
    MOHAIR_ASSERT("Split failed but we kept going anyway", superplan_mergerel != nullptr);

    // Initialize subplan messages from the superplan (ensures we propagate all context)
    vector<unique_ptr<SubstraitPlan>> subplan_msgs;
    subplan_msgs.reserve(subplan_rootrels.size());

    for (size_t subplan_ndx = 0; subplan_ndx < subplan_rootrels.size(); ++subplan_ndx) {
      auto plan_copy = std::make_unique<Plan>();
      plan_copy->CopyFrom(*super_plan);

      // It is required that we clear the output names in the subplan messages;
      // otherwise, we will always project the wrong columns from subplans
      int root_ndx = FindPlanRoot(*plan_copy);
      if (not plan_copy->relations(root_ndx).root().names().empty()) {
        plan_copy->mutable_relations(root_ndx)->mutable_root()->clear_names();
      }

      subplan_msgs.push_back(SubstraitPlan::FromPlan(std::move(plan_copy)));
    }

    // Move the op to an anchor (PlanRel) for future merging (modifies the anchor rel)
    PlanRel* superplan_anchor = MoveOpToReference(super_plan, superplan_mergerel->substrait_rel);
    unique_ptr<SuperPlan> refrel_superplan = CreateSuperPlanRel(superplan_anchor);

    // Then, modify subplan messages and insert reference rels into the superplan message
    for (size_t subplan_ndx = 0; subplan_ndx < subplan_rootrels.size(); ++subplan_ndx) {
      // Move the op tree from the superplan to the subplan
      PlanRel* subplan_planroot = subplan_msgs[subplan_ndx]->root_relation;
      Rel*     new_rootrel      = subplan_planroot->mutable_root()->mutable_input();
      Rel*     old_rootrel      = subplan_rootrels[subplan_ndx]->substrait_rel;

      MoveRelOp(old_rootrel, new_rootrel);
      CreateReferenceRel(old_rootrel, subplan_planroot);

      // Pack the `SuperPlan` message so we can match pushback plans to the merge rel
      Plan* subplan = subplan_msgs[subplan_ndx]->payload.get();

      AdvancedExtension* subplan_planext  = subplan->mutable_advanced_extensions();
      AnyMessage*        optimization_msg = subplan_planext->add_optimization();
      optimization_msg->PackFrom(*refrel_superplan);
    }

    MohairStopTS(ExtractPlanSplit);
    MohairLogTimestamps(ExtractPlanSplit);

    return subplan_msgs;
  }

  //! Finds the first pipeline stage of SystemPlan with a split annotation.
  //  This function returns the index of the stage as a split and clears the split
  //  annotation. This allows subsequent calls to find later splits.
  optional<size_t> FindSplitOverride(SystemPlan* sys_plan) {
    optional<size_t> override_ndx { std::nullopt };

    size_t count_stages { sys_plan->stages.size() };
    for (size_t stage_ndx = 0; stage_ndx < count_stages; ++stage_ndx) {
      PipelineStage* stage = (sys_plan->stages[stage_ndx]).get();

      if (stage->sink->substrait_rel->has_splitoverride()) {
        override_ndx = stage_ndx;
        break;
      }
    }

    return override_ndx;
  }

  //! Finds a `PlanSplit` matching a PipelineStage with a join as a sink and with at least
  //  one pipeline that has an origin operator (reads from a source relation)
  optional<size_t> FindTallJoinLeaf(SystemPlan* sys_plan) {
    optional<size_t> candidate_ndx { std::nullopt };
    size_t           peak_height   { 0 };

    // NOTE: done when stage_ndx underflows to max value
    size_t back_ndx  = sys_plan->stages.size();
    for (size_t stage_ndx = back_ndx - 1; stage_ndx < back_ndx; --stage_ndx) {
      PipelineStage* stage = (sys_plan->stages[stage_ndx]).get();

      if (stage->width != 2)            { continue; }
      if (stage->length <= peak_height) { continue; }

      if (   stage->pipelines[0]->source->IsOrigin()
          or stage->pipelines[1]->source->IsOrigin()) {
        peak_height   = stage->length;
        candidate_ndx = stage_ndx;
      }
    }

    return candidate_ndx;
  }

  //! Finds a `PlanSplit` matching a PipelineStage with only 1 origin relation and having
  //  the most operators in the lineage (any number of stages without joins)
  optional<size_t> FindLongPipelineLeaf(SystemPlan* sys_plan) {
    optional<size_t> candidate_ndx { std::nullopt };
    size_t           peak_height   { 0 };

    // NOTE: done when stage_ndx underflows to max value
    size_t back_ndx  = sys_plan->stages.size();
    for (size_t stage_ndx = back_ndx - 1; stage_ndx < back_ndx; --stage_ndx) {
      PipelineStage* stage = (sys_plan->stages[stage_ndx]).get();

      if (stage->origin_names.size() > 1) { continue; }

      size_t         total_stagelen = 0;
      PipelineStage* current_stage  = stage;
      while (current_stage != nullptr and current_stage->origin_names.size() == 1) {
        total_stagelen += current_stage->length;
        current_stage   = current_stage->next;
      }

      if (total_stagelen > peak_height) {
        peak_height = total_stagelen;
        candidate_ndx = stage_ndx;
      }
    }

    return candidate_ndx;
  }

  //! Finds the Join operator with the most width (origin names)
  optional<size_t> FindWideJoin(SystemPlan* sys_plan) {
    if (sys_plan->stages.empty()) { return std::nullopt; }

    optional<size_t> candidate_ndx { std::nullopt };
    size_t count_origins = 0;

    for (size_t stage_ndx = 0; stage_ndx < sys_plan->stages.size(); ++stage_ndx) {
      PipelineStage* stage = (sys_plan->stages[stage_ndx]).get();

      if (stage->width != 2)                          { continue; }
      if (stage->origin_names.size() < count_origins) { continue; }

      count_origins = stage->origin_names.size();
      candidate_ndx = stage_ndx;
    }

    return candidate_ndx;
  }


  // >> Translation functions

  //! Constructs a `SystemPlan` that wraps the given `SubstraitPlan`.
  //  This is an interface to creating a graph (query plan) of mohair operators.
  unique_ptr<SystemPlan> SystemPlanFrom(unique_ptr<SubstraitPlan>&& plan_msg) {
    // walk the top level relations until we find the root (should only be one)
    Plan* substrait_plan { plan_msg->payload.get() };
    int   root_ndx = FindPlanRoot(*substrait_plan);

    // set the plan root if not already set
    if (plan_msg->root_relndx < 0) {
      plan_msg->root_relndx   = root_ndx;
      plan_msg->root_relation = plan_msg->payload->mutable_relations(root_ndx);
    }

    // Parse system plan
    unique_ptr<SystemPlan> mohair_plan;
    MohairLogPerf(ParseSysPlan,
      {
        // translate from the top level `Rel` to mohair operators
        mohair_plan = std::make_unique<SystemPlan>(
          std::move(plan_msg), MohairFrom(substrait_plan)
        );

        // then, walk the function anchors so we can associate anchor IDs and function names
        mohair_plan->RegisterExtensionFunctions();
      }
    );

    // Construct pipelines
    MohairLogPerf(ConstructPipelines,
      {
        // then, walk the plan to build pipelines and discover characteristics
        mohair_plan->BuildPipelines();
      }
    );

    return mohair_plan;
  }

  //! Constructs a `SystemPlan` for the `SubstraitPlan` deserialized from `serialized_msg`.
  //  This is an interface to creating a graph (query plan) of mohair operators.
  unique_ptr<SystemPlan> SystemPlanFrom(const string& serialized_msg) {
    return SystemPlanFrom(SubstraitPlan::FromString(serialized_msg));
  }

} // namespace: mohair


// ------------------------------
// Method implementations

namespace mohair {

  // >> Implementations for SplitAnchor

  //! Merges the referenced subplan anchor back into its original location (merge rel)
  void SplitAnchor::MergeSubplan() {
    // Avoid accidentally merging multiple times
    if (is_merged) { return; }

    // Grab pointers using our indices
    Rel*     merge_rel      = plan->super_mergerels[merge_relndx];
    PlanRel* subplan_anchor = plan->mutable_relations(anchor_relndx);

    // Validate the merge rel references the subplan anchor
    uint32_t anchor_id = subplan_anchor.subtree_anchor();
    MOHAIR_ASSERT(
       "SplitAnchor is invalid: merge rel and subplan anchor do not match"
      ,anchor_id != merge_rel->reference().subtree_reference()
    );

    // Move the anchor operator back into its original location
    Rel* anchor_rel = anchor_op->MoveToRel(merge_rel);
    MOHAIR_ASSERT(
       "Expected anchor rel to match input into subplan anchor"
      ,anchor_rel != subplan_anchor->mutable_rel()
    );

    // Remove the subplan anchor (should now be empty)
    auto plan_rootrels = plan->mutable_relations();
    plan_rootrels->erase(plan_rootrels->begin() + anchor_relndx);

    // Untrack the merge rel (should now contain the anchor operator)
    plan->super_mergerels.erase(plan->super_mergerels.begin() + merge_relndx);

    is_merged = true;
  }

  // >> API for sending the SubstraitPlan to storage or the network
  //! Serialize the contained Plan message to binary and write to the specified file
  bool SubstraitPlan::SerializeToFile(const char* out_fpath) {
    std::fstream out_fstream { OutputStreamForFile(out_fpath); }
    if (not out_fstream.is_open()) {
      std::cerr << "Failed to open output file: " << out_fpath << std::endl;
      return false;
    }

    if (not this->plan->SerializeToOstream(&out_fstream)) {
      std::cerr << "Failed to serialize to output file: " << out_fpath << std::endl;
      return false;
    }

    return true;
  }

  bool SubstraitPlan::SerializeToFile(const string& out_fpath) {
    return this->SerializeToFile(out_fpath.data());
  }

  //! Serialize the contained Plan message to binary and store in the given string
  bool SubstraitPlan::SerializeToString(string* out_str) {
    if (not this->plan->SerializeToString(out_str)) {
      std::cerr << "Failed to serialize Plan message." << std::endl;
      return false;
    }

    return true;
  }

  bool SubstraitPlan::SerializeToString(string& out_str) {
    return this->SerializeToString(&out_str);
  }


  // >> API for inspecting the contained substrait plan
  //! Returns the serialized binary of the contained Plan message or an empty string
  string SubstraitPlan::Serialize() {
    string msg_serialized;
    if (this->SerializeToString(&msg_serialized)) { return msg_serialized; }

    return string {};
  }

  //! Returns the stringification of the contained Plan message or an empty string
  string SubstraitPlan::StringifyPlan() {
    std::optional<string> plan_str = StringifyMessage(*(this->plan));
    if (plan_str.has_value()) { return plan_str.value(); }

    return string {};
  }

  //! Prints the stringification of the contained Plan message
  void SubstraitPlan::PrintPlan() { PrintMessage(*(this->plan)); }

  //! Access the function name associated with the given anchor
  string SubstraitPlan::NameForFnAnchor(uint64_t anchor_id) {
    if (fn_anchors.find(anchor_id) == fn_anchors.end()) {
      throw std::out_of_range(
        "Unregistered function anchor: " + std::to_string(anchor_id)
      );
    }

    return fn_anchors[anchor_id];
  }

  // >> API for modifying the contained substrait plan
  //! Create mapping of function anchors in the query plan
  void SubstraitPlan::RegisterExtensionFunctions() {
    for (const auto& plan_ext : plan->extensions()) {
      if (!plan_ext.has_extension_function()) { continue; }

      const auto anchor_id  = plan_ext.extension_function().function_anchor();
      fn_anchors[anchor_id] = plan_ext.extension_function().name();
    }
  }


  //! Copies result aliases from RelRoot then clears them
  void SubstraitPlan::PushdownResultAliases() {
    RelRoot* root = root_rel->mutable_root();

    if      (root->names().empty()) { return; }
    else if (plan_root == nullptr)  { return; }

    plan_root->SetAliases(root->names());
    root->clear_names();
  }


  //! Move an operator into a PlanRel and create a ReferenceRel to it
  SplitAnchor SubstraitPlan::AnchorForSplit(SubstraitOp* anchor_op) {
    // Create PlanRel for subplan anchor, then move the anchor operator
    int32_t  anchor_relndx  = plan->relations_size();
    PlanRel* subplan_anchor = plan->add_relations();
    Rel*     merge_rel      = anchor_op->MoveToRel(subplan_anchor->mutable_rel());

    // Set the anchor and reference IDs
    uint32_t anchor_id = ++UUIDGenerator;
    subplan_anchor->set_subtree_anchor(anchor_id);
    merge_rel->mutable_reference()->set_subtree_reference(anchor_id);

    // Track the merge rel for convenience
    size_t merge_relndx = super_mergerels.size();
    super_mergerels.push_back(merge_rel);

    // sanity check the subplan anchor
    MOHAIR_ASSERT(
      "Unexpected anchor ID for subplan anchor"
      ,anchor_id == plan->relations(anchor_relndx).subtree_anchor()
    );

    return SplitAnchor { this, anchor_relndx, merge_relndx, anchor_op };
  }


  // >> API for construction of SubstraitPlan (Static methods)
  //! Constructs a SubstraitPlan from a deserialized Plan
  unique_ptr<SubstraitPlan> SubstraitPlan::FromPlan(unique_ptr<Plan> plan) {
    if (plan == nullptr) { return nullptr; }

    int    root_relndx { -1 };
    size_t count_roots {  0 };

    // Find the PlanRel that has a RelRoot and validate there is only 1
    for (int rel_ndx = 0; rel_ndx < substrait_plan.relations_size(); ++rel_ndx) {
      if (not substrait_plan.relations(rel_ndx).has_root()) { continue; }

      root_relndx = rel_ndx;
      ++count_roots;
    }

    if (count_roots != 1) {
      std::cerr << "Invalid Plan: "
                << "found [" << count_roots << "] root PlanRels"
                << std::endl
      ;
      return nullptr;
    }

    return std::make_unique<SubstraitPlan>(std::move(plan), root_relndx);
  }

  //! Constructs a SubstraitPlan from a serialized Plan
  unique_ptr<SubstraitPlan> SubstraitPlan::FromMessage(const string& plan_msg) {
    return this->FromPlan(ParsePlanMessage(plan_str));
  }

  //! Constructs a SubstraitPlan from a file containing a serialized Plan
  unique_ptr<SubstraitPlan> SubstraitPlan::FromFile(const char* plan_fpath) {
    return this->FromPlan(ParsePlanFile(plan_fpath));
  }

  unique_ptr<SubstraitPlan> SubstraitPlan::FromFile(const string& plan_fpath) {
    return SubstraitPlan::FromFile(plan_fpath.data());
  }

} // namespace: mohair
