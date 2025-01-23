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


// ------------------------------
// Classes

namespace mohair {

  // >> Structs representing types (tags)

  //! Tags to represent the "arity" of a relation (how many inputs)
  struct TagOpArity  {};
  struct TagLeaf   : TagOpArity {};
  struct TagUnary  : TagOpArity {}; 
  struct TagBinary : TagOpArity {};

  //! Tags to represent how data flows through the operator
  struct TagOpFlow   {};
  struct TagOrigin : TagOpFlow {};
  struct TagSource : TagOpFlow {};
  struct TagSink   : TagOpFlow {};
  struct TagStream : TagOpFlow {};


  // >> Base types to hold tags
  template <typename RelType>
  struct RelTraits {};


  // >> Tagging for relational operator types (e.g. `ProjectRel, `FetchRel`, etc.)

  // Unary operators
  template <>
  struct RelTraits<ProjectRel> {
    using Arity = TagUnary;
    using Flow  = TagStream;
  };

  template <>
  struct RelTraits<FilterRel> {
    using Arity = TagUnary;
    using Flow  = TagStream;
  };

  template <>
  struct RelTraits<FetchRel> {
    using Arity = TagUnary;
    using Flow  = TagStream;
  };

  template <>
  struct RelTraits<SortRel> {
    using Arity = TagUnary;
    using Flow  = TagSink;
  };

  template <>
  struct RelTraits<AggregateRel> {
    using Arity = TagUnary;
    using Flow  = TagSink;
  };


  // Binary operators (various types of joins)
  template <>
  struct RelTraits<JoinRel> {
    using Arity = TagBinary;
    using Flow  = TagSink;
  };

  template <>
  struct RelTraits<CrossRel> {
    using Arity = TagBinary;
    using Flow  = TagSink;
  };

  template <>
  struct RelTraits<HashJoinRel> {
    using Arity = TagBinary;
    using Flow  = TagSink;
  };

  template <>
  struct RelTraits<MergeJoinRel> {
    using Arity = TagBinary;
    using Flow  = TagSink;
  };


  // Leaf operators
  template <>
  struct RelTraits<ReferenceRel> {
    using Arity = TagLeaf;
    using Flow  = TagSource;
  };

  template <>
  struct RelTraits<ExtensionLeafRel> {
    using Arity = TagLeaf;
    using Flow  = TagOrigin;
  };

  template <>
  struct RelTraits<ReadRel> {
    using Arity = TagLeaf;
    using Flow  = TagOrigin;
  };


  // >> Template tag logic

  // Logic for arity checking
  template <typename RelType>
  using is_leaf = std::is_same<TagLeaf, RelTraits<RelType>::Arity>;

  template <typename RelType>
  using is_unary = std::is_same<TagUnary, RelTraits<RelType>::Arity>;

  template <typename RelType>
  using is_binary = std::is_same<TagBinary, RelTraits<RelType>::Arity>;

  // Logic for checking data flow properties
  template <typename RelType>
  using is_origin = std::is_same<TagOrigin, RelTraits<RelType>::Flow>;

  template <typename RelType>
  using is_source = std::is_same<TagSource, RelTraits<RelType>::Flow>;

  template <typename RelType>
  using is_sink = std::is_same<TagSink, RelTraits<RelType>::Flow>;

  template <typename RelType>
  using is_stream = std::is_same<TagStream, RelTraits<RelType>::Flow>;

  // >> SFINAE aliases

  // SFINAE for Arity
  template <typename RelType, typename RType = void>
  using enable_if_leafop = std::enable_if_t<is_leaf<RelType>::value, RType>;

  template <typename RelType, typename RType = void>
  using enable_if_unaryop = std::enable_if_t<is_unary<RelType>::value, RType>;

  template <typename RelType, typename RType = void>
  using enable_if_binaryop = std::enable_if_t<is_binary<RelType>::value, RType>;

  // SFINAE for data flow
  template <typename RelType, typename RType = void>
  using enable_if_originop = std::enable_if_t<is_origin<RelType>::value, RType>;

  template <typename RelType, typename RType = void>
  using enable_if_sourceop = std::enable_if_t<is_source<RelType>::value, RType>;

  template <typename RelType, typename RType = void>
  using enable_if_sinkop = std::enable_if_t<is_sink<RelType>::value, RType>;

  template <typename RelType, typename RType = void>
  using enable_if_streamop = std::enable_if_t<is_stream<RelType>::value, RType>;



} // namespace: mohair
