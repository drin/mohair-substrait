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
string FilenameForPlan(const string& basename, PlanType plan_type, size_t srv_ndx, size_t msg_ndx = 0);

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


int WritePlan(const string& plan_fname, Plan* plan) {
  std::cout << "\tWriting plan to file [" << plan_fname << "]" << std::endl;

  auto output_stream = mohair::OutputStreamForFile(plan_fname.data());
  if (not output_stream) { return 14; }
  if (not plan->SerializeToOstream(&output_stream)) {
    std::cerr << "\tFailed to write plan to file" << std::endl;
    return 15;
  }

  return 0;
}

string FilenameForPlan(const string& basename, PlanType plan_type, size_t srv_ndx, size_t msg_ndx) {
  string suffix { ".substrait" };

  switch (plan_type) {
    case PlanType::SuperPlan: {
      string dirname   { "resources/superplans/" };
      string plan_name { basename + "-super." + std::to_string(srv_ndx) };

      return dirname + plan_name + suffix;
    }

    case PlanType::SubPlan: {
      string dirname   { "resources/subplans/" };
      string plan_name {
          basename + "-sub."
        + std::to_string(srv_ndx) + "."
        + std::to_string(msg_ndx)
      };
      return dirname + plan_name + suffix;
    }

    case PlanType::Merged: {
      string dirname   { "resources/merged/" };
      string plan_name {
          basename + "-merged."
        + std::to_string(srv_ndx) + "."
        + std::to_string(msg_ndx)
      };

      return dirname + plan_name + suffix;
    }

    case PlanType::Pushdown: {
      string dirname   { "resources/pushdown/" };
      string plan_name { basename + "-pushdown." + std::to_string(srv_ndx) };

      return dirname + plan_name + suffix;
    }

    case PlanType::Pushback: {
      string dirname   { "resources/pushback/" };
      string plan_name {
          basename + "-pushback."
        + std::to_string(srv_ndx) + "."
        + std::to_string(msg_ndx)
      };

      return dirname + plan_name + suffix;
    }

    default:
      std::cerr << "Invalid plan type" << std::endl;
      return "";
  }
}


unique_ptr<Plan>
ProcessPlanAsService(const string& query_name, unique_ptr<PlanMessage> plan_msg, int count_splits) {
  if (plan_msg == nullptr) {
    std::cerr << "Received null plan message" << std::endl;
    return nullptr;
  }

  // Convert substrait to a plan we understand
  unique_ptr<SystemPlan> sys_plan { mohair::SystemPlanFrom(std::move(plan_msg)) };

  if (sys_plan == nullptr) {
    std::cerr << "Failed to construct system plan" << std::endl;
    return nullptr;
  }

  Plan* service_plan = sys_plan->plan_msg->payload.get();

  // Currently, plan is what we received as a pushdown
  string root_anchorid { "" };
  if (service_plan->mutable_relations(0)->has_root()) {
    root_anchorid = string {
        "-"
      + std::to_string(
          service_plan->mutable_relations(0)->subtree_anchor()
        )
    };
  }

  string pushdown_fname = FilenameForPlan(
    query_name + root_anchorid, PlanType::Pushdown, count_splits
  );
  if (WritePlan(pushdown_fname, service_plan) != 0) { return nullptr; }

  std::cout << "[" << count_splits << "] Pipelines" << std::endl;
  sys_plan->PrintPipelines();

  // >> If we're a leaf, just return our pushback message
  if (count_splits == 0) {
    auto pushback_msg = std::make_unique<Plan>();
    pushback_msg->CopyFrom(*service_plan);

    return pushback_msg;
  }


  // >> Otherwise, we'll split and propagate and then return
  // Find a split point
  unique_ptr<PlanSplit> service_split {
    PlanSplit::FindSplit(sys_plan.get(), DecomposeAlg::WideJoinHead)
  };

  // If there's no split we can do, function as a pass-through
  if (not service_split->CanSplit()) {
    return ProcessPlanAsService(
      query_name, std::move(sys_plan->plan_msg), count_splits - 1
    );
  }

  // Make the split
  vector<unique_ptr<PlanMessage>> subplan_msgs = service_split->ExtractSubplans();

  // Write the superplan (superplan of the original pushdown)
  int mergerel_ndx = service_split->super_plan->relations_size() - 1;
  uint32_t mergerel_anchorid = service_split->super_plan->mutable_relations(mergerel_ndx)->subtree_anchor();
  string superplan_fname = FilenameForPlan(
     query_name + "-" + std::to_string(mergerel_anchorid)
    ,PlanType::SuperPlan
    ,count_splits
  );
  WritePlan(superplan_fname, service_split->super_plan);

  for (size_t subplan_ndx = 0; subplan_ndx < subplan_msgs.size(); ++subplan_ndx) {
    PlanMessage* subplan_msg  = subplan_msgs[subplan_ndx].get();
    uint32_t subplan_anchorid = (
      subplan_msg->payload->mutable_relations(0)
                          ->subtree_anchor()
    );

    // Now, the service_plan is the result of merging with subplan [subplan_ndx]
    string subplan_fname = FilenameForPlan(
       query_name + "-" + std::to_string(subplan_anchorid)
      ,PlanType::SubPlan
      ,count_splits
      ,subplan_ndx
    );
    WritePlan(subplan_fname, subplan_msg->payload.get());

    if (subplan_msg->payload->mutable_relations(0)->mutable_root()->mutable_input()->has_aggregate()) {
      auto aggregate_rel = subplan_msg->payload->mutable_relations(0)->mutable_root()->mutable_input()->mutable_aggregate();
      if (aggregate_rel->mutable_input()->has_reference()) {
        std::cout << "Check processing of this subplan" << std::endl;
        // PrintSubstraitPlan(subplan_msg->payload.get());

        mohair::AdvancedExtension* subplan_planext  = subplan_msg->payload->mutable_advanced_extensions();
        auto optimizations = subplan_planext->mutable_optimization();
        for (auto itr = optimizations->begin(); itr != optimizations->end(); ++itr) {
          mohair::SuperPlan tmp_superplan {};
          if (not itr->Is<mohair::SuperPlan>()) { continue; }

          itr->UnpackTo(&tmp_superplan);
          // std::cout << "SuperPlan: " << tmp_superplan.DebugString() << std::endl;
        }
      }
    }

    auto subplan_copy = std::make_unique<Plan>();
    subplan_copy->CopyFrom(*subplan_msg->payload);

    // Recurse to downstream service
    unique_ptr<Plan> received_pushback = ProcessPlanAsService(
       query_name
      ,std::make_unique<PlanMessage>(std::move(subplan_copy))
      ,count_splits - 1
    );

    if (received_pushback == nullptr) {
      std::cout << "Received empty pushback" << std::endl;
      /*
      std::cout << "-- Super Plan --" << std::endl;
      PrintSubstraitPlan(service_plan);

      std::cout << "-- Subplan --" << std::endl;
      PrintSubstraitPlan(subplan_msg->payload.get());
      */

      return nullptr;
    }

    unique_ptr<PlanMessage> pushback_msg = PlanMessage::FromPlan(
      std::move(received_pushback)
    );

    // >> Merge the pushback and write the result
    service_split->MergeSubplan(pushback_msg.get());

    // Now, the service_plan is the result of merging with subplan [subplan_ndx]
    string mergeplan_fname = FilenameForPlan(
       query_name + root_anchorid
      ,PlanType::Merged
      ,service_split->stage_ndx
      ,subplan_ndx
    );
    WritePlan(mergeplan_fname, service_plan);
  }

  // Now, we respond with our updated PlanMessage
  auto pushback_plan = std::make_unique<Plan>();
  pushback_plan->CopyFrom(*service_plan);

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

  unique_ptr<Plan> pushback_plan = ProcessPlanAsService(
     substrait_fname
    ,std::make_unique<PlanMessage>(std::move(request_copy))
    ,2
  );

  if (pushback_plan == nullptr) {
    std::cerr << "Failed to process plan" << std::endl;
    return ERROR_PLAN_PROCESS;
  }

  return 0;
}
