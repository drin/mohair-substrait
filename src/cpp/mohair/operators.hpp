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
#include <array>

#include "mohair.hpp"

#include "mohair/operators/op_traits.hpp"
#include "mohair/analysis/schema_resolution.hpp"


// ------------------------------
// Templated functions

namespace mohair {

  // >> Templated functions to construct non-recursive message comparators
  template <typename RelType>
  unique_ptr<MessageDifferencer> UnaryRelComparator() {
    auto msg_differ = std::make_unique<MessageDifferencer>();
    msg_differ->IgnoreField(RelType::descriptor()->FindFieldByName("input"));

    return msg_differ;
  }

  template <typename RelType>
  unique_ptr<MessageDifferencer> BinaryRelComparator() {
    auto msg_differ = std::make_unique<MessageDifferencer>();
    msg_differ->IgnoreField(RelType::descriptor()->FindFieldByName("left"));
    msg_differ->IgnoreField(RelType::descriptor()->FindFieldByName("right"));

    return msg_differ;
  }

} // namespace: mohair


// ------------------------------
// Operator classes

namespace mohair {

  // >> Forward declarations
  struct PlanPipeline;
  struct PipelineStage;
  struct OpPipeline;

  //! A type-erased base class representing a query operator
  struct SubstraitOp {
    virtual ~SubstraitOp() = default;

    // API for inspecting the Rel and the op it represents
    virtual optional<bool>   IsSink()    const { return std::nullopt; }
    virtual optional<bool>   IsSource()  const { return std::nullopt; }
    virtual optional<bool>   IsOrigin()  const { return std::nullopt; }
    virtual optional<bool>   IsStream()  const { return std::nullopt; }
    virtual optional<size_t> GetArity()  const { return std::nullopt; }
    virtual string_view      Stringify() const { return ""sv;         }

    // API for comparison and manipulation
    virtual unique_ptr<Rel>     CopyRelOp()        const { return nullptr; }
    virtual MessageDifferencer* GetComparator()    const { return nullptr; }

    virtual bool
    SetAliases([[maybe_unused]] const RepeatedPtrField<string>& aliases) {
      return false;
    }

    // API for visits
    virtual void
    BuildPipelines(PlanPipeline& plan_pipe, PipelineStage& pipe_stage);

    // Static functions
    static unique_ptr<SubstraitOp> FromRel(Rel* rel);
  };

  // TODO:
  // - need to accommodate table name (needed for pipeline and pipeline name)
  // - need to accommodate extension rels and the deserialized payload (the actual op)

  //! Implementation for SubstraitOp that specializes for each substrait Rel type
  template <typename RelType>
  struct SubstraitOpImpl : public SubstraitOp {
    using OpTraits     = RelTraits<RelType>;
    using InputArray   = array<unique_ptr<SubstraitOp>, OpTraits::Arity>;
    using ResultSchema = unique_ptr<SubstraitSchema>;

    // Member attributes
    Rel*         rel;
    RelType*     rel_op;
    InputArray   input_ops;
    ResultSchema schema;

    // Destructors and Constructors
    SubstraitOpImpl(Rel* srel, RelType* srel_op): rel(srel), rel_op(srel_op) {}

    // Public API (via SubstraitOp)
    optional<bool>   IsSink()   const override;
    optional<bool>   IsSource() const override;
    optional<bool>   IsOrigin() const override;
    optional<bool>   IsStream() const override;

    optional<size_t> GetArity()  const override;
    string_view      Stringify() const override;

    unique_ptr<Rel>     CopyRelOp()        const override;
    MessageDifferencer* GetComparator()    const override;

    bool SetAliases(const RepeatedPtrField<string>& aliases) override;

    void BuildPipelines(PlanPipeline& plan_pipe, PipelineStage& pipe_stage) override;

    // Internal API (can only be called directly on SubstraitOpImpl<RelType>)
    bool BindLogicalSchema();

    void AddToPipeline(
       PlanPipeline&  plan_pipe
      ,PipelineStage& pipe_stage
      ,OpPipeline&    pipeline
    );

    Rel* MoveToRel(Rel* new_srel);
  };

  //! Implementation for SubstraitOp that specializes for ReferenceRel
  template <>
  struct SubstraitOpImpl<ReferenceRel> : public SubstraitOp {
    using OpTraits = RelTraits<ReferenceRel>;

    // Member attributes
    Rel*                        rel;
    ReferenceRel*               rel_op;
    unique_ptr<SubstraitSchema> schema;
    unique_ptr<SubstraitOp>     subplan_root;

    // Destructors and Constructors
    SubstraitOpImpl(Rel* srel, ReferenceRel* srel_op): rel(srel), rel_op(srel_op) {}

    // Public API (via SubstraitOp)
    optional<bool>   IsSink()    const override;
    optional<bool>   IsSource()  const override;
    optional<bool>   IsOrigin()  const override;
    optional<bool>   IsStream()  const override;
    optional<size_t> GetArity()  const override;
    string_view      Stringify() const override;

    unique_ptr<Rel>     CopyRelOp()     const override;
    MessageDifferencer* GetComparator() const override;

    bool SetAliases(const RepeatedPtrField<string>& aliases) override;

    void BuildPipelines(PlanPipeline& plan_pipe, PipelineStage& pipe_stage) override;

    // Internal API (can only be called directly on SubstraitOpImpl<RelType>)
    Rel* MoveToRel(Rel* new_srel);
  };

  //! Implementation for SubstraitOp that specializes for ReadRel
  template <>
  struct SubstraitOpImpl<ReadRel> : public SubstraitOp {
    using OpTraits = RelTraits<ReadRel>;

    // Member attributes
    Rel*                        rel;
    ReadRel*                    rel_op;
    unique_ptr<SubstraitSchema> schema;
    string                      source_name;

    // Destructors and Constructors
    SubstraitOpImpl(Rel* srel, ReadRel* srel_op): rel(srel), rel_op(srel_op) {}

    // Public API (via SubstraitOp)
    optional<bool>   IsSink()    const override;
    optional<bool>   IsSource()  const override;
    optional<bool>   IsOrigin()  const override;
    optional<bool>   IsStream()  const override;
    optional<size_t> GetArity()  const override;

    // TODO: figure out how to do this
    string_view Stringify() const override {
      return "Read("sv + source_name + ")"sv;
    }

    unique_ptr<Rel>     CopyRelOp()     const override;
    MessageDifferencer* GetComparator() const override;

    bool SetAliases(const RepeatedPtrField<string>& aliases) override;

    void BuildPipelines(PlanPipeline& plan_pipe, PipelineStage& pipe_stage) override;

    // Internal API (can only be called directly on SubstraitOpImpl<RelType>)
    Rel* MoveToRel(Rel* new_srel);
  };

  //! Implementation for SubstraitOp that specializes for a custom leaf operator
  //  TODO: need to support the actual extension payload (maybe another type-erased type)
  template <>
  struct SubstraitOpImpl<ExtensionLeafRel> : public SubstraitOp {
    using OpTraits = RelTraits<ExtensionLeafRel>;

    // Member attributes
    Rel*                        rel;
    ExtensionLeafRel*           rel_op;
    unique_ptr<SubstraitSchema> schema;
    string                      source_name;

    // Destructors and Constructors
    SubstraitOpImpl(Rel* srel, ExtensionLeafRel* srel_op): rel(srel), rel_op(srel_op) {}

    // Public API (via SubstraitOp)
    optional<bool>   IsSink()    const override;
    optional<bool>   IsSource()  const override;
    optional<bool>   IsOrigin()  const override;
    optional<bool>   IsStream()  const override;
    optional<size_t> GetArity()  const override;

    string_view Stringify() const override {
      return "ExtLeaf("sv + source_name + ")"sv;
    }

    unique_ptr<Rel>     CopyRelOp()     const override;
    MessageDifferencer* GetComparator() const override;

    bool SetAliases(const RepeatedPtrField<string>& aliases) override;

    void BuildPipelines(PlanPipeline& plan_pipe, PipelineStage& final_stage) override;

    // Internal API (can only be called directly on SubstraitOpImpl<RelType>)
    Rel* MoveToRel(Rel* new_srel);
  };

} // namespace: mohair

