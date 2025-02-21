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
#pragma once


// >> Internal headers

// Library configuration
#include "mohair-config.hpp"

// API dependencies
#include "mohair/apidep_standard.hpp"  // C++ standard library
#include "mohair/apidep_substrait.hpp" // Substrait protocol types

// Substrait extensions
#include "skyproto/mohair/algebra.pb.h"
#include "skyproto/mohair/topology.pb.h"


// ------------------------------
// Macros

#define MOHAIR_ASSERT(assert_msg, assert_expr) { \
    assert(assert_expr && assert_msg);           \
  }


#if MOHAIR_DEBUG
  #define MohairStartTS(phase_name) \
    SteadyTS ts_start_##phase_name = steady_clock::now();

  #define MohairStopTS(phase_name) \
    SteadyTS ts_stop_##phase_name = steady_clock::now();

  #define MohairLogTimestamps(phase_name) {                                      \
    auto ts_diff = StringifyTSDiff(ts_start_##phase_name, ts_stop_##phase_name); \
    *(MohairLogger()) << "["                                                     \
                              << StringifyTS(ts_start_##phase_name) << ":µs"     \
                      << ", " << StringifyTS(ts_stop_##phase_name)  << ":µs"     \
                      << ", " << ts_diff                            << ":µs"     \
                   << "] |> " << #phase_name << std::endl                        \
    ;                                                                            \
  }

  #define MohairLogPerf(phase_name, code_block) \
    MohairStartTS(phase_name)                   \
    code_block                                  \
    MohairStopTS(phase_name)                    \
    MohairLogTimestamps(phase_name)

#else
  #define MohairLogTimestamps(ts_name, log_msg) {}
  #define MohairLogPerf(phase_name, code_block) code_block
  #define MohairStartTS(phase_name)             {}
  #define MohairStopTS(phase_name)              {}

#endif


// ------------------------------
// Aliases

namespace mohair {

  // >> Standard types
  using SteadyTS = steady_clock::time_point;

  // >> Mohair types
  // Plan level
  using skyproto::mohair::SuperPlan;
  using skyproto::mohair::SubPlan;

  // Operator level
  using skyproto::mohair::QueryRel;
  using skyproto::mohair::ErrRel;

  using skyproto::mohair::SkyRel;
  using skyproto::mohair::SkyPartitionRel;
  using skyproto::mohair::SkySliceRel;

  using skyproto::mohair::SkyResultRel;
  using skyproto::mohair::SkyLakeRel;

  // Topology level
  using skyproto::mohair::ServiceConfig;
  using skyproto::mohair::DeviceClass;

  // Decomposition options
  using skyproto::mohair::DecomposeAlg;

} // namespace: mohair


// ------------------------------
// Global variables

namespace mohair {

  // >> Macro-backed Global variables (library internal)
  const string version         = MOHAIR_VERSION_STRING;
  const string version_major   = MOHAIR_VERSION_MAJOR;
  const string version_minor   = MOHAIR_VERSION_MINOR;
  const string version_patch   = MOHAIR_VERSION_PATCH;

  // >> Static variables
  static uint32_t UUIDGenerator { 0 };

} // namespace: mohair


// ------------------------------
// Functions

namespace mohair {

  //! Stringify a steady clock timestamp as microseconds
  string StringifyTS(const SteadyTS& ts);

  //! Stringify the difference between two timestamps as microseconds
  string StringifyTSDiff(const SteadyTS& ts_start, const SteadyTS& ts_stop);

  //! A function that returns a singleton file handle for a log file.
  std::fstream* MohairLogger();

  // TODO: hide `Message` to be internal linkage only
  // >> Wrapper functions for protobuf framework
  bool StringifyPlan (const Plan&    plan_msg, string*  text_result);
  bool StringifyRel  (const Rel&     rel_msg , string*  text_result);
  bool SerializeJson (const string&  msg_json, Message* msg_result );
  bool JsonifyMessage(const Message& msg     , string*  json_result);

  // >> Reader functions
  // helper functions
  std::fstream InputStreamForFile(const char* in_fpath);
  std::fstream OutputStreamForFile(const char* out_fpath);
  bool         FileToString(const char* in_fpath, string& file_data);

  // deserialization functions
  unique_ptr<Plan> SubstraitPlanFromString(const string& plan_msg);
  unique_ptr<Plan> SubstraitPlanFromFile(const char* plan_fpath);
  unique_ptr<Plan> SubstraitPlanFromFile(string& plan_fpath);


  // >> Debug functions
  void PrintSubstraitPlan(const Plan& plan_msg);
  void PrintSubstraitRel(const Rel&   rel_msg);

  void PrintSubstraitPlan(const Plan *plan_msg);
  void PrintSubstraitRel(const Rel   *rel_msg);


  // >> Helper functions

  //! Find the index of the plan's root operator in the list of op trees
  int FindPlanRoot(Plan& substrait_plan);

  //! Get the common op structure from the given Rel
  RelCommon*       GetRelCommon(Rel*       rel);
  const RelCommon& GetRelCommon(const Rel& rel);

  //! Move the operator from `src_rel` to `dst_rel`
  void MoveRelOp(Rel* src_rel, Rel* dst_rel);

  //! Move an operator into a PlanRel and create a ReferenceRel to it
  PlanRel* MoveOpToReference(Plan* plan, Rel* op);

  //! Move a PlanRel into an operator tree by swapping it with its ReferenceRel
  uint32_t MoveReferenceToOp(Plan* plan, Rel* ref_rel);

  //! Move a PlanRel into an operator tree by swapping it with its ReferenceRel
  uint32_t CreateReferenceRel(Rel* parent_rel, PlanRel* anchor_rel);

  //! Create a SuperPlan reference to the given PlanRel
  unique_ptr<SuperPlan> CreateSuperPlanRel(PlanRel* anchor_rel);

  //! Create a SkyResultRel that describes how to read a remote materialized result
  unique_ptr<SkyResultRel> CreateResultRel(PlanRel* view_plan);

  //! Copy the Rel but then clear its input (e.g. input to ProjectRel)
  unique_ptr<Rel> CopyRel(Rel* src_rel);

  //! Create a MessageDifferencer for rel op (e.g. `ProjectRel`) that is non-recursive
  unique_ptr<MessageDifferencer> DifferencerForRel(Rel* src_rel);

  //! Get vector of each input `Rel` to the given `Rel`
  vector<Rel*> GetInputRels(Rel* output_rel);

} // namespace: mohair


// ------------------------------
// Classes and structs

namespace mohair {

  //! A base class representing a query plan sent as a message
  struct PlanMessage {
    // attributes
    unique_ptr<Plan> payload;
    int              root_relndx { -1 }; // initialized to -1 as a sentinel
    PlanRel*         root_relation;

    // destructors and constructors
    virtual ~PlanMessage() = default;

    PlanMessage(unique_ptr<Plan>&& msg): payload(std::move(msg)) {}
    PlanMessage(unique_ptr<Plan>&& msg, int root_relndx)
      : payload(std::move(msg)), root_relndx(root_relndx) {
      this->root_relation = this->payload->mutable_relations(root_relndx);
    }

    // methods
    virtual string Serialize();
    virtual bool   SerializeToFile(const char *out_fpath);

    // static methods
    static unique_ptr<PlanMessage> FromPlan(unique_ptr<Plan>&& plan);
    static unique_ptr<PlanMessage> FromString(const string&    plan_str);
    static unique_ptr<PlanMessage> FromFile(const char*        plan_fpath);
    static unique_ptr<PlanMessage> FromFile(string             plan_fpath);
  };

} // namespace: mohair
