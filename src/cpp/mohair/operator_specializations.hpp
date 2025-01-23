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


#include "mohair/adapters/adapter_standard.hpp"
#include "mohair/apidep_substrait.hpp"
#include "mohair/operator_traits.hpp"

#include "mohair/plans.hpp"


// ------------------------------
// Specializations (for use by `RelOp`)

namespace mohair {

  // >> Specializations for IsSink
  template <typename RelType>
  constexpr bool IsSinkOp(RelType*)          { return false; }

  constexpr bool IsSinkOp(ReferenceRel*)     { return false; }
  constexpr bool IsSinkOp(ReadRel*)          { return false; }
  constexpr bool IsSinkOp(ExtensionLeafRel*) { return false; }
  constexpr bool IsSinkOp(ProjectRel*)       { return false; }
  constexpr bool IsSinkOp(FilterRel*)        { return false; }
  constexpr bool IsSinkOp(FetchRel*)         { return false; }

  constexpr bool IsSinkOp(SortRel*)          { return true; }
  constexpr bool IsSinkOp(AggregateRel*)     { return true; }
  constexpr bool IsSinkOp(JoinRel*)          { return true; }
  constexpr bool IsSinkOp(CrossRel*)         { return true; }
  constexpr bool IsSinkOp(HashJoinRel*)      { return true; }
  constexpr bool IsSinkOp(MergeJoinRel*)     { return true; }


  // >> Specializations for IsOrigin
  template <typename RelType>
  constexpr enable_if_originop<RelType, bool>
  IsOriginOp(RelType*)          { return false; }

  constexpr bool IsOriginOp(ProjectRel*)       { return false; }
  constexpr bool IsOriginOp(FilterRel*)        { return false; }
  constexpr bool IsOriginOp(FetchRel*)         { return false; }
  constexpr bool IsOriginOp(SortRel*)          { return false; }
  constexpr bool IsOriginOp(AggregateRel*)     { return false; }
  constexpr bool IsOriginOp(JoinRel*)          { return false; }
  constexpr bool IsOriginOp(CrossRel*)         { return false; }
  constexpr bool IsOriginOp(HashJoinRel*)      { return false; }
  constexpr bool IsOriginOp(MergeJoinRel*)     { return false; }

  constexpr bool IsOriginOp(ReferenceRel*)     { return true; }
  constexpr bool IsOriginOp(ReadRel*)          { return true; }
  constexpr bool IsOriginOp(ExtensionLeafRel*) { return true; }


  // >> Specializations for iterating over Rel inputs
  template <typename RelType>
  enable_if_leafop<RelType, OpTreeItr>
  GetInputs(Rel* rel, RelType* rel_type) {
    return OpTreeItr(rel, {});
  }

  template <typename RelType>
  enable_if_unaryop<RelType, OpTreeItr>
  GetInputs(Rel* rel, RelType* rel_type) {
    return OpTreeItr(rel, { rel_type->mutable_input() });
  }

  template <typename RelType>
  enable_if_binaryop<RelType, OpTreeItr>
  GetInputs(Rel* rel, RelType* rel_type) {
    return OpTreeItr(rel, { rel_type->mutable_left(), rel_type->mutable_right() });
  }


  // >> Specializations for CopyRel
  //    (efficiently copies a `Rel` by not recursing on inputs)

  //! For an unspecified RelType, do a normal `CopyFrom`
  template <typename RelType>
  unique_ptr<Rel> CopySubstraitRel(Rel* src_rel, RelType* rel_type) {
    unique_ptr<Rel> rel_copy  { std::make_unique<Rel>() };
    rel_copy->CopyFrom(*src_rel);

    return rel_copy;
  }

  unique_ptr<Rel> CopySubstraitRel(Rel* src_rel, ProjectRel*   rel_type);
  unique_ptr<Rel> CopySubstraitRel(Rel* src_rel, FilterRel*    rel_type);
  unique_ptr<Rel> CopySubstraitRel(Rel* src_rel, FetchRel*     rel_type);
  unique_ptr<Rel> CopySubstraitRel(Rel* src_rel, SortRel*      rel_type);
  unique_ptr<Rel> CopySubstraitRel(Rel* src_rel, AggregateRel* rel_type);
  unique_ptr<Rel> CopySubstraitRel(Rel* src_rel, JoinRel*      rel_type);
  unique_ptr<Rel> CopySubstraitRel(Rel* src_rel, CrossRel*     rel_type);
  unique_ptr<Rel> CopySubstraitRel(Rel* src_rel, HashJoinRel*  rel_type);
  unique_ptr<Rel> CopySubstraitRel(Rel* src_rel, MergeJoinRel* rel_type);


  // >> Specializations for StringifyOp
  template <typename RelType>
  const string StringifyOp(RelType* rel_op) { return u8"Unknown()"; }

  const string StringifyOp(ProjectRel*       rel_op);
  const string StringifyOp(FilterRel*        rel_op);
  const string StringifyOp(FetchRel*         rel_op);
  const string StringifyOp(SortRel*          rel_op);
  const string StringifyOp(AggregateRel*     rel_op);
  const string StringifyOp(JoinRel*          rel_op);
  const string StringifyOp(CrossJoinRel*     rel_op);
  const string StringifyOp(HashJoinRel*      rel_op);
  const string StringifyOp(MergeJoinRel*     rel_op);
  const string StringifyOp(ReferenceRel*     rel_op);
  const string StringifyOp(ReadRel*          rel_op);
  const string StringifyOp(ExtensionLeafRel* rel_op);
  const string StringifyOp(SkyRel*           rel_op);
  const string StringifyOp(SkyPartitionRel*  rel_op);
  const string StringifyOp(SkySliceRel*      rel_op);

  // >> Specializations for message comparison (non-recursive)
  //! Instantiate a MessageDifferencer that ignores "input"
  unique_ptr<MessageDifferencer> UnaryComparator(MessageDescriptor* desc);

  //! Instantiate a MessageDifferencer that ignores "left" and "right"
  unique_ptr<MessageDifferencer> BinaryComparator(MessageDescriptor* desc);

  //! A templated function to catch any `RelType` we don't have a specialization for
  template <typename RelType>
  unique_ptr<MessageDifferencer> RelComparator(RelType*) {
    return std::make_unique<MessageDifferencer>();
  }

  //! Templated functions that use SFINAE to instantiate the appropriate "differencer"
  template <typename RelType>
  enable_if_unaryop<RelType, unique_ptr<MessageDifferencer>>
  RelComparator(RelType*) {
    auto differ = std::make_unique<MessageDifferencer>();
    differ->IgnoreField(RelType::descriptor()->FindFieldByName("input"));

    return differ;
  }

  template <typename RelType>
  enable_if_binaryop<RelType, unique_ptr<MessageDifferencer>>
  RelComparator(RelType*) {
    auto differ = std::make_unique<MessageDifferencer>();
    differ->IgnoreField(RelType::descriptor()->FindFieldByName("left"));
    differ->IgnoreField(RelType::descriptor()->FindFieldByName("right"));

    return differ;
  }


  //! Manages a singleton `MessageDifferencer` per `RelType` value and returns it
  template <typename RelType>
  MessageDifferencer* GetComparator(RelType* rel_op) {
    static unique_ptr<MessageDifferencer> differ = RelComparator(rel_op);

    return differ.get();
  }

} // namespace: mohair
