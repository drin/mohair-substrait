// ------------------------------
// License
//
// Copyright 2025 Aldrin Montana
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

// API dependencies
#include "mohair/apidep_substrait.hpp"


// ------------------------------
// Aliases

// >> type aliases
namespace mohair {

  // >> Protobuf types
  using google::protobuf::RepeatedPtrField;
  using google::protobuf::Message;
  using google::protobuf::util::MessageDifferencer;

  // >> Substrait types
  // Statistics
  using skyproto::substrait::DecomposeStats;

  // Type representation
  using SubstraitType   = skyproto::substrait::Type;
  using SubstraitSchema = skyproto::substrait::NamedStruct;

  // Expression representation
  using SubstraitExpr   = skyproto::substrait::Expression;

  // Plan level
  using skyproto::substrait::Plan;
  using skyproto::substrait::PlanRel;

  // Relation level
  using skyproto::substrait::Rel;
  using skyproto::substrait::RelCommon;
  using skyproto::substrait::RelRoot;
  using skyproto::substrait::extensions::AdvancedExtension;

  // Leaf types
  using skyproto::substrait::ReadRel;
  using skyproto::substrait::ExtensionLeafRel;
  using skyproto::substrait::ReferenceRel;

  // Unary types (streaming)
  using skyproto::substrait::ProjectRel;
  using skyproto::substrait::FilterRel;
  using skyproto::substrait::FetchRel;

  // Unary types (sink)
  using skyproto::substrait::SortRel;
  using skyproto::substrait::AggregateRel;

  // Binary types (sink)
  using skyproto::substrait::JoinRel;
  using skyproto::substrait::CrossRel;
  using skyproto::substrait::HashJoinRel;
  using skyproto::substrait::MergeJoinRel;

  // N-ary types (streaming)
  using skyproto::substrait::SetRel;

} // namespace: mohair

