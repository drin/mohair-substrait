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

#include "mohair/adapters/adapter_standard.hpp"
#include "mohair/apidep_substrait.hpp"


// ------------------------------
// Functions

namespace mohair {

  // >> Public functions

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
    // AnchorIDGenerator is static
    uint32_t anchor_id = ++AnchorIDGenerator;

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
  //  NOTE: returns 0 on failure (AnchorIDGenerator starts at 1)
  uint32_t MoveReferenceToOp(Plan* plan, Rel* ref_rel) {
    int32_t  anchor_relndx = ref_rel->reference().subtree_ordinal();
    uint32_t anchor_id     = ref_rel->reference().subtree_reference();
    PlanRel* anchor_rel    = plan->mutable_relations(anchor_relndx);

    // Do some validation
    if (not anchor_rel->has_rel())                 { return 0; }
    if (not ref_rel->has_reference())              { return 0; }
    if (anchor_id != anchor_rel->subtree_anchor()) { return 0; }

    // Replace the ReferenceRel
    MoveRelOp(anchor_rel->mutable_rel(), ref_rel);

    auto plan_rels = plan->mutable_relations();
    plan_rels->erase(plan_rels->begin() + anchor_relndx);
    return anchor_id;
  }

  //! Create a ReferenceRel pointing to `PlanRel` and hang it on parent_rel
  uint32_t CreateReferenceRel(Rel* parent_rel, PlanRel* anchor_rel) {
    uint32_t anchor_id { ++AnchorIDGenerator };

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

} // namespace: mohair
