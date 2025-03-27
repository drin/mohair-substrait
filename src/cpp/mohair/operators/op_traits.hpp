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

#include "mohair/apidep_standard.hpp"
#include "mohair/adapter_substrait.hpp"


// ------------------------------
// Variables
namespace mohair {

  //! Constants to represent the "arity" of a relation (input cardinality)
  static constexpr size_t arity_leaf   { 0 };
  static constexpr size_t arity_unary  { 1 };
  static constexpr size_t arity_binary { 2 };

} // namespace: mohair


// ------------------------------
// Classes

namespace mohair {

  // >> Structs representing types (tags)

  //! Tags to represent how data flows through the operator
  struct TagOpFlow   {};
  struct TagFlowErr : TagOpFlow {};
  struct TagOrigin  : TagOpFlow {};
  struct TagSource  : TagOpFlow {};
  struct TagSink    : TagOpFlow {};
  struct TagStream  : TagOpFlow {};

  // >> Base types to hold tags
  template <typename RelType>
  struct RelTraits {
    using Flow = TagFlowErr;

    static constexpr string_view OpSymbol  { "?"sv };
  };


  // >> Tagging for relational operator types (e.g. `ProjectRel, `FetchRel`, etc.)

  // Unary operators
  template <>
  struct RelTraits<ProjectRel> {
    using Flow = TagStream;

    static constexpr size_t      Arity    { arity_unary };
    static constexpr string_view OpSymbol { "Π"sv       };
  };

  template <>
  struct RelTraits<FilterRel> {
    using Flow = TagStream;

    static constexpr size_t      Arity    { arity_unary };
    static constexpr string_view OpSymbol { "σ"sv       };
  };

  template <>
  struct RelTraits<FetchRel> {
    using Flow = TagStream;

    static constexpr size_t      Arity    { arity_unary };
    static constexpr string_view OpSymbol { "lim"sv     };
  };

  template <>
  struct RelTraits<SortRel> {
    using Flow = TagSink;

    static constexpr size_t      Arity    { arity_unary };
    static constexpr string_view OpSymbol { "Φ"sv       };
  };

  template <>
  struct RelTraits<AggregateRel> {
    using Flow = TagSink;

    static constexpr size_t      Arity    { arity_unary };
    static constexpr string_view OpSymbol { "Γ"sv       };
  };


  // Binary operators (various types of joins)
  template <>
  struct RelTraits<JoinRel> {
    using Flow = TagSink;

    static constexpr size_t      Arity    { arity_binary };
    static constexpr string_view OpSymbol { "⋈"sv        };
  };

  template <>
  struct RelTraits<CrossRel> {
    using Flow = TagSink;

    static constexpr size_t      Arity    { arity_unary };
    static constexpr string_view OpSymbol { "×"sv       };
  };

  template <>
  struct RelTraits<HashJoinRel> {
    using Flow = TagSink;

    static constexpr size_t      Arity    { arity_unary };
    static constexpr string_view OpSymbol { "⋈→"sv      };
  };

  template <>
  struct RelTraits<MergeJoinRel> {
    using Flow = TagSink;

    static constexpr size_t      Arity    { arity_unary };
    static constexpr string_view OpSymbol { "⋈⊕"sv      };
  };


  // Leaf operators
  template <>
  struct RelTraits<ReferenceRel> {
    using Flow = TagOrigin;

    static constexpr size_t      Arity    { arity_leaf };
    static constexpr string_view OpSymbol { "Ref"sv    };
  };

  template <>
  struct RelTraits<ReadRel> {
    using Flow = TagOrigin;

    static constexpr size_t      Arity    { arity_leaf };
    static constexpr string_view OpSymbol { "Read"sv   };
  };

  template <>
  struct RelTraits<ExtensionLeafRel> {
    using Flow = TagOrigin;

    static constexpr size_t      Arity    { arity_leaf };
    static constexpr string_view OpSymbol { "Ext"sv    };
  };

  template <>
  struct RelTraits<SkyRel> {
    using Flow = TagOrigin;

    static constexpr size_t      Arity    { arity_leaf };
    static constexpr string_view OpSymbol { "SkyRel"sv };
  };

  template <>
  struct RelTraits<SkySliceRel> {
    using Flow  = TagOrigin;

    static constexpr size_t      Arity    { arity_leaf   };
    static constexpr string_view OpSymbol { "SliceRel"sv };
  };

  template <>
  struct RelTraits<SkyPartitionRel> {
    using Flow  = TagOrigin;

    static constexpr size_t      Arity    { arity_leaf       };
    static constexpr string_view OpSymbol { "PartitionRel"sv };
  };


  // >> Template tag logic

  // Logic for checking data flow properties
  template <typename RelType>
  using is_origin = std::is_same<TagOrigin, typename RelTraits<RelType>::Flow>;

  template <typename RelType>
  using is_source = std::is_same<TagSource, typename RelTraits<RelType>::Flow>;

  template <typename RelType>
  using is_sink = std::is_same<TagSink, typename RelTraits<RelType>::Flow>;

  template <typename RelType>
  using is_stream = std::is_same<TagStream, typename RelTraits<RelType>::Flow>;


  // >> SFINAE aliases

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

