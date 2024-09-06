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


// >> API dependencies
#include "apidep_standard.hpp"         // C++ standard library
#include "apidep_substrait.hpp"        // Substrait protocol types


// >> Internal deps
#include "mohair-substrait-config.hpp" // Library configuration

#include "mohair-substrait/mohair/algebra.pb.h"  // Mohair protocol types
#include "mohair-substrait/mohair/topology.pb.h"


// ------------------------------
// Aliases

namespace mohair {

  // mohair-protocol types (for query processing)
  using mohair::PlanAnchor;
  using mohair::ErrRel;

  // mohair-protocol types (for topology representation)
  using mohair::ServiceConfig;
  using mohair::DeviceClass;


  // global variables (within the library)
  const string version       = MOHAIR_VERSION_STRING;
  const string version_major = MOHAIR_VERSION_MAJOR;
  const string version_minor = MOHAIR_VERSION_MINOR;
  const string version_patch = MOHAIR_VERSION_PATCH;
};


// ------------------------------
// Functions

namespace mohair {

  // TODO: hide `Message` to be internal linkage only
  // >> Wrapper functions for protobuf framework
  bool StringifyPlan (const Message& msg    , string* text_result);
  bool StringifyRel  (const Message& msg    , string* text_result);
  bool SerializeJson (const string& msg_json, Message* msg_result);
  bool JsonifyMessage(const Message& msg    , string* json_result);

  // >> Reader functions
  // helper functions
  fstream InputStreamForFile(const char* in_fpath);
  fstream OutputStreamForFile(const char* out_fpath);
  bool    FileToString(const char* in_fpath, string& file_data);

  // deserialization functions
  unique_ptr<Plan> SubstraitPlanFromString(string& plan_msg);
  unique_ptr<Plan> SubstraitPlanFromFile(const char* plan_fpath);
  unique_ptr<Plan> SubstraitPlanFromFile(string& plan_fpath);


  // >> Debug functions
  void PrintSubstraitPlan(Plan *plan_msg);
  void PrintSubstraitRel(Rel   *rel_msg);


  // >> Helper functions
  int FindPlanRoot(Plan& substrait_plan);

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
  };


  //! Derived class of `PlanMessage` that uses substrait
  struct SubstraitMessage : PlanMessage {

    // destructors and constructors
    virtual ~SubstraitMessage() = default;

    SubstraitMessage(unique_ptr<Plan>&& msg): PlanMessage(std::move(msg)) {}
    SubstraitMessage(unique_ptr<Plan>&& msg, int root_relndx)
      : PlanMessage(std::move(msg), root_relndx) {}

    // methods
    virtual string Serialize();
    virtual bool   SerializeToFile(const char *out_fpath);

    // static methods
    static unique_ptr<PlanMessage> FromString(string& plan_str);
    static unique_ptr<PlanMessage> FromFile(const char* plan_fpath);
    static unique_ptr<PlanMessage> FromFile(string plan_fpath);
  };

} // namespace: mohair
