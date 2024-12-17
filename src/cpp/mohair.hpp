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
#include "mohair/apidep_standard.hpp"  // C++ standard library
#include "mohair/apidep_substrait.hpp" // Substrait protocol types


// >> Internal deps
#include "mohair-config.hpp" // Library configuration

#include "skyproto/mohair/algebra.pb.h"  // Mohair protocol types
#include "skyproto/mohair/topology.pb.h"


// ------------------------------
// Aliases

namespace mohair {

  // mohair-protocol types (for query processing)
  using skyproto::mohair::SuperPlan;
  using skyproto::mohair::SubPlan;
  using skyproto::mohair::ErrRel;

  // mohair-protocol types (for topology representation)
  using skyproto::mohair::ServiceConfig;
  using skyproto::mohair::DeviceClass;


  // global variables (within the library)
  const std::string version       = MOHAIR_VERSION_STRING;
  const std::string version_major = MOHAIR_VERSION_MAJOR;
  const std::string version_minor = MOHAIR_VERSION_MINOR;
  const std::string version_patch = MOHAIR_VERSION_PATCH;

} // namespace: mohair


// ------------------------------
// Functions

namespace mohair {

  // TODO: hide `Message` to be internal linkage only
  // >> Wrapper functions for protobuf framework
  bool StringifyPlan (const Message&     msg     , std::string* text_result);
  bool StringifyRel  (const Message&     msg     , std::string* text_result);
  bool SerializeJson (const std::string& msg_json, Message*     msg_result);
  bool JsonifyMessage(const Message&     msg     , std::string* json_result);

  // >> Reader functions
  // helper functions
  std::fstream InputStreamForFile(const char* in_fpath);
  std::fstream OutputStreamForFile(const char* out_fpath);
  bool         FileToString(const char* in_fpath, std::string& file_data);

  // deserialization functions
  std::unique_ptr<Plan> SubstraitPlanFromString(std::string& plan_msg);
  std::unique_ptr<Plan> SubstraitPlanFromFile(const char* plan_fpath);
  std::unique_ptr<Plan> SubstraitPlanFromFile(std::string& plan_fpath);


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
    std::unique_ptr<Plan> payload;
    int              root_relndx { -1 }; // initialized to -1 as a sentinel
    PlanRel*         root_relation;

    // destructors and constructors
    virtual ~PlanMessage() = default;

    PlanMessage(std::unique_ptr<Plan>&& msg): payload(std::move(msg)) {}
    PlanMessage(std::unique_ptr<Plan>&& msg, int root_relndx)
      : payload(std::move(msg)), root_relndx(root_relndx) {
      this->root_relation = this->payload->mutable_relations(root_relndx);
    }
  };


  //! Derived class of `PlanMessage` that uses substrait
  struct SubstraitMessage : PlanMessage {

    // destructors and constructors
    virtual ~SubstraitMessage() = default;

    SubstraitMessage(std::unique_ptr<Plan>&& msg): PlanMessage(std::move(msg)) {}
    SubstraitMessage(std::unique_ptr<Plan>&& msg, int root_relndx)
      : PlanMessage(std::move(msg), root_relndx) {}

    // methods
    virtual std::string Serialize();
    virtual bool        SerializeToFile(const char *out_fpath);

    // static methods
    static std::unique_ptr<PlanMessage> FromString(std::string& plan_str);
    static std::unique_ptr<PlanMessage> FromFile(const char*    plan_fpath);
    static std::unique_ptr<PlanMessage> FromFile(std::string    plan_fpath);
  };

} // namespace: mohair
