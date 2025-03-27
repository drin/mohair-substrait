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
// Dependencies
#pragma once


// >> Internal headers

// Library configuration
#include "mohair-config.hpp"

// API dependencies
#include "mohair/apidep_standard.hpp"   // C++ standard library
#include "mohair/adapter_substrait.hpp" // Substrait protocol types

// Substrait extensions
#include "skyproto/mohair/algebra.pb.h"
#include "skyproto/mohair/topology.pb.h"


// ------------------------------
// Macros

#define MOHAIR_ASSERT(assert_msg, assert_expr) { \
    assert(assert_expr && assert_msg);           \
  }


#define MohairInitLogger(logger_name) {   \
  MOHAIR_ASSERT(                          \
     "Failed to initialize logger"        \
    ,MohairLogger(logger_name) != nullptr \
  );                                      \
}

#define MohairStartTS(phase_name) \
  SteadyTS ts_start_##phase_name = steady_clock::now();

#define MohairStopTS(phase_name) \
  SteadyTS ts_stop_##phase_name = steady_clock::now();

#define MohairLogTimestamps(phase_name) {                                      \
  auto ts_diff = StringifyTSDiff(ts_start_##phase_name, ts_stop_##phase_name); \
  *(mohair::MohairLogger()) << #phase_name                                     \
                            << " " << StringifyTS(ts_start_##phase_name)       \
                            << " " << StringifyTS(ts_stop_##phase_name)        \
                            << " " << ts_diff                                  \
                            << std::endl                                       \
  ;                                                                            \
}

#define MohairLogPerf(phase_name, code_block) \
  MohairStartTS(phase_name)                   \
  code_block                                  \
  MohairStopTS(phase_name)                    \
  MohairLogTimestamps(phase_name)


// ------------------------------
// Aliases

namespace mohair {

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
  std::fstream* MohairLogger(string logger_name);
  std::fstream* MohairLogger();

  // >> Reader functions
  // helper functions
  std::fstream InputStreamForFile(const char* in_fpath);
  std::fstream OutputStreamForFile(const char* out_fpath);
  bool         FileToString(const char* in_fpath, string& file_data);

  // >> Helper functions

  //! Find the index of the plan's root operator in the list of op trees
  int FindPlanRoot(Plan& substrait_plan);

  //! Return the PlanRel from substrait_plan containing the root subtree
  PlanRel* GetPlanRoot(Plan* substrait_plan);

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
  unique_ptr<SkyResultRel> CreateResultRelForPlan(Plan* view_plan);

} // namespace: mohair


// ------------------------------
// Classes and structs

namespace mohair {

  // >> Forward declarations
  struct SubstraitOp;
  struct SplitAnchor;

  //! A base class representing a query plan represented with substrait
  //  TODO: need some UUID for a plan, preferably based on its pipeline IDs
  struct SubstraitPlan {
    unique_ptr<Plan>                plan;
    unique_ptr<SubstraitOp>         plan_root;
    int                             root_relndx;

    vector<Rel*>                    super_mergerels;
    unordered_map<uint64_t, string> fn_anchors;

    // >> Destructors and constructors
    SubstraitPlan(unique_ptr<Plan>&& plan_msg, int rel_ndx)
      : plan(std::move(plan_msg)), plan_root(nullptr), root_relndx(rel_ndx) {}

    // >> API for sending the SubstraitPlan to storage or the network
    bool   SerializeToFile(const char*   out_fpath);
    bool   SerializeToFile(const string& out_fpath);
    bool   SerializeToString(string* out_str);
    bool   SerializeToString(string& out_str);
    string Serialize();

    // >> API for inspecting the contained substrait plan
    string StringifyPlan();
    void   PrintPlan();

    string NameForFnAnchor(uint64_t anchor_id);

    // >> API for modifying the contained substrait plan
    //! Create mapping of function anchors in the query plan
    void RegisterExtensionFunctions();

    //! Copies result aliases from RelRoot then clears them
    void ParseOperators();

    //! Copies result aliases from RelRoot then clears them
    void PushdownResultAliases();

    //! Move given operator into a subtree PlanRel and replace it with a ReferenceRel
    SplitAnchor AnchorForSplit(SubstraitOp* anchor_op);

    // >> API for construction of SubstraitPlan (Static methods)
    static unique_ptr<SubstraitPlan> FromPlan   (unique_ptr<Plan>&& plan);
    static unique_ptr<SubstraitPlan> FromMessage(const string&      plan_msg);
    static unique_ptr<SubstraitPlan> FromFile   (const char*        plan_fpath);
    static unique_ptr<SubstraitPlan> FromFile   (const string&      plan_fpath);
  };

} // namespace: mohair
