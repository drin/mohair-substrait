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
// An adapter layer for the Substrait specification, which attempts to
// standardize how to represent query plans.


// ------------------------------
// Dependencies
#pragma once

// >> Configuration
#include "mohair-config.hpp"

// >> Protobuf framework deps
#include "mohair/apidep_standard.hpp"
#include "mohair/apidep_protobuf.hpp"

// >> Generated protobuf deps for substrait
#include "skyproto/substrait/plan.pb.h"
#include "skyproto/substrait/algebra.pb.h"
#include "skyproto/substrait/extensions/extensions.pb.h"


// ------------------------------
// Aliases

namespace mohair {

  // >> Standard types
  using FunctionAnchorMap = unordered_map<uint64_t, string>;

  // >> Substrait types
  // Data types and Metadata
  using SubstraitSchema = skyproto::substrait::NamedStruct;
  using SubstraitType   = skyproto::substrait::Type;

  using skyproto::substrait::extensions::AdvancedExtension;

  // Plan level
  using skyproto::substrait::Plan;
  using skyproto::substrait::PlanRel;
  using skyproto::substrait::RelRoot;

  // Relation level
  using skyproto::substrait::Rel;
  using skyproto::substrait::RelCommon;

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

