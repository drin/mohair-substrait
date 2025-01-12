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
using mohair::SubstraitMessage;
using mohair::SystemPlan;
using mohair::PipelineStage;
using mohair::PlanSplit;
using mohair::DecomposeAlg;


// ------------------------------
// Functions
int ValidateArgs(int argc, char **argv) {
  if (argc != 2) {
    std::cerr << "Usage: mohair <path-to-substrait-plan>" << std::endl;
    return 1;
  }

  // Check if the file exists
  if (!fs::exists(argv[1])) {
    std::cerr << "Unable to find plan file: " << argv[1] << std::endl;
    return 2;
  }

  return 0;
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
  auto substrait_msg = SubstraitMessage::FromFile(substrait_fpath);
  if (substrait_msg->payload == nullptr) {
    std::cerr << "Failed to read substrait plan from file" << std::endl;
    return 2;
  }

  // Convert substrait to a plan we understand
  // NOTE: keep this alive, everything else references from it.
  std::cout << "Parsing Substrait..." << std::endl;
  unique_ptr<SystemPlan> sys_plan { mohair::SystemPlanFrom(std::move(substrait_msg)) };

  if (sys_plan == nullptr) {
    std::cerr << "Failed to construct system plan" << std::endl;
    return 10;
  }

  sys_plan->PrintPipelines();

  // >> split super-plan into sub-plans
  unique_ptr<PlanSplit> stage_split {
    PlanSplit::FindSplit(sys_plan.get(), DecomposeAlg::WideJoinHead)
  };

  // do the actual split
  vector<unique_ptr<PlanMessage>> subplan_msgs = stage_split->ExtractSubplans();

  // write the superplan
  string superplan_fname {
    "resources/superplans/" + substrait_fname + "-super."
                            + std::to_string(stage_split->stage_ndx)
                            + ".substrait"
  };

  std::cout << "\tWriting to file [" << superplan_fname << "]" << std::endl;

  SubstraitMessage& superplan_msg = dynamic_cast<SubstraitMessage&>(*(sys_plan->plan_msg));
  auto success = superplan_msg.SerializeToFile(superplan_fname.data());

  if (not success) {
    std::cerr << "\tFailed to write superplan" << std::endl;
    return 12;
  }


  for (size_t msg_ndx = 0; msg_ndx < subplan_msgs.size(); ++msg_ndx) {
    PlanMessage& subplan_msg = *(subplan_msgs[msg_ndx]);

    // Write just the sub plan
    string subplan_fname {
      "resources/subplans/" + substrait_fname + "-sub."
                            + std::to_string(stage_split->stage_ndx)
                            + "." + std::to_string(msg_ndx)
                            + ".substrait"
    };

    std::cout << "\tWriting to file [" << subplan_fname << "]" << std::endl;

    SubstraitMessage& msg = dynamic_cast<SubstraitMessage&>(subplan_msg);
    auto success = msg.SerializeToFile(subplan_fname.data());

    if (not success) {
      std::cerr << "\tFailed to write subplan" << std::endl;
      return 13;
    }

    // Write the merged plan
    string merge_fname {
      "resources/merged/" + substrait_fname + "-merged."
                          + std::to_string(stage_split->stage_ndx)
                          + "." + std::to_string(msg_ndx)
                          + ".substrait"
    };

    stage_split->MergeSubplan(&subplan_msg);

    std::cout << "\tWriting to file [" << merge_fname << "]" << std::endl;

    auto output_stream = mohair::OutputStreamForFile(merge_fname.data());
    if (output_stream) {
      if (not stage_split->super_plan->SerializeToOstream(&output_stream)) {
        std::cerr << "\tFailed to write merged plan to file" << std::endl;
        return 14;
      }
    }
  }

  return 0;
}
