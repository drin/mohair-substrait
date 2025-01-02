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

#include "mohair/operators.hpp"


// ------------------------------
// Operator class implementations

namespace mohair {

  // >> Implementations for the base class
  const string MohairOp::ToString() { return "MohairOp";       }
  const string MohairOp::ViewStr()  { return this->ToString(); }

  bool   MohairOp::IsSink()     { return false; }
  bool   MohairOp::IsOrigin()   { return false; }
  size_t MohairOp::GetOpArity() { return 0;     }

  unique_ptr<MohairOp>* MohairOp::GetOpInputs() { return nullptr; }
  void MohairOp::SimplifyMessage([[maybe_unused]] Rel* rel) { return; }

  // >> ToString implementations for each op type
  // leaf ops
  const string OpErr::ToString() { return u8"Err()"; }

  const string OpRead::ToString()          { return u8"Read("             + table_name + u8")"; }
  const string OpSkyRead::ToString()       { return u8"SkyRead("          + table_name + u8")"; }
  const string OpPartitionRead::ToString() { return u8"SkyPartitionRead(" + table_name + u8")"; }
  const string OpSliceRead::ToString()     { return u8"SkySliceRead("     + table_name + u8")"; }

  // streaming ops
  const string OpProj::ToString()  { return u8"Π()";   }
  const string OpSel::ToString()   { return u8"σ()";   }
  const string OpLimit::ToString() { return u8"Lim()"; }

  // sink ops
  const string OpSort::ToString()      { return u8"Sort()"; }
  const string OpAggr::ToString()      { return u8"Aggr()"; }

  const string OpCrossJoin::ToString() { return u8"×()"; }
  const string OpJoin::ToString()      { return u8"⋈()"; }
  const string OpHashJoin::ToString()  { return u8"⋈→()"; }
  const string OpMergeJoin::ToString() { return u8"⋈⊕()"; }

  // >> Accessor methods for the number inputs into an operator
  size_t OpProj::GetOpArity()  { return 1; }
  size_t OpSel::GetOpArity()   { return 1; }
  size_t OpLimit::GetOpArity() { return 1; }

  size_t OpSort::GetOpArity()  { return 1; }
  size_t OpAggr::GetOpArity()  { return 1; }

  size_t OpJoin::GetOpArity()      { return 2; }
  size_t OpCrossJoin::GetOpArity() { return 2; }
  size_t OpHashJoin::GetOpArity()  { return 2; }
  size_t OpMergeJoin::GetOpArity() { return 2; }

  // >> Accessor methods for an operator's children (input operators)
  unique_ptr<MohairOp>* OpProj::GetOpInputs()  { return op_inputs.data(); }
  unique_ptr<MohairOp>* OpSel::GetOpInputs()   { return op_inputs.data(); }
  unique_ptr<MohairOp>* OpLimit::GetOpInputs() { return op_inputs.data(); }

  unique_ptr<MohairOp>* OpSort::GetOpInputs()  { return op_inputs.data(); }
  unique_ptr<MohairOp>* OpAggr::GetOpInputs()  { return op_inputs.data(); }

  unique_ptr<MohairOp>* OpJoin::GetOpInputs()      { return op_inputs.data(); }
  unique_ptr<MohairOp>* OpCrossJoin::GetOpInputs() { return op_inputs.data(); }
  unique_ptr<MohairOp>* OpHashJoin::GetOpInputs()  { return op_inputs.data(); }
  unique_ptr<MohairOp>* OpMergeJoin::GetOpInputs() { return op_inputs.data(); }

  // >> Methods to "simplify" a Rel message (clear inputs based on RelType)
  void OpProj::SimplifyMessage(Rel* rel)  { rel->mutable_project()->clear_input();   }
  void OpSel::SimplifyMessage(Rel* rel)   { rel->mutable_filter()->clear_input();    }
  void OpLimit::SimplifyMessage(Rel* rel) { rel->mutable_fetch()->clear_input();     }

  void OpSort::SimplifyMessage(Rel* rel)  { rel->mutable_sort()->clear_input();      }
  void OpAggr::SimplifyMessage(Rel* rel)  { rel->mutable_aggregate()->clear_input(); }

  void OpJoin::SimplifyMessage(Rel* rel) {
    rel->mutable_join()->clear_left();
    rel->mutable_join()->clear_right();
  }

  void OpCrossJoin::SimplifyMessage(Rel* rel) {
    rel->mutable_cross()->clear_left();
    rel->mutable_cross()->clear_right();
  }

  void OpHashJoin::SimplifyMessage(Rel* rel) {
    rel->mutable_hash_join()->clear_left();
    rel->mutable_hash_join()->clear_right();
  }

  void OpMergeJoin::SimplifyMessage(Rel* rel) {
    rel->mutable_merge_join()->clear_left();
    rel->mutable_merge_join()->clear_right();
  }

} // namespace: mohair


// ------------------------------
// Translation functions (Substrait <-> Mohair)

namespace mohair {

  // >> Translation to Substrait (Mohair -> Substrait)
  unique_ptr<SuperPlan> SuperPlanFrom(MohairOp* mohair_op) {
    // copy the Rel message so we can modify it
    auto simplified_rel = std::make_unique<Rel>(*(mohair_op->substrait_rel));

    // simplify the message by clearing its inputs (trim the tree)
    mohair_op->SimplifyMessage(simplified_rel.get());

    // construct the SuperPlan message and return it
    auto superplan_msg = std::make_unique<SuperPlan>();
    superplan_msg->set_allocated_merge_rel(simplified_rel.release());

    return superplan_msg;
  }


  // >> Translation to Mohair (Substrait -> Mohair)

  // Function prototypes for helper functions (they are implemented at the bottom)
  string SourceNameFromReadRel(ReadRel *rel_op);

  string SourceNameFromExtensionLeaf(SkyRel*          extrel_op);
  string SourceNameFromExtensionLeaf(SkyPartitionRel* extrel_op);
  string SourceNameFromExtensionLeaf(SkySliceRel*     extrel_op);

  //! Templated translation function for unary relational operators.
  //  `UnaryRelMsg` is the specific substrait message,
  //  `MohairRel`   is the equivalent query operator in mohair 
  template <typename UnaryRelMsg, typename MohairRel>
  unique_ptr<MohairOp> FromUnaryOpMsg(Rel *rel_msg, UnaryRelMsg *rel_op) {
    // recurse on input relation
    unique_ptr<MohairOp> op_input = MohairFrom(rel_op->mutable_input());

    return std::make_unique<MohairRel>(rel_op, rel_msg, std::move(op_input));
  }

  //! Templated translation function for binary relational operators.
  //  `BinaryRelMsg` is the specific substrait message,
  //  `MohairRel`    is the equivalent query operator in mohair 
  template <typename BinaryRelMsg, typename MohairRel>
  unique_ptr<MohairOp> FromBinaryOpMsg(Rel *rel_msg, BinaryRelMsg *rel_op) {
    // recurse on input relations
    unique_ptr<MohairOp> left_input  = MohairFrom(rel_op->mutable_left() );
    unique_ptr<MohairOp> right_input = MohairFrom(rel_op->mutable_right());

    return std::make_unique<MohairRel>(
      rel_op, rel_msg, std::move(left_input), std::move(right_input)
    );
  }

  //! Templated translation function for leaf relational operators (no inputs).
  //  `SourceRelMsg` is the specific substrait message,
  //  `MohairRel`    is the equivalent query operator in mohair 
  template <typename SourceRelMsg, typename MohairRel>
  unique_ptr<MohairOp> FromSourceOpMsg(Rel *rel_msg, SourceRelMsg *rel_op, string tname) {
    return std::make_unique<MohairRel>(rel_op, rel_msg, tname);
  }

  //! Templated translation function for leaf relational operators (no inputs).
  //  `SourceRelMsg` is the specific substrait message,
  //  `MohairRel`    is the equivalent query operator in mohair 
  template <typename ExtensionMsgType, typename MohairRel>
  unique_ptr<MohairOp> FromExtensionLeafMsg(Rel *rel_msg, ExtensionLeafRel *rel_op) {
    auto extrel_op = std::make_unique<ExtensionMsgType>();
    rel_op->detail().UnpackTo(extrel_op.get());

    string src_name = SourceNameFromExtensionLeaf(extrel_op.get());

    return std::make_unique<MohairRel>(rel_op, rel_msg, std::move(extrel_op), src_name);
  }

  //! "Internalizes" a substrait `Rel` message by wrapping it in an appropriate `MohairOp`
  unique_ptr<MohairOp> MohairFrom(Rel *rel_msg) {
    switch(rel_msg->rel_type_case()) {
      // Unary streaming operators
      case Rel::RelTypeCase::kProject: {
        return FromUnaryOpMsg<ProjectRel, OpProj>(rel_msg, rel_msg->mutable_project());
      }

      case Rel::RelTypeCase::kFilter: {
        return FromUnaryOpMsg<FilterRel, OpSel>(rel_msg, rel_msg->mutable_filter());
      }

      case Rel::RelTypeCase::kFetch: {
        return FromUnaryOpMsg<FetchRel, OpLimit>(rel_msg, rel_msg->mutable_fetch());
      }

      // Unary sink operators
      case Rel::RelTypeCase::kSort: {
        return FromUnaryOpMsg<SortRel, OpSort>(rel_msg, rel_msg->mutable_sort());
      }

      case Rel::RelTypeCase::kAggregate: {
        return FromUnaryOpMsg<AggregateRel, OpAggr>(rel_msg, rel_msg->mutable_aggregate());
      }

      // Binary sink operators
      case Rel::RelTypeCase::kJoin: {
        return FromBinaryOpMsg<JoinRel, OpJoin>(rel_msg, rel_msg->mutable_join());
      }

      case Rel::RelTypeCase::kCross: {
        return FromBinaryOpMsg<CrossRel, OpCrossJoin>(rel_msg, rel_msg->mutable_cross());
      }

      case Rel::RelTypeCase::kHashJoin: {
        return FromBinaryOpMsg<HashJoinRel, OpHashJoin>(rel_msg, rel_msg->mutable_hash_join());
      }

      case Rel::RelTypeCase::kMergeJoin: {
        return FromBinaryOpMsg<MergeJoinRel, OpMergeJoin>(rel_msg, rel_msg->mutable_merge_join());
      }

      // Leaf operators
      case Rel::RelTypeCase::kRead: {
        ReadRel* rel_op = rel_msg->mutable_read();

        return FromSourceOpMsg<ReadRel, OpRead>(rel_msg, rel_op, SourceNameFromReadRel(rel_op));
      }

      case Rel::RelTypeCase::kExtensionLeaf: {
        ExtensionLeafRel* extleaf_rel = rel_msg->mutable_extension_leaf();

        // ExtensionLeafRel must have `detail` attribute populated with a message
        if (not extleaf_rel->has_detail()) {
          return std::make_unique<OpErr>(rel_msg, "ExtensionLeafRel is missing data");
        }

        // Check for known extension types
        if (extleaf_rel->detail().Is<SkyRel>()) {
          return FromExtensionLeafMsg<SkyRel, OpSkyRead>(rel_msg, extleaf_rel);
        }

        else if (extleaf_rel->detail().Is<SkyPartitionRel>()) {
          return FromExtensionLeafMsg<SkyPartitionRel, OpPartitionRead>(rel_msg, extleaf_rel);
        }

        else if (extleaf_rel->detail().Is<SkySliceRel>()) {
          return FromExtensionLeafMsg<SkySliceRel, OpSliceRead>(rel_msg, extleaf_rel);
        }

        // Otherwise, fail
        return std::make_unique<OpErr>(rel_msg, "Unknown message type in ExtensionLeafRel");
      }

      // Catch all error
      default: {
        return std::make_unique<OpErr>(rel_msg, "ParseError: operator not yet supported");
      }
    }
  }


  // >> Implementations for helper functions
  // NOTE: we put these down here just to make it easier to look at translation logic
  //       separately from extraction of individual attributes

  //! Given a substrait operator, `ReadRel`, extracts a "table name".
  //  This is useful for "tagging" a pipeline with what sources it needs to access, so we
  //  can quickly identify candidate pipelines when splitting queries.
  string SourceNameFromReadRel(ReadRel *rel_op) {
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

  //! Overloaded function that extracts a "table name" from the given extension operator.
  string SourceNameFromExtensionLeaf(SkyRel *extrel_op) {
    return string { extrel_op->domain() + "-" + extrel_op->partition() };
  }

  //! Overloaded function that extracts a "table name" from the given extension operator.
  string SourceNameFromExtensionLeaf(SkyPartitionRel *extrel_op) {
    return string { extrel_op->domain() + "-" + extrel_op->partition() };
  }

  //! Overloaded function that extracts a "table name" from the given extension operator.
  string SourceNameFromExtensionLeaf(SkySliceRel *extrel_op) {
    return string { extrel_op->domain() + "-" + extrel_op->partition() };
  }

} // namespace: mohair

