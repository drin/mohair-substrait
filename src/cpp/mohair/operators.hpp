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

// >> Standard headers
#include <array>

#include "mohair/plans.hpp"


// ------------------------------
// Operator classes

namespace mohair {

  // >> Convenience aliases and types
  template <size_t input_arity>
  using InputArity = array<unique_ptr<MohairOp>, input_arity>;

  using LeafInputType   = InputArity<0>;
  using UnaryInputType  = InputArity<1>;
  using BinaryInputType = InputArity<2>;

  // >> Base classes
  struct SinkOp : public MohairOp {
    SinkOp(Rel* rel): MohairOp(rel) {}

    bool IsSink() override { return true; }
  };

  // >> Leaf operators
  struct OpErr : MohairOp {
    string err_msg;

    OpErr(Rel *rel, const char *msg): MohairOp(rel), err_msg(msg) {}

    const string ToString() override;
  };

  struct OpReference : MohairOp {
    ReferenceRel* rel_op;

    OpReference(ReferenceRel* op, Rel* rel)
      : MohairOp(rel), rel_op(op) {}

    const string ToString() override;
  };

  struct OpRead : SourceOp {
    ReadRel* rel_op;

    OpRead(ReadRel* op, Rel* rel, string tname)
      : SourceOp(rel, tname), rel_op(op) {}

    const string ToString() override;
  };

  //! An operator that represents an extension operator holding a `mohair::SkyRel`.
  struct OpSkyRead : SourceOp {
    ExtensionLeafRel*  rel_op;
    unique_ptr<SkyRel> sky_rel;

    OpSkyRead( ExtensionLeafRel*    op
              ,Rel*                 rel
              ,unique_ptr<SkyRel>&& unpacked_rel
              ,string&              tname)
      : SourceOp(rel, tname), rel_op(op), sky_rel(std::move(unpacked_rel)) {}

    const string ToString() override;
  };

  struct OpPartitionRead : SourceOp {
    ExtensionLeafRel*           rel_op;
    unique_ptr<SkyPartitionRel> sky_rel;

    OpPartitionRead( ExtensionLeafRel*             op
                    ,Rel*                          rel
                    ,unique_ptr<SkyPartitionRel>&& unpacked_rel
                    ,string&                       tname)
      : SourceOp(rel, tname), rel_op(op), sky_rel(std::move(unpacked_rel)) {}

    const string ToString() override;
  };

  struct OpSliceRead : SourceOp {
    ExtensionLeafRel*       rel_op;
    unique_ptr<SkySliceRel> sky_rel;

    OpSliceRead( ExtensionLeafRel*         op
                ,Rel*                      rel
                ,unique_ptr<SkySliceRel>&& unpacked_rel
                ,string&                   tname)
      : SourceOp(rel, tname), rel_op(op), sky_rel(std::move(unpacked_rel)) {}

    const string ToString() override;
  };

  // >> Unary operators (stream-able)
  struct OpProj : public MohairOp {
    ProjectRel*    rel_op;
    UnaryInputType op_inputs;

    OpProj(ProjectRel *op, Rel *rel, unique_ptr<MohairOp>&& input_op)
      : MohairOp(rel), rel_op(op), op_inputs({ std::move(input_op) }) {}

    const string ToString()   override;
    size_t       GetOpArity() override;

    unique_ptr<MohairOp>* GetOpInputs() override;
  };

  struct OpSel : public MohairOp {
    FilterRel*     rel_op;
    UnaryInputType op_inputs;

    OpSel(FilterRel *op, Rel *rel, unique_ptr<MohairOp>&& input_op)
      : MohairOp(rel), rel_op(op), op_inputs({ std::move(input_op) }) {}

    const string ToString()   override;
    size_t       GetOpArity() override;

    unique_ptr<MohairOp>* GetOpInputs() override;
  };

  struct OpLimit : public MohairOp {
    FetchRel*      rel_op;
    UnaryInputType op_inputs;

    OpLimit(FetchRel *op, Rel *rel, unique_ptr<MohairOp>&& input_op)
      : MohairOp(rel), rel_op(op), op_inputs({ std::move(input_op) }) {}

    const string ToString()   override;
    size_t       GetOpArity() override;

    unique_ptr<MohairOp>* GetOpInputs() override;
  };

  // >> Unary operators (sinks)
  struct OpSort : public SinkOp {
    SortRel*       rel_op;
    UnaryInputType op_inputs;

    OpSort(SortRel *op, Rel *rel, unique_ptr<MohairOp>&& input_op)
      : SinkOp(rel), rel_op(op), op_inputs({ std::move(input_op) }) {}

    const string ToString()   override;
    size_t       GetOpArity() override;

    unique_ptr<MohairOp>* GetOpInputs() override;
  };

  struct OpAggr : public SinkOp {
    AggregateRel*  rel_op;
    UnaryInputType op_inputs;

    OpAggr(AggregateRel *op, Rel *rel, unique_ptr<MohairOp>&& input_op)
      : SinkOp(rel), rel_op(op), op_inputs({ std::move(input_op) }) {}

    const string ToString()   override;
    size_t       GetOpArity() override;

    unique_ptr<MohairOp>* GetOpInputs() override;
  };

  // >> Binary operators (sinks)
  struct OpJoin : public SinkOp {
    JoinRel*        rel_op;
    BinaryInputType op_inputs;

    OpJoin(JoinRel *op, Rel *rel, unique_ptr<MohairOp>&& left, unique_ptr<MohairOp>&& right)
      : SinkOp(rel), rel_op(op), op_inputs({ std::move(left), std::move(right) }) {}

    const string ToString()   override;
    size_t       GetOpArity() override;

    unique_ptr<MohairOp>* GetOpInputs() override;
  };

  struct OpCrossJoin : public SinkOp {
    CrossRel*       rel_op;
    BinaryInputType op_inputs;

    OpCrossJoin(CrossRel *op, Rel *rel, unique_ptr<MohairOp>&& left, unique_ptr<MohairOp>&& right)
      : SinkOp(rel), rel_op(op), op_inputs({ std::move(left), std::move(right) }) {}

    const string ToString()   override;
    size_t       GetOpArity() override;

    unique_ptr<MohairOp>* GetOpInputs() override;
  };

  struct OpHashJoin : public SinkOp {
    HashJoinRel*    rel_op;
    BinaryInputType op_inputs;

    OpHashJoin(HashJoinRel *op, Rel *rel, unique_ptr<MohairOp>&& left, unique_ptr<MohairOp>&& right)
      : SinkOp(rel), rel_op(op), op_inputs({ std::move(left), std::move(right) }) {}

    const string ToString()   override;
    size_t       GetOpArity() override;

    unique_ptr<MohairOp>* GetOpInputs() override;
  };

  struct OpMergeJoin : public SinkOp {
    MergeJoinRel*   rel_op;
    BinaryInputType op_inputs;

    OpMergeJoin(MergeJoinRel *op, Rel *rel, unique_ptr<MohairOp>&& left, unique_ptr<MohairOp>&& right)
      : SinkOp(rel), rel_op(op), op_inputs({ std::move(left), std::move(right) }) {}

    const string ToString()   override;
    size_t       GetOpArity() override;

    unique_ptr<MohairOp>* GetOpInputs() override;
  };

} // namespace: mohair

