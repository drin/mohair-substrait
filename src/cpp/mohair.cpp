// ------------------------------
// License
//
// Copyright 2024-2025 Aldrin Montana
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
// Overview
//
// Headers, aliases, and macros for the protobuf framework


// ------------------------------
// Dependencies
#pragma once

#include "mohair.hpp"


// ------------------------------
// Functions

namespace mohair {

  //! Wrapper that uses `TextFormat::PrintToString` to write to stdout
  void PrintMessage(const Message& msg) {
    string msg_text;

    if (TextFormat::PrintToString(msg, &msg_text)) {
      std::cout << msg_text << std::endl;
    }

    else {
      MohairLogMsg("Failed to print message");
    }
  }

  //! Wrapper that uses `TextFormat::PrintToString` to write to a string
  bool StringifyMessage(const Message& msg, string* text_result) {
    return TextFormat::PrintToString(msg, text_result);
  }

  std::optional<string> StringifyMessage(const Message& msg) {
    string msg_text;
    if (StringifyMessage(msg, &msg_text)) { return msg_text; }

    return std::nullopt;
  }

} // namespace: mohair


// ------------------------------
// Class implementations

namespace mohair {

  // >> SubstraitPlan Methods
  void SubstraitPlan::Print() { PrintMessage(*(this->plan)); }
  bool SubstraitPlan::ToString(string* out_str) {
    string plan_str;
    
    if (TextFormat::PrintToString(*this->plan, &plan_str)) {
      *out_str = plan_str;
      return true;
    }
    
    MohairLogMsg("Failed to stringify substrait plan");
    return false;
  }

  bool SubstraitPlan::SerializeToString(string& out_data) {
    return this->SerializeToString(&out_data);
  }

  bool SubstraitPlan::SerializeToString(string* out_data) {
    if (this->payload->SerializeToString(&out_data)) { return true; }

    MohairLogMsg("Error when serializing substrait message.");
    return false;
  }

  bool SubstraitPlan::SerializeToFile(const string& out_fpath) {
    return SerializeToFile(out_fpath.data());
  }

  bool SubstraitPlan::SerializeToFile(const char *out_fpath) {
    auto file_stream = OutputStreamForFile(out_fpath);
    if (!file_stream) {
      std::cerr << "Failed to open IO stream for serialization" << std::endl;
      return false;
    }

    if (not this->payload->SerializeToOstream(&file_stream)) {
      std::cerr << "Unable to substrait message to file" << std::endl;
      return false;
    }

    return true;
  }

  // >> SubstraitPlan Static methods
  // Builder functions (from memory)
  unique_ptr<SubstraitPlan> SubstraitPlan::FromPlan(unique_ptr<Plan>&& plan) {
    auto [root_rel, rel_ndx] = FindPlanRoot(*plan);
    if (root_rel == nullptr) {
      MohairLogMsg("Failed to find root relation in substrait plan");
      return nullptr;
    }

    return std::make_unique<SubstraitPlan>(std::move(plan), rel_ndx);
  }

  unique_ptr<SubstraitPlan> SubstraitPlan::FromString(const string& plan_str) {
    unique_ptr<Plan> substrait_plan { std::make_unique<Plan>() };
    substrait_plan->ParseFromString(plan_str);

    return SubstraitPlan::FromPlan(std::move(substrait_plan));
  }

  // Builder functions (from files)
  unique_ptr<SubstraitPlan> SubstraitPlan::FromFile(const char* plan_fpath) {
    auto    substrait_plan { std::make_unique<Plan>()       };
    fstream plan_fstream   { InputStreamForFile(plan_fpath) };

    if (not substrait_plan->ParseFromIstream(&plan_fstream)) {
      MohairLogMsg("Failed to parse substrait plan");
      return nullptr;
    }

    auto [root_rel, rel_ndx] = FindPlanRoot(*substrait_plan);
    if (root_rel == nullptr) {
      MohairLogMsg("Failed to find root relation in substrait plan");
      return nullptr;
    }

    return std::make_unique<SubstraitPlan>(std::move(substrait_plan), rel_ndx);
  }

  unique_ptr<SubstraitPlan> SubstraitPlan::FromFile(const string& plan_fpath) {
    return SubstraitPlan::FromFile(plan_fpath.data());
  }


} // namespace: mohair

