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

// >> Standard headers

#include "mohair/apidep_substrait.hpp"
#include "mohair/adapters/adapter_standard.hpp"

#include "mohair/operator_traits.hpp"
#include "mohair/op_specializations.hpp" // Specialized helpers for `RelOp`
#include "mohair/plans.hpp"



// ------------------------------
// Functions

namespace mohair {

  // Functions for doing things directly on substrait types without `RelOp`
  bool IsSinkRel(Rel::RelTypeCase   rel_type);
  bool IsOriginRel(Rel::RelTypeCase rel_type);

  string GetSourceName(ReadRel*          rel_op);
  string GetSourceName(ExtensionLeafRel* rel_op);
  string GetSourceName(SkyRel*           extrel_op);
  string GetSourceName(SkyPartitionRel*  extrel_op);
  string GetSourceName(SkySliceRel*      extrel_op);

} // namespace: mohair


// ------------------------------
// Operator classes

namespace mohair {

  // >> Structural helpers

  //    |> Move operators around the substrait plan

  //! Moves an operator in the plan from `src_rel` to `dst_rel`
  void MoveRelOp(Rel* src_rel, Rel* dst_rel);

  //! Promotes an operator to a PlanRel and creates a ReferenceRel that references it
  PlanRel* MoveOpToReference(Plan* plan, Rel* op);

  //! Demotes a PlanRel to an operator by swapping it with its ReferenceRel
  //  NOTE: this should only be done if it is only referenced by a single ReferenceRel
  uint32_t MoveReferenceToOp(Plan* plan, Rel* ref_rel);


  //    |> Create new operators

  //! Copy the Rel but then clear its input (e.g. input to ProjectRel)
  unique_ptr<Rel> CopyRel(Rel* src_rel);

  //! Creates a ReferenceRel that points to the given PlanRel.
  uint32_t CreateReferenceRel(Rel* parent_rel, PlanRel* anchor_rel);


  //    |> Comparison mechanisms

  //! Create a MessageDifferencer for operators that does not recurse on inputs
  unique_ptr<MessageDifferencer> DifferencerForRel(Rel* src_rel);

} // namespace: mohair

