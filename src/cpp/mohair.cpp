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
// Aliases

namespace mohair {

  // Types to wrap
  using google::protobuf::TextFormat;

  // Functions to wrap
  using google::protobuf::util::JsonStringToMessage;
  using google::protobuf::util::MessageToJsonString;

} // namespace: mohair


// ------------------------------
// Functions

// >> Wrapper functions for protobuf framework functions
namespace mohair {

  string StringifyTS(const SteadyTS& ts) {
    auto ts_ms = std::chrono::duration_cast<std::chrono::microseconds>(
      ts.time_since_epoch()
    );

    return std::to_string(ts_ms.count());
  }

  string
  StringifyTSDiff(const SteadyTS& ts_start, const SteadyTS& ts_stop) {
    auto start_ms = std::chrono::duration_cast<std::chrono::microseconds>(
      ts_start.time_since_epoch()
    );

    auto stop_ms = std::chrono::duration_cast<std::chrono::microseconds>(
      ts_stop.time_since_epoch()
    );

    return std::to_string(stop_ms.count() - start_ms.count());
  }

  string PathForInstantiatedLog(const string& logger_name) {
    const string path_prefix { "mohair." + logger_name + "." };
    const string path_suffix { ".log"                        };

    auto     ts_logstart = system_clock::to_time_t(system_clock::now());
    std::tm* local_ts    = std::localtime(&ts_logstart);

    stringstream ss;
    ss << path_prefix << std::put_time(local_ts, "%Y%m%d%H%M%S") << path_suffix;

    return ss.str();
  }

  std::fstream* MohairLogger() {
    static string empty_name;
    return MohairLogger(empty_name);
  }

  std::fstream* MohairLogger(string logger_name) {
    static bool         is_initialized { false };
    static std::fstream log_handle;

    if (not is_initialized) {
      string log_fpath = PathForInstantiatedLog(logger_name);
      log_handle       = OutputStreamForFile(log_fpath.data());

      auto ts_init = steady_clock::now();
      log_handle << "[" << StringifyTS(ts_init) << ":µs] "
                 << "|> initial timestamp"      << std::endl
      ;

      is_initialized = true;
    }

    return &log_handle;
  }

  // Wrapper implementation for `TextFormat::PrintToString`
  bool StringifyMessage(const Message& msg, string* text_result) {
    return TextFormat::PrintToString(msg, text_result);
  }

  bool StringifyPlan(const Plan& plan_msg, string* text_result) {
    return StringifyMessage(plan_msg, text_result);
  }

  bool StringifyRel(const Rel& rel_msg, string* text_result) {
    return StringifyMessage(rel_msg, text_result);
  }

  // TODO: decide if I should return a status object that has an error message
  // Wrapper implementation for `JsonStringToMessage`
  bool SerializeJson(const string& msg_json, Message* msg_result) {
    absl::Status status = JsonStringToMessage(msg_json, msg_result);
    return status.ok();
  }

  bool JsonifyMessage(const Message& msg, string* json_result) {
    absl::Status status = MessageToJsonString(msg, json_result);
    return status.ok();
  }


  //! Helper function to traverse a plan and gather its final schema
  void ResolveResultSchema(Rel* view_op) {
    switch (view_op->rel_type_case()) {
      // unary operators
      case Rel::RelTypeCase::kProject: { break; }
      case Rel::RelTypeCase::kFilter: { break; }
      case Rel::RelTypeCase::kFetch: { break; }
      case Rel::RelTypeCase::kSort: { break; }
      case Rel::RelTypeCase::kAggregate: { break; }

      // binary operators
      case Rel::RelTypeCase::kJoin: { break; }
      case Rel::RelTypeCase::kCross: { break; }
      case Rel::RelTypeCase::kHashJoin: { break; }
      case Rel::RelTypeCase::kMergeJoin: { break; }

      // Leaf operators
      case Rel::RelTypeCase::kReference: { break; }

      case Rel::RelTypeCase::kRead: { break; }
      case Rel::RelTypeCase::kExtensionLeaf: { break; }

      // Unimplemented operators
      default: {
        throw std::runtime_error("Cannot inline anchor rel: unimplemented type");
      }
    }
  }

  //! Move a RelOp (e.g. ProjectRel) from the src Rel to the dest Rel
  void MoveRelOp(Rel* src_rel, Rel* dst_rel) {
    switch (src_rel->rel_type_case()) {
      // unary operators
      case Rel::RelTypeCase::kProject: {
        dst_rel->set_allocated_project(src_rel->release_project());
        break;
      }

      case Rel::RelTypeCase::kFilter: {
        dst_rel->set_allocated_filter(src_rel->release_filter());
        break;
      }

      case Rel::RelTypeCase::kFetch: {
        dst_rel->set_allocated_fetch(src_rel->release_fetch());
        break;
      }

      case Rel::RelTypeCase::kSort: {
        dst_rel->set_allocated_sort(src_rel->release_sort());
        break;
      }

      case Rel::RelTypeCase::kAggregate: {
        dst_rel->set_allocated_aggregate(src_rel->release_aggregate());
        break;
      }

      // binary operators
      case Rel::RelTypeCase::kJoin: {
        dst_rel->set_allocated_join(src_rel->release_join());
        break;
      }

      case Rel::RelTypeCase::kCross: {
        dst_rel->set_allocated_cross(src_rel->release_cross());
        break;
      }

      case Rel::RelTypeCase::kHashJoin: {
        dst_rel->set_allocated_hash_join(src_rel->release_hash_join());
        break;
      }

      case Rel::RelTypeCase::kMergeJoin: {
        dst_rel->set_allocated_merge_join(src_rel->release_merge_join());
        break;
      }

      // Leaf operators
      case Rel::RelTypeCase::kReference: {
        dst_rel->set_allocated_reference(src_rel->release_reference());
        break;
      }

      case Rel::RelTypeCase::kRead: {
        dst_rel->set_allocated_read(src_rel->release_read());
        break;
      }

      case Rel::RelTypeCase::kExtensionLeaf: {
        dst_rel->set_allocated_extension_leaf(src_rel->release_extension_leaf());
        break;
      }

      // Unimplemented operators
      default: {
        throw std::runtime_error("Cannot inline anchor rel: unimplemented type");
      }
    }
  }

  //! Move an operator into a PlanRel and create a ReferenceRel to it
  PlanRel* MoveOpToReference(Plan* plan, Rel* op) {
    // UUIDGenerator is static
    uint32_t anchor_id = ++UUIDGenerator;

    // Create a place to move the operator to
    unique_ptr<Rel> anchor_rel { std::make_unique<Rel>() };

    int32_t  next_relndx = plan->relations_size();
    PlanRel* new_anchor  = plan->add_relations();
    new_anchor->set_allocated_rel(anchor_rel.release());
    new_anchor->set_subtree_anchor(anchor_id);

    // Create a reference to replace the operator with
    unique_ptr<ReferenceRel> ref_rel { std::make_unique<ReferenceRel>() };
    ref_rel->set_subtree_ordinal(next_relndx);
    ref_rel->set_subtree_reference(anchor_id);

    // Move the operator into the anchor
    MoveRelOp(op, new_anchor->mutable_rel());

    // Insert the ReferenceRel
    op->set_allocated_reference(ref_rel.release());

    return new_anchor;
  }

  //! Move a PlanRel into an operator tree by swapping it with its ReferenceRel
  //  NOTE: returns 0 on failure (UUIDGenerator starts at 1)
  uint32_t MoveReferenceToOp(Plan* plan, Rel* ref_rel) {
    if (not ref_rel->has_reference()) { return 0; }

    int32_t  anchor_relndx = ref_rel->reference().subtree_ordinal();
    uint32_t anchor_id     = ref_rel->reference().subtree_reference();
    PlanRel* anchor_rel    = plan->mutable_relations(anchor_relndx);

    // Do some validation
    if (not anchor_rel->has_rel())                 { return 0; }
    if (anchor_id != anchor_rel->subtree_anchor()) { return 0; }

    // Replace the ReferenceRel
    MoveRelOp(anchor_rel->mutable_rel(), ref_rel);

    auto plan_rels = plan->mutable_relations();
    plan_rels->erase(plan_rels->begin() + anchor_relndx);
    return anchor_id;
  }

  //! Create a ReferenceRel pointing to `PlanRel` and hang it on parent_rel
  uint32_t CreateReferenceRel(Rel* parent_rel, PlanRel* anchor_rel) {
    uint32_t anchor_id { ++UUIDGenerator };

    // Set the anchor ID
    anchor_rel->set_subtree_anchor(anchor_id);

    // Create a ReferenceRel and point it to the anchor
    unique_ptr<ReferenceRel> ref_rel { std::make_unique<ReferenceRel>() };
    ref_rel->set_subtree_reference(anchor_id);

    // Insert the ReferenceRel into Rel
    parent_rel->set_allocated_reference(ref_rel.release());

    // Return the anchor ID in case it's useful
    return anchor_id;
  }

  //! Create a SuperPlan reference to the given PlanRel
  unique_ptr<SuperPlan> CreateSuperPlanRel(PlanRel* anchor_rel) {
    Rel* merge_rel;
    if (anchor_rel->has_rel()) { merge_rel = anchor_rel->mutable_rel();                   }
    else                       { merge_rel = anchor_rel->mutable_root()->mutable_input(); }

    unique_ptr<Rel>       rel_copy      { CopyRel(merge_rel) };
    unique_ptr<SuperPlan> superplan_msg { std::make_unique<SuperPlan>() };

    superplan_msg->set_allocated_merge_rel(rel_copy.release());
    superplan_msg->set_mergerel_reference(anchor_rel->subtree_anchor());

    return superplan_msg;
  }

  //! Copy the Rel but then clear its input (e.g. input to ProjectRel)
  unique_ptr<Rel> CopyRel(Rel* src_rel) {
    unique_ptr<Rel> rel_copy { std::make_unique<Rel>(*src_rel) };

    switch(src_rel->rel_type_case()) {
      // unary operators
      case Rel::RelTypeCase::kProject: {
        rel_copy->mutable_project()->clear_input();
        break;
      }

      case Rel::RelTypeCase::kFilter: {
        rel_copy->mutable_filter()->clear_input();
        break;
      }

      case Rel::RelTypeCase::kFetch: {
        rel_copy->mutable_fetch()->clear_input();
        break;
      }

      case Rel::RelTypeCase::kSort: {
        rel_copy->mutable_sort()->clear_input();
        break;
      }

      case Rel::RelTypeCase::kAggregate: {
        rel_copy->mutable_aggregate()->clear_input();
        break;
      }

      // binary operators
      case Rel::RelTypeCase::kJoin: {
        rel_copy->mutable_join()->clear_left();
        rel_copy->mutable_join()->clear_right();
        break;
      }

      case Rel::RelTypeCase::kCross: {
        rel_copy->mutable_cross()->clear_left();
        rel_copy->mutable_cross()->clear_right();
        break;
      }

      case Rel::RelTypeCase::kHashJoin: {
        rel_copy->mutable_hash_join()->clear_left();
        rel_copy->mutable_hash_join()->clear_right();
        break;
      }

      case Rel::RelTypeCase::kMergeJoin: {
        rel_copy->mutable_merge_join()->clear_left();
        rel_copy->mutable_merge_join()->clear_right();
        break;
      }

      // Leaf operators (no-op)
      case Rel::RelTypeCase::kReference:
      case Rel::RelTypeCase::kRead:
      case Rel::RelTypeCase::kExtensionLeaf: {
        break;
      }

      // Unimplemented operators
      default: {
        std::cerr << "Simplification unimplemented for operator." << std::endl;
        return nullptr;
      }
    }

    return rel_copy;
  }


  //! Create a MessageDifferencer for rel op (e.g. `ProjectRel`) that is non-recursive
  unique_ptr<MessageDifferencer> DifferencerForRel(Rel* src_rel) {
    auto differ = std::make_unique<MessageDifferencer>();

    switch (src_rel->rel_type_case()) {
      case Rel::RelTypeCase::kProject: {
        differ->IgnoreField(ProjectRel::descriptor()->FindFieldByName("input"));
        break;
      }

      case Rel::RelTypeCase::kFilter: {
        differ->IgnoreField(FilterRel::descriptor()->FindFieldByName("input"));
        break;
      }

      case Rel::RelTypeCase::kFetch: {
        differ->IgnoreField(FetchRel::descriptor()->FindFieldByName("input"));
        break;
      }

      case Rel::RelTypeCase::kSort: {
        differ->IgnoreField(SortRel::descriptor()->FindFieldByName("input"));
        break;
      }

      case Rel::RelTypeCase::kAggregate: {
        differ->IgnoreField(AggregateRel::descriptor()->FindFieldByName("input"));
        break;
      }

      // binary operators
      case Rel::RelTypeCase::kJoin: {
        differ->IgnoreField(JoinRel::descriptor()->FindFieldByName("left"));
        differ->IgnoreField(JoinRel::descriptor()->FindFieldByName("right"));
        break;
      }

      case Rel::RelTypeCase::kCross: {
        differ->IgnoreField(CrossRel::descriptor()->FindFieldByName("left"));
        differ->IgnoreField(CrossRel::descriptor()->FindFieldByName("right"));
        break;
      }

      case Rel::RelTypeCase::kHashJoin: {
        differ->IgnoreField(HashJoinRel::descriptor()->FindFieldByName("left"));
        differ->IgnoreField(HashJoinRel::descriptor()->FindFieldByName("right"));
        break;
      }

      case Rel::RelTypeCase::kMergeJoin: {
        differ->IgnoreField(MergeJoinRel::descriptor()->FindFieldByName("left"));
        differ->IgnoreField(MergeJoinRel::descriptor()->FindFieldByName("right"));
        break;
      }

      // Leaf operators (no-op)
      case Rel::RelTypeCase::kReference:
      case Rel::RelTypeCase::kRead:
      case Rel::RelTypeCase::kExtensionLeaf: {
        break;
      }

      // Unimplemented operators
      default: {
        std::cerr << "Simplification unimplemented for operator." << std::endl;
        return nullptr;
      }
    }

    return differ;
  }


  vector<Rel*> GetInputRels(Rel* output_rel) {
    switch (output_rel->rel_type_case()) {
      // Unary operators
      case Rel::RelTypeCase::kProject:
        return { output_rel->mutable_project()->mutable_input()   };

      case Rel::RelTypeCase::kFilter:
        return { output_rel->mutable_filter()->mutable_input()    };

      case Rel::RelTypeCase::kFetch:
        return { output_rel->mutable_fetch()->mutable_input()     };

      case Rel::RelTypeCase::kSort:
        return { output_rel->mutable_sort()->mutable_input()      };

      case Rel::RelTypeCase::kAggregate:
        return { output_rel->mutable_aggregate()->mutable_input() };

      // binary operators
      case Rel::RelTypeCase::kJoin:
        return {
           output_rel->mutable_join()->mutable_left()
          ,output_rel->mutable_join()->mutable_right()
        };

      case Rel::RelTypeCase::kCross:
        return {
           output_rel->mutable_cross()->mutable_left()
          ,output_rel->mutable_cross()->mutable_right()
        };

      case Rel::RelTypeCase::kHashJoin:
        return {
           output_rel->mutable_hash_join()->mutable_left()
          ,output_rel->mutable_hash_join()->mutable_right()
        };

      case Rel::RelTypeCase::kMergeJoin:
        return {
           output_rel->mutable_merge_join()->mutable_left()
          ,output_rel->mutable_merge_join()->mutable_right()
        };

      // Leaf operators (no-op)
      case Rel::RelTypeCase::kReference:
      case Rel::RelTypeCase::kRead:
      case Rel::RelTypeCase::kExtensionLeaf:
        return vector<Rel*>(0);

      // Unimplemented operators
      default: {
        throw std::runtime_error("Cannot get inputs for unimplemented RelType");
      }
    }
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
  bool FileToString(const char* in_fpath, string& file_data) {
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
  unique_ptr<Plan> SubstraitPlanFromString(const string &plan_msg) {
    unique_ptr<Plan> substrait_plan { std::make_unique<Plan>() };
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

  std::unique_ptr<Plan> SubstraitPlanFromFile(string& plan_fpath) {
    return SubstraitPlanFromFile(plan_fpath.data());
  }


  // >> Debug functions
  void PrintProtoMessage(const Message& msg) {
    string msg_text;

    bool status_stringify { StringifyMessage(msg, &msg_text) };
    if (not status_stringify) {
      std::cerr << "Unable to print message" << std::endl;
      return;
    }

    std::cout << msg_text << std::endl;
  }

  void  PrintSubstraitRel(const Rel&  rel_msg ) { PrintProtoMessage(rel_msg);  }
  void PrintSubstraitPlan(const Plan& plan_msg) { PrintProtoMessage(plan_msg); }

  void  PrintSubstraitRel(const Rel*  rel_msg ) { PrintProtoMessage(*rel_msg);  }
  void PrintSubstraitPlan(const Plan* plan_msg) { PrintProtoMessage(*plan_msg); }


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

  RelCommon* GetRelCommon(Rel* rel) {
    switch (rel->rel_type_case()) {
      // unary operators
      case Rel::RelTypeCase::kProject:       return rel->mutable_project()->mutable_common();
      case Rel::RelTypeCase::kFilter:        return rel->mutable_filter()->mutable_common();
      case Rel::RelTypeCase::kFetch:         return rel->mutable_fetch()->mutable_common();
      case Rel::RelTypeCase::kSort:          return rel->mutable_sort()->mutable_common();
      case Rel::RelTypeCase::kAggregate:     return rel->mutable_aggregate()->mutable_common();

      // binary operators
      case Rel::RelTypeCase::kJoin:          return rel->mutable_join()->mutable_common();
      case Rel::RelTypeCase::kCross:         return rel->mutable_cross()->mutable_common();
      case Rel::RelTypeCase::kHashJoin:      return rel->mutable_hash_join()->mutable_common();
      case Rel::RelTypeCase::kMergeJoin:     return rel->mutable_merge_join()->mutable_common();

      // Leaf operators
      case Rel::RelTypeCase::kRead:          return rel->mutable_read()->mutable_common();
      case Rel::RelTypeCase::kExtensionLeaf: return rel->mutable_extension_leaf()->mutable_common();

      // Unimplemented operators
      case Rel::RelTypeCase::kReference:
        throw std::runtime_error("ReferenceRel does not have a common field");

      default: throw std::runtime_error("GetRelCommon not yet implemented for type");
    }
  }

  const RelCommon& GetRelCommon(const Rel& rel) {
    switch (rel.rel_type_case()) {
      // unary operators
      case Rel::RelTypeCase::kProject:       return rel.project().common();
      case Rel::RelTypeCase::kFilter:        return rel.filter().common();
      case Rel::RelTypeCase::kFetch:         return rel.fetch().common();
      case Rel::RelTypeCase::kSort:          return rel.sort().common();
      case Rel::RelTypeCase::kAggregate:     return rel.aggregate().common();

      // binary operators
      case Rel::RelTypeCase::kJoin:          return rel.join().common();
      case Rel::RelTypeCase::kCross:         return rel.cross().common();
      case Rel::RelTypeCase::kHashJoin:      return rel.hash_join().common();
      case Rel::RelTypeCase::kMergeJoin:     return rel.merge_join().common();

      // Leaf operators
      case Rel::RelTypeCase::kRead:          return rel.read().common();
      case Rel::RelTypeCase::kExtensionLeaf: return rel.extension_leaf().common();

      // Unimplemented operators
      case Rel::RelTypeCase::kReference:
        throw std::runtime_error("ReferenceRel does not have a common field");

      default: throw std::runtime_error("GetRelCommon not yet implemented for type");
    }
  }


} // namespace: mohair


// ------------------------------
// Method implementations

namespace mohair {

  // >> Methods for PlanMessage
  string PlanMessage::Serialize() {
    string msg_serialized;

    if (not this->payload->SerializeToString(&msg_serialized)) {
      std::cerr << "Error when serializing substrait message." << std::endl;
    }

    return msg_serialized;
  }

  bool PlanMessage::SerializeToFile(const char *out_fpath) {
    auto file_stream = OutputStreamForFile(out_fpath);
    if (!file_stream) {
      std::cerr << "Failed to open IO stream for serialization:" << std::endl
                << "\t" << out_fpath                             << std::endl
      ;
      return false;
    }

    if (not this->payload->SerializeToOstream(&file_stream)) {
      std::cerr << "Unable to substrait message to file" << std::endl;
      return false;
    }

    return true;
  }

  unique_ptr<PlanMessage> PlanMessage::FromPlan(unique_ptr<Plan>&& plan) {
    int root_relndx { FindPlanRoot(*plan) };

    return std::make_unique<PlanMessage>(std::move(plan), root_relndx);
  }

  unique_ptr<PlanMessage> PlanMessage::FromString(const string& plan_str) {
    unique_ptr<Plan> substrait_plan { SubstraitPlanFromString(plan_str) };
    int              root_relndx    { FindPlanRoot(*substrait_plan)     };

    return std::make_unique<PlanMessage>(std::move(substrait_plan), root_relndx);
  }

  unique_ptr<PlanMessage> PlanMessage::FromFile(const char* plan_fpath) {
    unique_ptr<Plan> substrait_plan { SubstraitPlanFromFile(plan_fpath) };
    int              root_relndx    { FindPlanRoot(*substrait_plan)     };

    return std::make_unique<PlanMessage>(std::move(substrait_plan), root_relndx);
  }

  unique_ptr<PlanMessage> PlanMessage::FromFile(string plan_fpath) {
    return PlanMessage::FromFile(plan_fpath.data());
  }

} // namespace: mohair

