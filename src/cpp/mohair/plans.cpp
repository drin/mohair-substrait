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
      OpPipeline& final_pipe = final_stage.CreatePipeline(nullptr);

      // Recurse through the input operator
      MohairOp* child_op = (plan_root->GetOpInputs()[child_ndx]).get();
      BuildPipelineWithOp(this, final_stage, final_pipe, child_op);
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
  PipelineStage* FindTallJoinLeaf(SystemPlan* sys_plan);
  PipelineStage* FindLongPipelineLeaf(SystemPlan* sys_plan);

  //! Finds a candidate `PlanSplit` given a decomposition algorithm (metric)
  unique_ptr<PlanSplit> PlanSplit::FindSplit(SystemPlan* sys_plan, DecomposeAlg method) {
    switch (method) {
      case TallJoinLeaf: {
        return std::make_unique<PlanSplit>(FindTallJoinLeaf(sys_plan));
      }

      // LongPipelineLeaf is currently default algorithm
      case LongPipelineLeaf: {
        return std::make_unique<PlanSplit>(FindLongPipelineLeaf(sys_plan));
      }

      default: {
        std::cerr << "Unknown decomposition method" << std::endl;
        return nullptr;
      }
    }
  }

  /**
   * 
   *
   * For each subplan, the general process is to:
   *  1. create a copy of the original substrait message
   *  2. replace the original root rel with the root rel of the sub-plan
   *  3. set an anchor (rel in super-plan), which identifies the sink for the sub-plan.
   * 
   * The order of 2 and 3 is unimportant (we already have references to both in
   * PlanSplit). Step 2 is what will allow the next consumer to only see the sub-plan.
   * Step 3 will allow us to make merging of the pushback plan trivial (we will be able to
   * use operator equality).
   */
  //! Create a substrait message for each subplan of derived from a PlanSplit.
  vector<unique_ptr<PlanMessage>>
  PlanSplit::SubplansFor(PlanMessage* plan_msg) {
    // Get the anchor op and initialize some variables
    MohairOp* anchor_op     = this->merge_rel;
    size_t    count_inputs  = anchor_op->GetOpArity();

    // Create a superplan message that we'll reuse for each subplan message
    unique_ptr<SuperPlan> superplan_msg { SuperPlanFrom(anchor_op) };

    // Initialize the list of messages to return
    vector<unique_ptr<PlanMessage>> subplan_msgs;
    subplan_msgs.reserve(count_inputs);

    // Create a substrait message for each input to the anchor
    unique_ptr<MohairOp>* anchor_inputs = anchor_op->GetOpInputs();
    for (size_t input_ndx = 0; input_ndx < count_inputs; ++input_ndx) {
      MohairOp* input_op        = anchor_inputs[input_ndx].get();
      Rel*      subplan_rootrel = input_op->substrait_rel;

      // Create a copy of the original substrait message that we can modify
      auto subplan_msg = std::make_unique<Plan>();
      subplan_msg->CopyFrom(*(plan_msg->payload));

      // Set the `SuperPlan` message and replace the super-plan root
      auto subplan_oldroot = subplan_msg->mutable_relations(plan_msg->root_relndx)->mutable_root();
      auto subplan_planext = subplan_msg->mutable_advanced_extensions();

      // Pack the anchor message into an `Any` message as an "optimization"
      AnyMessage* optimization_msg = subplan_planext->add_optimization();
      optimization_msg->PackFrom(*(superplan_msg));

      subplan_oldroot->mutable_input()->CopyFrom(*subplan_rootrel);

      // Add a `SubstraitMessage` that wraps the `Plan` message
      subplan_msgs.push_back(
        std::make_unique<SubstraitMessage>(std::move(subplan_msg), plan_msg->root_relndx)
      );
    }

    return subplan_msgs;
  }

  //! Finds a `PlanSplit` matching a PipelineStage with a join as a sink and with at least
  //  one pipeline that has an origin operator (reads from a source relation)
  PipelineStage* FindTallJoinLeaf(SystemPlan* sys_plan) {
    PipelineStage* candidate   = nullptr;
    size_t         peak_height = 0;

    // An element in PlanVec may be null if we previously moved it
    size_t back_ndx = sys_plan->pipeline_stages.size();
    for (size_t stage_ndx = back_ndx; stage_ndx >= 0; --stage_ndx) {
      PipelineStage* stage = (sys_plan->pipeline_stages[stage_ndx]).get();

      if (stage->width == 1)            { continue; }
      if (stage->length <= peak_height) { continue; }

      if (   stage->pipelines[0]->source->IsOrigin()
          or stage->pipelines[1]->source->IsOrigin()) {
        peak_height = stage->length;
        candidate   = stage;
      }
    }

    return candidate;
  }

  //! Finds a `PlanSplit` matching a PipelineStage with only 1 origin relation and having
  //  the most operators in the lineage (any number of stages without joins)
  PipelineStage* FindLongPipelineLeaf(SystemPlan* sys_plan) {
    PipelineStage* candidate   = nullptr;
    size_t         peak_height = 0;

    // An element in PlanVec may be null if we previously moved it
    size_t back_ndx = sys_plan->pipeline_stages.size();
    for (size_t stage_ndx = back_ndx; stage_ndx >= 0; --stage_ndx) {
      PipelineStage* stage = (sys_plan->pipeline_stages[stage_ndx]).get();

      if (stage->origin_names.size() > 1) { continue; }

      // NOTE: theoretically can recompute a lot; but unlikely in practice
      size_t         total_stagelen = 0;
      PipelineStage* current_stage  = stage;
      while (current_stage != nullptr and current_stage->origin_names.size() == 1) {
        total_stagelen += current_stage->length;
        current_stage   = current_stage->next;
      }

      if (total_stagelen > peak_height) {
        peak_height = total_stagelen;
        candidate   = current_stage;
      }
    }

    return candidate;
  }


  // >> Translation functions

  //! Constructs a `SystemPlan` that wraps the given `PlanMessage`.
  //  This is an interface to creating a graph (query plan) of mohair operators.
  unique_ptr<SystemPlan> SystemPlanFrom(unique_ptr<PlanMessage>&& plan_msg) {
    // walk the top level relations until we find the root (should only be one)
    int root_ndx = FindPlanRoot(*(plan_msg->payload));

    // set the plan root if not already set
    if (plan_msg->root_relndx < 0) {
      plan_msg->root_relndx   = root_ndx;
      plan_msg->root_relation = plan_msg->payload->mutable_relations(root_ndx);
    }

    // translate from the top level `Rel` to mohair operators
    Rel* substrait_rootrel { plan_msg->root_relation->mutable_root()->mutable_input() };
    auto mohair_plan = std::make_unique<SystemPlan>(
       std::move(plan_msg), MohairFrom(substrait_rootrel)
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
    return SystemPlanFrom(SubstraitMessage::FromString(serialized_msg));
  }

} // namespace: mohair
