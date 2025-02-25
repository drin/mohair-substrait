// ------------------------------
// License
//
// Copyright 2023-2025 Aldrin Montana
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

// >> Standard libs
#include <filesystem>

// >> Internal
#include "mohair.hpp"
#include "mohair/plans.hpp"


// ------------------------------
// Aliases

// >> Namespace Aliases
namespace fs = std::filesystem;

// >> Type Aliases
using std::unique_ptr;
using std::string;
using std::vector;

using mohair::PlanMessage;
using mohair::SystemPlan;
using mohair::PipelineStage;
using mohair::Plan;
using mohair::PlanSplit;
using mohair::DecomposeAlg;

// >> Function Aliases
using mohair::PrintSubstraitPlan;
using mohair::PrintSubstraitRel;


// ------------------------------
// Constants

constexpr int SUCCESS = 0;
constexpr int ERROR_INVALID_ARGS = 1;
constexpr int ERROR_INVALID_PLAN = 2;
constexpr int ERROR_FILE_READ    = 3;
constexpr int ERROR_PLAN_PROCESS = 6;

// ------------------------------
// Classes

enum PlanType {
   SuperPlan
  ,SubPlan
  ,Merged
  ,Pushdown
  ,Pushback
};


// ------------------------------
// Functions

// >> Prototypes
unique_ptr<Plan>
ProcessPlanAsService( const string&           query_name
                     ,unique_ptr<PlanMessage> plan_msg
                     ,bool                    use_eagersplit
                     ,int                     count_splits);


// >> Implementations
int ValidateArgs(int argc, char **argv) {
  if (argc != 2) {
    std::cerr << "Usage: mohair <path-to-substrait-plan>" << std::endl;
    return ERROR_INVALID_ARGS;
  }

  // Check if the file exists
  if (!fs::exists(argv[1])) {
    std::cerr << "Unable to find plan file: " << argv[1] << std::endl;
    return ERROR_INVALID_PLAN;
  }

  return SUCCESS;
}


int WritePlan(Plan& plan, const string& plan_fname) {
  std::cout << "\tWriting plan to file [" << plan_fname << "]" << std::endl;

  auto output_stream = mohair::OutputStreamForFile(plan_fname.data());
  if (not output_stream) { return 14; }
  if (not plan.SerializeToOstream(&output_stream)) {
    std::cerr << "\tFailed to write plan to file" << std::endl;
    return 15;
  }

  return 0;
}

int WritePushdownPlan(Plan& pushdown_plan, string& plan_name) {
  string pushdown_fname { "resources/pushdown/" + plan_name + "pushdown.substrait" };

  return WritePlan(pushdown_plan, pushdown_fname);
}

int WriteSuperPlan(Plan& super_plan, string& plan_name) {
  string super_fname { "resources/superplans/" + plan_name + "super.substrait" };

  return WritePlan(super_plan, super_fname);
}

int WriteSubPlan(Plan& sub_plan, string& plan_name) {
  string sub_fname { "resources/subplans/" + plan_name + "sub.substrait" };

  return WritePlan(sub_plan, sub_fname);
}

int WritePushbackPlan(Plan& pushback_plan, string& plan_name) {
  string pushback_fname { "resources/pushback/" + plan_name + "pushback.substrait" };

  return WritePlan(pushback_plan, pushback_fname);
}

int WriteMergedPlan(Plan& merged_plan, string& plan_name) {
  string merged_fname { "resources/merged/" + plan_name + "merged.substrait" };

  return WritePlan(merged_plan, merged_fname);
}

int WriteExecutionPlan(Plan& exec_plan, string& plan_name) {
  string execution_fname { "resources/execution/" + plan_name + "execution.substrait" };

  return WritePlan(exec_plan, execution_fname);
}

// NOTE: hardcoded to assume the first PlanRel is a root
string GetPlanID(Plan& plan) {
  if (not plan.relations(0).has_root()) { return string { "" }; }

  return string { "-" + std::to_string(plan.relations(0).subtree_anchor()) };
}

const mohair::SubstraitSchema&
SchemaFromRel(const mohair::Rel& src_rel) {
  const mohair::RelCommon& src_common = mohair::GetRelCommon(src_rel);

  if (not src_common.hint().has_output_schema()) {
    throw std::runtime_error("Expected Rel to have output schema");
  }

  return src_common.hint().output_schema();
}

void
SetResultRel(Plan* pushback_plan) {
  if (not pushback_plan->mutable_relations(0)->has_root()) {
    std::cerr << "First PlanRel is not a root?" << std::endl;
    throw std::runtime_error("Expected first PlanRel to be a root");
  }

  mohair::Rel* root_rel = pushback_plan->mutable_relations(0)
                                       ->mutable_root()
                                       ->mutable_input();

  mohair::SkyResultRel result_op;
  *(result_op.mutable_result_name()) = string { "test-name" };
  result_op.mutable_schema()->CopyFrom(SchemaFromRel(*root_rel));

  mohair::Rel result_rel;
  result_rel.set_allocated_extension_leaf(new mohair::ExtensionLeafRel);
  result_rel.mutable_extension_leaf()->mutable_detail()->PackFrom(result_op);
  mohair::MoveRelOp(&result_rel, root_rel);
}

void
SetResultRel(mohair::Rel* merge_rel, const mohair::Rel& exec_rootrel) {
  mohair::SkyResultRel result_op;
  *(result_op.mutable_result_name()) = string { "test-name" };
  result_op.mutable_schema()->CopyFrom(SchemaFromRel(exec_rootrel));

  mohair::Rel result_rel;
  result_rel.set_allocated_extension_leaf(new mohair::ExtensionLeafRel);
  result_rel.mutable_extension_leaf()->mutable_detail()->PackFrom(result_op);
  mohair::MoveRelOp(&result_rel, merge_rel);
}

unique_ptr<SystemPlan>
ParseQueryPlan(unique_ptr<PlanMessage>&& received_plan) {
  if (received_plan == nullptr) {
    std::cerr << "Received null plan message" << std::endl;
    return nullptr;
  }

  unique_ptr<SystemPlan> sys_plan { mohair::SystemPlanFrom(std::move(received_plan)) };
  if (sys_plan == nullptr) {
    std::cerr << "Failed to construct system plan" << std::endl;
    return nullptr;
  }

  return sys_plan;
}

void DecomposePlanEager( unique_ptr<PlanSplit> eager_split
                        ,string&               plan_name
                        ,const string&         query_name
                        ,bool                  use_eagersplit
                        ,int                   count_splits) {
  vector<unique_ptr<PlanMessage>> subplan_msgs = eager_split->ExtractSubplans();

  // Write the superplan (superplan of the original pushdown)
  Plan&    super_plan        = *(eager_split->super_plan);
  int      mergerel_ndx      = super_plan.relations_size() - 1;
  uint32_t mergerel_anchorid = super_plan.relations(mergerel_ndx).subtree_anchor();

  string splan_name { plan_name + "anchor-" + std::to_string(mergerel_anchorid) + "." };
  WriteSuperPlan(super_plan,  splan_name);

  for (size_t sub_ndx = 0; sub_ndx < subplan_msgs.size(); ++sub_ndx) {
    Plan&    subplan      = *(subplan_msgs[sub_ndx]->payload);
    uint32_t sub_anchorid = subplan.relations(0).subtree_anchor();

    string subplan_name { plan_name + "anchor-" + std::to_string(sub_anchorid) + "." };
    WriteSubPlan(subplan, subplan_name);

    // Delegate subplan to downstream service
    unique_ptr<Plan> pushback_plan = ProcessPlanAsService(
       query_name, std::move(subplan_msgs[sub_ndx]), use_eagersplit, count_splits
    );

    if (pushback_plan == nullptr) {
      std::cerr << "Error: Received empty pushback" << std::endl;
      throw std::runtime_error("Eager split received empty pushback");
    }

    unique_ptr<PlanMessage> pushback_msg = PlanMessage::FromPlan(std::move(pushback_plan));

    // >> Merge the pushback and write the result
    eager_split->MergeSubplan(pushback_msg.get());
    WriteMergedPlan(*(eager_split->super_plan), splan_name);
  }
}

unique_ptr<Plan> DecomposePlanLazy(SystemPlan& sys_plan, string& plan_name) {
  unique_ptr<PlanSplit> lazy_split {
    PlanSplit::FindSplit(&sys_plan, DecomposeAlg::LongPipelineLeaf)
  };

  if (not lazy_split->CanSplit()) {
    Plan& parsed_plan   = *(sys_plan.plan_msg->payload);
    auto  pushback_plan = std::make_unique<Plan>();
    pushback_plan->CopyFrom(parsed_plan);

    WriteExecutionPlan(parsed_plan, plan_name);
    SetResultRel(pushback_plan.get());
    WritePushbackPlan(*pushback_plan, plan_name);
    return pushback_plan;
  }

  // NOTE: lazy  splitting means subplan  == candidate execution plan
  // NOTE: eager splitting means pushback == candidate execution plan
  // NOTE: ExtractSubplans modifies sys_plan by moving the execution portion to a subtree
  // TODO: probably true, but if the anchor operator is a join, a lazy split can choose
  //       a join input. That is technically 1 subplan if the other join input is
  //       "unavailable".
  // NOTE: define "data availability" as whether there is a local relation that can be
  //       matched. define "data consistency" as being sure that available data is
  //       consistent with the data source (all places it is persisted downstream).
  //       for our initial paper, we consider "being in a partition" means failure
  //       should happen ("strong consistency" therefore data is unavailable).
  unique_ptr<PlanMessage> exec_subplan = lazy_split->ExtractExecSubplan();
  WriteExecutionPlan(*(exec_subplan->payload), plan_name);

  // TODO: only necessary if lazy_split does not modify sys_plan (which it should)
  // pushback_plan->CopyFrom(*(lazy_split->super_plan));
  Plan* super_plan        = lazy_split->super_plan;
  int   mergerel_ndx      = super_plan->relations_size() - 1;
  mohair::Rel* merge_rel  = super_plan->mutable_relations(mergerel_ndx)->mutable_rel();
  mohair::Rel* anchor_rel = lazy_split->superplan_mergerel->substrait_rel;

  SetResultRel(merge_rel, exec_subplan->root_relation->root().input());
  mohair::MoveReferenceToOp(super_plan, anchor_rel);
  WritePushbackPlan(*(sys_plan.plan_msg->payload), plan_name);

  return std::move(sys_plan.plan_msg->payload);
}


unique_ptr<Plan>
ProcessPlanAsService( const string&           query_name
                     ,unique_ptr<PlanMessage> plan_msg
                     ,bool                    use_eagersplit
                     ,int                     count_splits) {
  // Parse phase
  unique_ptr<SystemPlan> sys_plan = ParseQueryPlan(std::move(plan_msg));
  if (sys_plan == nullptr) { return nullptr; }

  Plan&  substrait_plan = *(sys_plan->plan_msg->payload);
  string plan_name {
    query_name + GetPlanID(substrait_plan) + "." + std::to_string(count_splits) + "."
  };

  // Decomposition phase
  std::cout << "Decomposing plan [stage: " << count_splits << "]" << std::endl;
  if (WritePushdownPlan(substrait_plan, plan_name) != 0) { return nullptr; }

  // Base case for delegation
  if (count_splits == 0) {
    if (not use_eagersplit) { return DecomposePlanLazy(*sys_plan, plan_name); }

    auto pushback_plan = std::make_unique<Plan>();
    pushback_plan->CopyFrom(substrait_plan);

    // The whole plan is "executed" and the pushback is just the ResultRel
    WriteExecutionPlan(*pushback_plan, plan_name);
    SetResultRel(pushback_plan.get());
    WritePushbackPlan(*pushback_plan, plan_name);
    return pushback_plan;
  }

  // Check if we're doing a lazy split (simpler logic)
  if (not use_eagersplit) {
    // Delegate whole plan
    unique_ptr<Plan> pushback_plan = ProcessPlanAsService(
       query_name, std::move(sys_plan->plan_msg), use_eagersplit, count_splits - 1
    );

    if (pushback_plan == nullptr) {
      std::cerr << "Error: Received empty pushback" << std::endl;
      return nullptr;
    }

    // Parse pushback so that we can do lazy splitting
    unique_ptr<SystemPlan> lazy_sysplan = ParseQueryPlan(
      PlanMessage::FromPlan(std::move(pushback_plan))
    );

    if (lazy_sysplan == nullptr) { return nullptr; }

    // Do lazy splitting, "execution," then return the pushback
    return DecomposePlanLazy(*lazy_sysplan, plan_name);
  }

  // Otherwise, do eager split
  unique_ptr<PlanSplit> eager_split {
    PlanSplit::FindSplit(sys_plan.get(), DecomposeAlg::WideJoinHead)
  };

  // If there's no split we can do, function as a pass-through
  if (not eager_split->CanSplit()) {
    return ProcessPlanAsService(
      query_name, std::move(sys_plan->plan_msg), use_eagersplit, count_splits - 1
    );
  }

  DecomposePlanEager(
     std::move(eager_split)
    ,plan_name
    ,query_name
    ,use_eagersplit
    ,count_splits - 1
  );

  // >> Execution phase
  auto pushback_plan = std::make_unique<Plan>();
  pushback_plan->CopyFrom(substrait_plan);

  // The whole plan is "executed" and the pushback is just the ResultRel
  WriteExecutionPlan(*pushback_plan, plan_name);
  SetResultRel(pushback_plan.get());
  WritePushbackPlan(*pushback_plan, plan_name);
  return pushback_plan;
}


// ------------------------------
// Main Logic
int main(int argc, char **argv) {
  int validate_status = ValidateArgs(argc, argv);
  if (validate_status != 0) {
    std::cerr << "Failed to validate input command-line args" << std::endl;
    return validate_status;
  }

  // Process CLI arg a bit
  string substrait_fpath { argv[1] };
  string substrait_fname { fs::path(substrait_fpath).stem() };

  // Initialize the logger
  string logger_name { "test-decomposer" };
  MohairInitLogger(logger_name);

  // Read the example substrait from a file
  unique_ptr<PlanMessage> substrait_msg { PlanMessage::FromFile(substrait_fpath) };
  if (substrait_msg->payload == nullptr) {
    std::cerr << "Failed to read substrait plan from file" << std::endl;
    return ERROR_FILE_READ;
  }

  PlanMessage received_pushback { std::make_unique<Plan>() };

  // Make a copy to represent sending over the network
  auto request_copy = std::make_unique<Plan>();
  request_copy->CopyFrom(*(substrait_msg->payload));

  // Exercise query splitting using eager and lazy strategies
  // NOTE: this test code is naive and so the whole chain is either eager or lazy
  unique_ptr<Plan> eager_pushback = ProcessPlanAsService(
     substrait_fname + "-eager"
    ,std::make_unique<PlanMessage>(std::move(request_copy))
    ,true
    ,2
  );

  if (eager_pushback == nullptr) {
    std::cerr << "Failed to process plan with eager splits" << std::endl;
    return ERROR_PLAN_PROCESS;
  }

  // Initialize a new request to test lazy splitting
  request_copy = std::make_unique<Plan>();
  request_copy->CopyFrom(*(substrait_msg->payload));
  unique_ptr<Plan> lazy_pushback = ProcessPlanAsService(
     substrait_fname + "-lazy"
    ,std::make_unique<PlanMessage>(std::move(request_copy))
    ,false
    ,2
  );

  if (lazy_pushback == nullptr) {
    std::cerr << "Failed to process plan with lazy splits" << std::endl;
    return ERROR_PLAN_PROCESS;
  }

  return 0;
}
