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

#include <stdexcept>

#include "mohair.hpp"
#include "mohair/operators.hpp"


// ------------------------------
// Functions

namespace mohair {

  //! Use the rel type to determine if it is a sink rel
  bool IsSinkRel(Rel* rel) {
    switch (rel->rel_type_case()) {
      case Rel::RelTypeCase::kSort:
      case Rel::RelTypeCase::kAggregate:
      case Rel::RelTypeCase::kJoin:
      case Rel::RelTypeCase::kCross:
      case Rel::RelTypeCase::kMergeJoin:
      case Rel::RelTypeCase::kHashJoin:
        return true;

      default:
        return false;
    }
  }

  //! Use the rel type to determine if it is an origin rel
  bool IsOriginRel(Rel* rel) {
    switch (rel->rel_type_case()) {
      case Rel::RelTypeCase::kRead:
      case Rel::RelTypeCase::kExtensionLeaf:
        return true;

      default:
        return false;
    }
  }

  //! Extracts the source name from a `ReadRel` operator (e.g. table name)
  string GetSourceName(ReadRel* rel_op) {
    switch (rel_op->read_type_case()) {
      case ReadRel::ReadTypeCase::kNamedTable: {
        auto& catalog_name = rel_op->named_table();

        // Grab the parts of the table name, but ibis only encodes 1.
        std::stringstream tname_stream;
        tname_stream << catalog_name.names(0);
        for (int tname_ndx = 1; tname_ndx < catalog_name.names_size(); ++tname_ndx) {
          tname_stream << "." << catalog_name.names(tname_ndx);
        }

        return tname_stream.str();
      }

      case ReadRel::ReadTypeCase::kLocalFiles: {
        auto& src_files = rel_op->local_files();

        // On success, return a copy of the file path
        if (src_files.items_size() == 1) {
          auto& src_file = src_files.items(0);

          if (src_file.has_uri_path() and src_file.has_arrow()) {
            return string { src_files.items(0).uri_path() };
          }
        }

        std::cerr << "Error: Mohair expects a single URI path to an Arrow file." << std::endl;
        break;
      }

      // all other cases can get a default name for now
      case ReadRel::ReadTypeCase::kVirtualTable:
      case ReadRel::ReadTypeCase::kExtensionTable:
      default:
          std::cerr << "Error: Mohair only supports 'NamedTable' or 'LocalFiles'"
                    << std::endl
          ;

          break;
    }

    // Return empty string when unsuccessful
    return string {};
  }

  //! Extracts the source name from a custom operator used in `ExtensionLeaf`
  string GetSourceName(SkyRel* extrel_op) {
    return string { extrel_op->domain() + "-" + extrel_op->partition() };
  }

  //! Extracts the source name from a custom operator used in `ExtensionLeaf`
  string GetSourceName(SkyPartitionRel* extrel_op) {
    return string { extrel_op->domain() + "-" + extrel_op->partition() };
  }

  //! Extracts the source name from a custom operator used in `ExtensionLeaf`
  string GetSourceName(SkySliceRel* extrel_op) {
    return string { extrel_op->domain() + "-" + extrel_op->partition() };
  }


  // >> Functions to help instantiate `RelOp`

  //! Instantiates a `RelOp` from the "details" of an `ExtensionLeafRel`
  template <typename RelType>
  RelOp<RelType> DispatchToCustomRel(Rel* rel, ExtensionLeafRel* rel_op) {
    if (not rel_op->has_extension()) { return RelOp<ExtensionLeafRel>(rel, nullptr); }

    if (ext_detailt.Is<SkyRel>()) {
      unique_ptr<SkyRel> rel_ext;
      rel_op->detail().UnpackTo(rel_ext.get());

      return RelOp<SkyRel>(rel, std::move(rel_ext));
    }

    else if (ext_detailt.Is<SkySliceRel>()) {
      unique_ptr<SkySliceRel> rel_ext;
      rel_op->detail().UnpackTo(rel_ext.get());

      return RelOp<SkySliceRel>(rel, std::move(rel_ext));
    }

    else if (ext_detailt.Is<SkyPartitionRel>()) {
      unique_ptr<SkyPartitionRel> rel_ext;
      rel_op->detail().UnpackTo(rel_ext.get());

      return RelOp<SkyPartitionRel>(rel, std::move(rel_ext));
    }

    return RelOp<ExtensionLeafRel>(rel, nullptr);
  }

  //! Instantiates a `RelOp` based on the operator type of `Rel`
  template <typename RelType>
  bool RelOpForSubstraitRel(Rel* rel, RelOpVariant* out_relop) {
    if (rel == nullptr) {
      MohairLogMsg("Cannot handle dispatch for null operator");
      return false;
    }

    switch (rel->rel_type_case()) {
      case Rel::RelTypeCase::kProject:
        *out_relop = RelOp(rel, rel->mutable_project());
        break;

      case Rel::RelTypeCase::kFilter:
        *out_relop = RelOp(rel, rel->mutable_filter());
        break;

      case Rel::RelTypeCase::kFetch:
        *out_relop = RelOp(rel, rel->mutable_fetch());
        break;

      case Rel::RelTypeCase::kSort:
        *out_relop = RelOp(rel, rel->mutable_sort());
        break;

      case Rel::RelTypeCase::kAggregate:
        *out_relop = RelOp(rel, rel->mutable_aggregate());
        break;

      case Rel::RelTypeCase::kJoin:
        *out_relop = RelOp(rel, rel->mutable_join());
        break;

      case Rel::RelTypeCase::kCross:
        *out_relop = RelOp(rel, rel->mutable_cross());
        break;

      case Rel::RelTypeCase::kHashJoin:
        *out_relop = RelOp(rel, rel->mutable_hash_join());
        break;

      case Rel::RelTypeCase::kMergeJoin:
        *out_relop = RelOp(rel, rel->mutable_merge_join());
        break;

      case Rel::RelTypeCase::kReference:
        *out_relop = RelOp(rel, rel->mutable_reference());
        break;

      case Rel::RelTypeCase::kRead:
        *out_relop = RelOp(rel, rel->mutable_read());
        break;

      case Rel::RelTypeCase::kExtensionLeaf:
        *out_relop = DispatchToCustomRel(rel, rel->mutable_extension_leaf());
        break;

      default:
        MohairLogMsg("Cannot handle dispatch for operator type: " << rel->rel_type_case());
        return false;
    }

    return true;
  }

  //! Walks a substrait plan to construct easier access to each operator
  OpTreeItr WalkSubstraitPlan(SubstraitPlan* plan) {
    Rel* plan_root  = plan->root_rel->mutable_root()->mutable_input();
    auto op_variant = RelOpForSubstraitRel(plan_root).value_or(
  }

}


// ------------------------------
// Implementations of specialized functions

namespace mohair {

  // >> Implementations for CopySubstraitRel
  unique_ptr<Rel> CopySubstraitRel(Rel* src_rel, ProjectRel* rel_type) {
    unique_ptr<Rel> rel_copy  { std::make_unique<Rel>() };
    ProjectRel&     proj_copy = *(rel_copy->mutable_project());

    proj_copy.mutable_common()->CopyFrom(rel_type->common());
    proj_copy.mutable_advanced_extension()->CopyFrom(rel_type->advanced_extension());

    proj_copy.mutable_expressions()->CopyFrom(rel_type->expressions());

    return rel_copy;
  }

  unique_ptr<Rel> CopySubstraitRel(Rel* src_rel, FilterRel* rel_type) {
    unique_ptr<Rel> rel_copy    { std::make_unique<Rel>() };
    FilterRel&      filter_copy = *(rel_copy->mutable_filter());

    filter_copy.mutable_common()->CopyFrom(rel_type->common());
    filter_copy.mutable_advanced_extension()->CopyFrom(rel_type->advanced_extension());

    filter_copy.mutable_condition()->CopyFrom(rel_type->condition());

    return filter_copy;
  }

  unique_ptr<Rel> CopySubstraitRel(Rel* src_rel, FetchRel* rel_type) {
    unique_ptr<Rel> rel_copy   { std::make_unique<Rel>() };
    FetchRel&       fetch_copy = *(rel_copy->mutable_fetch());

    fetch_copy.mutable_common()->CopyFrom(rel_type->common());
    fetch_copy.mutable_advanced_extension()->CopyFrom(rel_type->advanced_extension());

    if (rel_type->has_offset_expr()) {
      fetch_copy.mutable_offset_expr()->CopyFrom(rel_type->offset_expr());
    }
    else { fetch_copy.set_offset(rel_type->offset()); }

    if (rel_type->has_count_expr()) {
      fetch_copy.mutable_count_expr()->CopyFrom(rel_type->count_expr());
    }
    else { fetch_copy.set_count(rel_type->count()); }

    return fetch_copy;
  }

  unique_ptr<Rel> CopySubstraitRel(Rel* src_rel, SortRel* rel_type) {
    unique_ptr<Rel> rel_copy  { std::make_unique<Rel>() };
    SortRel&        sort_copy = *(rel_copy->mutable_sort());

    sort_copy.mutable_common()->CopyFrom(rel_type->common());
    sort_copy.mutable_advanced_extension()->CopyFrom(rel_type->advanced_extension());

    sort_copy.mutable_sorts()->CopyFrom(rel_type->sorts());

    return sort_copy;
  }

  unique_ptr<Rel> CopySubstraitRel(Rel* src_rel, AggregateRel* rel_type) {
    unique_ptr<Rel> rel_copy  { std::make_unique<Rel>() };
    AggregateRel&   agg_copy = *(rel_copy->mutable_aggregate());

    agg_copy.mutable_common()->CopyFrom(rel_type->common());
    agg_copy.mutable_advanced_extension()->CopyFrom(rel_type->advanced_extension());

    agg_copy.mutable_groupings()->CopyFrom(rel_type->groupings());
    agg_copy.mutable_measures()->CopyFrom(rel_type->measures());
    agg_copy.mutable_grouping_expressions()->CopyFrom(rel_type->grouping_expressions());

    return agg_copy;
  }

  unique_ptr<Rel> CopySubstraitRel(Rel* src_rel, JoinRel* rel_type) {
    unique_ptr<Rel> rel_copy  { std::make_unique<Rel>() };
    JoinRel&        join_copy = *(rel_copy->mutable_join());

    join_copy.mutable_common()->CopyFrom(rel_type->common());
    join_copy.mutable_advanced_extension()->CopyFrom(rel_type->advanced_extension());

    join_copy.mutable_expression()->CopyFrom(rel_type->expression());
    join_copy.mutable_post_join_filter()->CopyFrom(rel_type->post_join_filter());
    join_copy.set_type(rel_type->type());

    return join_copy;
  }

  unique_ptr<Rel> CopySubstraitRel(Rel* src_rel, CrossRel* rel_type) {
    unique_ptr<Rel> rel_copy   { std::make_unique<Rel>() };
    CrossRel&       cross_copy = *(rel_copy->mutable_cross());

    cross_copy.mutable_common()->CopyFrom(rel_type->common());
    cross_copy.mutable_advanced_extension()->CopyFrom(rel_type->advanced_extension());

    return cross_copy;
  }

  unique_ptr<Rel> CopySubstraitRel(Rel* src_rel, HashJoinRel* rel_type) {
    unique_ptr<Rel> rel_copy  { std::make_unique<Rel>() };
    HashJoinRel&    hash_copy = *(rel_copy->mutable_hash_join());

    hash_copy.mutable_common()->CopyFrom(rel_type->common());
    hash_copy.mutable_advanced_extension()->CopyFrom(rel_type->advanced_extension());

    hash_copy.set_post_join_filter(rel_type->post_join_filter());
    hash_copy.set_type(rel_type->type());

    if (rel_type->has_left_keys()) {
      hash_copy.mutable_left_keys()->CopyFrom(rel_type->left_keys());
    }

    if (rel_type->has_right_keys()) {
      hash_copy.mutable_right_keys()->CopyFrom(rel_type->right_keys());
    }

    if (rel_type->has_keys()) { hash_copy.mutable_keys()->CopyFrom(rel_type->keys()); }


    return hash_copy;
  }

  unique_ptr<Rel> CopySubstraitRel(Rel* src_rel, MergeJoinRel* rel_type) {
    unique_ptr<Rel> rel_copy   { std::make_unique<Rel>() };
    MergeJoinRel&   merge_copy = *(rel_copy->mutable_merge_join());

    merge_copy.mutable_common()->CopyFrom(rel_type->common());
    merge_copy.mutable_advanced_extension()->CopyFrom(rel_type->advanced_extension());

    merge_copy.set_post_join_filter(rel_type->post_join_filter());
    merge_copy.set_type(rel_type->type());

    if (rel_type->has_left_keys()) {
      merge_copy.mutable_left_keys()->CopyFrom(rel_type->left_keys());
    }

    if (rel_type->has_right_keys()) {
      merge_copy.mutable_right_keys()->CopyFrom(rel_type->right_keys());
    }

    if (rel_type->has_keys()) { merge_copy.mutable_keys()->CopyFrom(rel_type->keys()); }

    return merge_copy;
  }


  // >> Implementations for StringifyOp
  const string StringifyOp(ProjectRel*   rel_op) { return u8"Π()";    }
  const string StringifyOp(FilterRel*    rel_op) { return u8"σ()";    }
  const string StringifyOp(FetchRel*     rel_op) { return u8"Lim()";  }
  const string StringifyOp(SortRel*      rel_op) { return u8"Sort()"; }
  const string StringifyOp(AggregateRel* rel_op) { return u8"Aggr()"; }
  const string StringifyOp(JoinRel*      rel_op) { return u8"⋈()";    }
  const string StringifyOp(CrossJoinRel* rel_op) { return u8"×()";    }
  const string StringifyOp(HashJoinRel*  rel_op) { return u8"⋈→()";   }
  const string StringifyOp(MergeJoinRel* rel_op) { return u8"⋈⊕()";   }

  const string StringifyOp(ExtensionLeafRel* rel_op) {
    return u8"ExtensionLeaf()";
  }

  const string StringifyOp(ReferenceRel* rel_op) {
    return u8"Ref(" + rel_op->subtree_reference() + ")";
  }

  const string StringifyOp(ReadRel* rel_op) {
    return u8"Read(" + GetSourceName(rel_op) + u8")";
  }

  const string StringifyOp(SkyRel* rel_op) {
    return u8"SkyRead(" + GetSourceName(rel_op) + u8")";
  }

  const string StringifyOp(SkyPartitionRel* rel_op) {
    return u8"SkyPartitionRead(" + GetSourceName(rel_op) + u8")";
  }

  const string StringifyOp(SkySliceRel* rel_op) {
    return u8"SkySliceRead(" + GetSourceName(rel_op) + u8")";
  }

} // namespace: mohair
