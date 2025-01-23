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

// >> Standard libs
#include <unistd.h>
#include <filesystem>

// >> Internal
#include "mohair.hpp"
#include "mohair/plans.hpp"


// ------------------------------
// Type aliases

// >> Namespaces
namespace fs = std::filesystem;

// standard types
using std::unique_ptr;
using std::string;

// query processing types
using mohair::SubstraitPlan;
using mohair::SystemPlan;


// ------------------------------
// Structs and Classes

struct ToolInterface {
  fs::path plan_fpath;
  bool     show_plan      { true  };
  bool     show_pipelines { false };

  int Start() {
    if (plan_fpath.empty()) {
      std::cerr << "File containing serialized plan message is required" << std::endl;
      return 1;
    }

    auto plan = SubstraitPlan::FromFile(plan_fpath.string());

    if (show_plan) { plan->Print(); }
    else {
      unique_ptr<SystemPlan> sys_plan = mohair::SystemPlanFrom(std::move(plan));
      sys_plan->PrintPipelines();
    }

    return 0;
  }
};


// ------------------------------
// Functions

int PrintHelp() {
    std::cout << "read-substrait"
              << " [-h] [-b] [-s]"
              << " -f <path-to-substrait-file>"
              << " -p [ plan | pipelines ]"
              << std::endl
    ;

    return 1;
}


// ------------------------------
// Main Logic

int main(int argc, char **argv) {
  ToolInterface my_cli;

  // Parse each argument and internalize the provided option
  constexpr char  is_done_parsing = -1;
  const     char* opt_template    = "f:hbsp:";

  char parsed_opt;
  while ((parsed_opt = (char) getopt(argc, argv, opt_template)) != is_done_parsing) {
    switch (parsed_opt) {

      case 'h': { return PrintHelp(); }

      case 'f': {
        my_cli.plan_fpath = fs::absolute(optarg).string();
        break;
      }

      case 'p': {
        string print_mode { optarg };

        if (print_mode == "plan") {
          my_cli.show_plan      = true;
          my_cli.show_pipelines = false;
        }

        else if (print_mode == "pipelines") {
          my_cli.show_plan      = false;
          my_cli.show_pipelines = true;
        }

        else {
          std::cerr << "Invalid print mode: [" << print_mode << "]" << std::endl;
          return 1;
        }

        break;
      }

      default: { break; }
    }
  }

  return my_cli.Start();
}
