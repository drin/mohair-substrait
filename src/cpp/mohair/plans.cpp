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
  OpPipeline& PipelineStage::CreatePipeline(MohairOp* sink_next) {
    auto new_pipeline = (
      pipelines.emplace_back(std::make_unique<OpPipeline>(sink, sink_next))
               .get()
    );

    return *new_pipeline;
  }


  // >> Implementations for `SystemPlan` methods

  string SystemPlan::FnNameForAnchor(uint64_t anchor_id) {
    if (fn_anchors.find(anchor_id) == fn_anchors.end()) {
      throw std::out_of_range(
        "Extension function anchor not registered: " + std::to_string(anchor_id)
      );
    }

    return fn_anchors[anchor_id];
  }

  const RelRoot& SystemPlan::RootRelation() const {
    return plan_msg->payload->relations(0).root();
  }

  PipelineStage&
  SystemPlan::CreatePipelineStage(MohairOp* sink, PipelineStage* next, size_t width) {
    auto new_stage = (
      pipeline_stages.emplace_back(std::make_unique<PipelineStage>(sink, next, width))
                     .get()
    );

    return *new_stage;
  }

  //! Populates a pipeline if the operator is a streaming operator
  void BuildPipelineWithOp( SystemPlan*    plan
                           ,PipelineStage& current_stage
                           ,OpPipeline&    current_pipe
                           ,MohairOp*      current_op) {
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

      PipelineStage* tmp_stage = &current_stage;

      tmp_stage->origin_names.push_back(origin_src->table_name);

      while (tmp_stage->next != nullptr) {
        tmp_stage = tmp_stage->next;
        tmp_stage->origin_names.push_back(origin_src->table_name);
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
  void SystemPlan::BuildPipelines() {
    size_t count_inputs = plan_root->GetOpArity();

    // Base case: Create a pipeline stage with the root operator as a sink.
    // NOTE: this base case is special to substrait (any operator can be a root operator)
    PipelineStage& final_stage = CreatePipelineStage(plan_root.get(), nullptr, count_inputs);

    for (size_t child_ndx = 0; child_ndx < count_inputs; ++child_ndx) {
      OpPipeline& stage_pipe = final_stage.CreatePipeline(nullptr);

      // Recurse through the input operator
      MohairOp* child_op = (plan_root->GetOpInputs()[child_ndx]).get();
      BuildPipelineWithOp(this, final_stage, stage_pipe, child_op);
    }
  }

  void SystemPlan::PrintPipelines() {
    std::cout << "System Plan [" << pipeline_stages.size() << " Pipeline Stages]"
              << std::endl
    ;

    for (size_t stage_ndx = 0; stage_ndx < pipeline_stages.size(); ++stage_ndx) {
      std::cout << "[stage | " << std::to_string(stage_ndx) << "]:" << std::endl;
      std::cout << pipeline_stages[stage_ndx]->ToString("  ")       << std::endl;
    }
  }

  //! Create mapping of function anchors in the query plan
  void SystemPlan::RegisterExtensionFunctions() {
    for (auto &plan_ext : plan_msg->payload->extensions()) {
      if (!plan_ext.has_extension_function()) { continue; }

      const auto anchor_id  = plan_ext.extension_function().function_anchor();
      fn_anchors[anchor_id] = plan_ext.extension_function().name();
    }
  }

  // Traversal functions for finding candidate plan splits
  optional<size_t> FindTallJoinLeaf(SystemPlan* sys_plan);
  optional<size_t> FindLongPipelineLeaf(SystemPlan* sys_plan);
  optional<size_t> FindWideJoin(SystemPlan* sys_plan);

  //! Finds a candidate `PlanSplit` given a decomposition algorithm (metric)
  unique_ptr<PlanSplit> PlanSplit::FindSplit(SystemPlan* sys_plan, DecomposeAlg method) {
    optional<size_t> stage_ndx { std::nullopt };

    switch (method) {
      case TallJoinLeaf: {
        stage_ndx = FindTallJoinLeaf(sys_plan);
        break;
      }

      // LongPipelineLeaf is currently default algorithm
      case LongPipelineLeaf: {
        stage_ndx = FindLongPipelineLeaf(sys_plan);
        break;
      }

      case WideJoinHead: {
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
      split_stage = sys_plan->pipeline_stages[stage_ndx.value()].get();
    }

    return std::make_unique<PlanSplit>(
       sys_plan
      ,sys_plan->plan_msg->payload.get()
      ,split_stage
      ,stage_ndx.value_or(0)
    );
  }

  //! Returns true if this instance can split the given `SystemPlan`
  bool PlanSplit::CanSplit() { return this->stage != nullptr; }

  //! Merge the given `PlanMessage` into this instance's superplan.
  bool PlanSplit::MergeSubplan(PlanMessage* subplan_msg) {
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
        *new_rel = *rel_itr;
    }

    // If any input to the anchor rel is a reference, we're done
    const vector<Rel*>& anchor_inputs = GetInputRels(superplan_anchor->mutable_rel());
    for (const Rel* input_rel : anchor_inputs) {
      if (input_rel->has_reference()) { return true; }
    }

    // Otherwise, we can move the anchor back (undo the reference)
    std::cout << "Anchor has had all subplans merged" << std::endl;
    MoveReferenceToOp(super_plan, superplan_mergerel->substrait_rel);

    return true;
  }

  //! Create a list of substrait messages given this instance's split information.
  vector<unique_ptr<PlanMessage>> PlanSplit::ExtractSubplans() {
    // This happens when the plan could not be split and we missed it
    MOHAIR_ASSERT("Split failed but we kept going anyway", superplan_mergerel != nullptr);

    // Initialize subplan messages from the superplan (ensures we propagate all context)
    vector<unique_ptr<PlanMessage>> subplan_msgs;
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

      subplan_msgs.push_back(PlanMessage::FromPlan(std::move(plan_copy)));
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

    return subplan_msgs;
  }

  //! Finds a `PlanSplit` matching a PipelineStage with a join as a sink and with at least
  //  one pipeline that has an origin operator (reads from a source relation)
  optional<size_t> FindTallJoinLeaf(SystemPlan* sys_plan) {
    optional<size_t> candidate_ndx { std::nullopt };
    size_t           peak_height   { 0 };

    size_t back_ndx = sys_plan->pipeline_stages.size();
    for (size_t stage_ndx = back_ndx; stage_ndx >= 0; --stage_ndx) {
      PipelineStage* stage = (sys_plan->pipeline_stages[stage_ndx]).get();

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

    size_t back_ndx = sys_plan->pipeline_stages.size();
    for (size_t stage_ndx = back_ndx; stage_ndx >= 0; --stage_ndx) {
      PipelineStage* stage = (sys_plan->pipeline_stages[stage_ndx]).get();

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
    if (sys_plan->pipeline_stages.empty()) { return std::nullopt; }

    optional<size_t> candidate_ndx { std::nullopt };
    size_t count_origins = 0;

    for (size_t stage_ndx = 0; stage_ndx < sys_plan->pipeline_stages.size(); ++stage_ndx) {
      PipelineStage* stage = (sys_plan->pipeline_stages[stage_ndx]).get();

      if (stage->width != 2)                          { continue; }
      if (stage->origin_names.size() < count_origins) { continue; }

      count_origins = stage->origin_names.size();
      candidate_ndx = stage_ndx;
    }

    return candidate_ndx;
  }


  // >> Translation functions

  //! Constructs a `SystemPlan` that wraps the given `PlanMessage`.
  //  This is an interface to creating a graph (query plan) of mohair operators.
  unique_ptr<SystemPlan> SystemPlanFrom(unique_ptr<PlanMessage>&& plan_msg) {
    // walk the top level relations until we find the root (should only be one)
    Plan* substrait_plan { plan_msg->payload.get() };
    int   root_ndx = FindPlanRoot(*substrait_plan);

    // set the plan root if not already set
    if (plan_msg->root_relndx < 0) {
      plan_msg->root_relndx   = root_ndx;
      plan_msg->root_relation = plan_msg->payload->mutable_relations(root_ndx);
    }

    // translate from the top level `Rel` to mohair operators
    auto mohair_plan = std::make_unique<SystemPlan>(
       std::move(plan_msg), MohairFrom(substrait_plan)
    );

    // then, walk the function anchors so we can associate anchor IDs and function names
    mohair_plan->RegisterExtensionFunctions();

    // then, walk the plan to build pipelines and discover characteristics
    mohair_plan->BuildPipelines();

    return mohair_plan;
  }

  //! Constructs a `SystemPlan` for the `PlanMessage` deserialized from `serialized_msg`.
  //  This is an interface to creating a graph (query plan) of mohair operators.
  unique_ptr<SystemPlan> SystemPlanFrom(const string& serialized_msg) {
    return SystemPlanFrom(PlanMessage::FromString(serialized_msg));
  }

} // namespace: mohair
