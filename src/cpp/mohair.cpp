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

#include "mohair.hpp"


// ------------------------------
// Functions

// >> Wrapper functions for protobuf framework functions
namespace mohair {

  // Types to wrap
  using google::protobuf::TextFormat;

  // Functions to wrap
  using google::protobuf::util::JsonStringToMessage;
  using google::protobuf::util::MessageToJsonString;

  // Wrapper implementation for `TextFormat::PrintToString`
  bool StringifyMessage(const Message& msg, std::string* text_result) {
    return TextFormat::PrintToString(msg, text_result);
  }

  // TODO: decide if I should return a status object that has an error message
  // Wrapper implementation for `JsonStringToMessage`
  bool SerializeJson(const std::string& msg_json, Message* msg_result) {
    absl::Status status = JsonStringToMessage(msg_json, msg_result);
    return status.ok();
  }

  bool JsonifyMessage(const Message& msg, std::string* json_result) {
    absl::Status status = MessageToJsonString(msg, json_result);
    return status.ok();
  }


} // namespace: mohair

namespace mohair {

  //  >> Reader functions
  //! Returns a binary input stream for the given file path
  std::fstream InputStreamForFile(const char* in_fpath) {
    return std::fstream { in_fpath, std::ios::in | std::ios::binary };
  }

  //! Returns a binary output stream for the given file path
  std::fstream OutputStreamForFile(const char* out_fpath) {
    return std::fstream { out_fpath, std::ios::out | std::ios::trunc | std::ios::binary };
  }

  //! Reads data from the given file path into an output string as binary
  bool FileToString(const char* in_fpath, std::string& file_data) {
    // create an IO stream for the file
    auto file_stream = InputStreamForFile(in_fpath);
    if (!file_stream) {
      std::cerr << "Failed to open IO stream for file" << std::endl;
      return false;
    }

    // go to end of stream, read the position, then reset position
    file_stream.seekg(0, std::ios_base::end);
    auto size = file_stream.tellg();
    file_stream.seekg(0);

    // Resize the output and read the file data into it
    file_data.resize(size);
    auto output_ptr = &(file_data[0]);
    file_stream.read(output_ptr, size);

    // On success, the number of characters read will match size
    return file_stream.gcount() == size;
  }


  // >> Conversion functions (into/out of substrait plans)
  std::unique_ptr<Plan> SubstraitPlanFromString(std::string &plan_msg) {
    auto substrait_plan = std::make_unique<Plan>();
    substrait_plan->ParseFromString(plan_msg);

    #if MOHAIR_DEBUG
      substrait_plan->PrintDebugString();
    #endif

    return substrait_plan;
  }

  std::unique_ptr<Plan> SubstraitPlanFromFile(const char* plan_fpath) {
    std::fstream plan_fstream = InputStreamForFile(plan_fpath);

    auto substrait_plan = std::make_unique<Plan>();
    if (substrait_plan->ParseFromIstream(&plan_fstream)) { return substrait_plan; }

    std::cerr << "Failed to parse substrait plan" << std::endl;
    return nullptr;
  }

  std::unique_ptr<Plan> SubstraitPlanFromFile(std::string& plan_fpath) {
    return SubstraitPlanFromFile(plan_fpath.data());
  }


  // >> Debug functions
  void PrintProtoMessage(const Message& msg) {
    std::string msg_text;

    bool status_stringify { StringifyMessage(msg, &msg_text) };
    if (not status_stringify) {
      std::cerr << "Unable to print message" << std::endl;
      return;
    }

    std::cout << msg_text << std::endl;
  }

  void  PrintSubstraitRel(Rel  *rel_msg ) { PrintProtoMessage(*rel_msg);  }
  void PrintSubstraitPlan(Plan *plan_msg) { PrintProtoMessage(*plan_msg); }


  // >> Helper functions
  int FindPlanRoot(Plan& substrait_plan) {
    int root_count = 0;
    int root_ndx   = -1;

    for (int ndx = 0; ndx < substrait_plan.relations_size(); ++ndx) {
      PlanRel plan_root { substrait_plan.relations(ndx) };

      // Don't break after we find a RelRoot; validate there is only 1
      // We expect there to be a reasonably small number of RelRoot
      if (plan_root.rel_type_case() == PlanRel::RelTypeCase::kRoot) {
        ++root_count;
        root_ndx = ndx;
      }
    }

    if (root_count != 1) {
      std::cerr << "Found [" << std::to_string(root_count) << "] RootRels" << std::endl;
      return -1;
    }

    return root_ndx;
  }


} // namespace: mohair


// ------------------------------
// Method implementations

namespace mohair {

  // >> Methods for SubstraitMessage
  std::string SubstraitMessage::Serialize() {
    std::string msg_serialized;

    if (not this->payload->SerializeToString(&msg_serialized)) {
      std::cerr << "Error when serializing substrait message." << std::endl;
    }

    return msg_serialized;
  }

  bool SubstraitMessage::SerializeToFile(const char *out_fpath) {
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

  std::unique_ptr<PlanMessage> SubstraitMessage::FromString(std::string& plan_str) {
    auto query_plan = SubstraitPlanFromString(plan_str);
    return std::make_unique<SubstraitMessage>(std::move(query_plan));
  }

  std::unique_ptr<PlanMessage> SubstraitMessage::FromFile(const char* plan_fpath) {
    auto query_plan = SubstraitPlanFromFile(plan_fpath);
    return std::make_unique<SubstraitMessage>(std::move(query_plan));
  }

  std::unique_ptr<PlanMessage> SubstraitMessage::FromFile(std::string plan_fpath) {
    return SubstraitMessage::FromFile(plan_fpath.data());
  }

} // namespace: mohair

