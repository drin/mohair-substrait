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
#include "mohair_substrait.hpp"


// ------------------------------
// Type aliases

// >> Namespaces
namespace fs = std::filesystem;

using mohair::SubstraitMessage;


// ------------------------------
// Structs and Classes

struct ToolInterface {
  fs::path plan_fpath;

  int Start() {
    if (plan_fpath.empty()) {
      std::cerr << "File containing serialized plan message is required" << std::endl;
      return 1;
    }

    auto query_plan = SubstraitMessage::FromFile(plan_fpath.string());
    mohair::PrintSubstraitPlan(query_plan->payload.get());
  }
};


// ------------------------------
// Functions

int PrintHelp() {
    std::cout << "read-substrait"
              << " [-h] [-b] [-s]"
              << " -f <path-to-substrait-file>"
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
  const     char* opt_template    = "f:hbs";

  char parsed_opt;
  while ((parsed_opt = (char) getopt(argc, argv, opt_template)) != is_done_parsing) {
    switch (parsed_opt) {

      case 'h': { return PrintHelp(); }

      case 'f': {
        my_cli.plan_fpath = fs::absolute(optarg).string();
        break;
      }

      default: { break; }
    }
  }

  return my_cli.Start();
}
