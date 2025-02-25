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
// Functions

namespace mohair {

  // >> Forward declarations
  string SourceNameFromExtLeaf(SkyRel*          extrel_op);
  string SourceNameFromExtLeaf(SkyPartitionRel* extrel_op);
  string SourceNameFromExtLeaf(SkySliceRel*     extrel_op);
  string SourceNameFromExtLeaf(SkyResultRel*    extrel_op);

  // >> Reusable function kernels (templated)
  //! Templated translation function for unary relational operators.
  //  `UnaryRelMsg` is the specific substrait message,
  //  `MohairRel`   is the equivalent query operator in mohair 
  template <typename UnaryRelMsg, typename MohairRel>
  unique_ptr<MohairOp>
  FromUnaryOpMsg(Plan* plan_msg, Rel* rel_msg, UnaryRelMsg* rel_op) {
    // Recurse on input relation
    unique_ptr<MohairOp> input_op = MohairFrom(plan_msg, rel_op->mutable_input());

    // Initialize a new schema from the input schema
    unique_ptr<SubstraitSchema> output_schema;
    if (rel_op->has_common() and rel_op->common().hint().has_output_schema()) {
      output_schema = std::make_unique<SubstraitSchema>();
      output_schema->CopyFrom(rel_op->common().hint().output_schema());
    }
    else {
      auto input_schema = std::make_unique<SubstraitSchema>();
      input_schema->CopyFrom(*(input_op->schema));

      output_schema = SchemaFromRel(*rel_op, std::move(input_schema));
      rel_op->mutable_common()
            ->mutable_hint()
            ->mutable_output_schema()
            ->CopyFrom(*output_schema);
    }

    return std::make_unique<MohairRel>(
       rel_op
      ,rel_msg
      ,std::move(input_op)
      ,std::move(output_schema)
    );
  }

  //! Templated translation function for binary relational operators.
  //  `BinaryRelMsg` is the specific substrait message,
  //  `MohairRel`    is the equivalent query operator in mohair 
  template <typename BinaryRelMsg, typename MohairRel>
  unique_ptr<MohairOp>
  FromBinaryOpMsg(Plan* plan_msg, Rel* rel_msg, BinaryRelMsg* rel_op) {
    // recurse on input relations
    unique_ptr<MohairOp> left_input  = MohairFrom(plan_msg, rel_op->mutable_left());
    unique_ptr<MohairOp> right_input = MohairFrom(plan_msg, rel_op->mutable_right());

    // Initialize input schema from left input
    auto input_schema = std::make_unique<SubstraitSchema>();
    input_schema->CopyFrom(*(left_input->schema));

    // Merge schema from right input
    int rndx { 0 };
    for (const SubstraitType& r_type : right_input->schema->struct_().types()) {
      SubstraitType* join_rtype = input_schema->mutable_struct_()->add_types();

      join_rtype->CopyFrom(r_type);
      if (rndx < right_input->schema->names_size()) {
        input_schema->add_names(right_input->schema->names(rndx++));
      }
    }

    unique_ptr<SubstraitSchema> output_schema {
      SchemaFromRel(*rel_op, std::move(input_schema))
    };

    rel_op->mutable_common()
          ->mutable_hint()
          ->mutable_output_schema()
          ->CopyFrom(*output_schema);

    return std::make_unique<MohairRel>(
       rel_op
      ,rel_msg
      ,std::move(left_input)
      ,std::move(right_input)
      ,std::move(output_schema)
    );
  }

  //! Templated translation function for leaf relational operators (no inputs).
  //  `SourceRelMsg` is the specific substrait message,
  //  `MohairRel`    is the equivalent query operator in mohair 
  template <typename ExtensionMsgType, typename MohairRel>
  unique_ptr<MohairOp>
  FromExtensionLeafMsg(Rel *rel_msg, ExtensionLeafRel *rel_op) {
    auto extrel_op = std::make_unique<ExtensionMsgType>();
    rel_op->detail().UnpackTo(extrel_op.get());

    // Each skytether read operator has a `schema` field
    string src_name    = SourceNameFromExtLeaf(extrel_op.get());
    auto   read_schema = std::make_unique<SubstraitSchema>();
    read_schema->CopyFrom(extrel_op->schema());

    return std::make_unique<MohairRel>(
       rel_op
      ,rel_msg
      ,std::move(extrel_op)
      ,src_name
      ,std::move(read_schema)
    );
  }

  //! Templated helper function for moving a `Rel` to be a reference rel
  template <typename RelType>
  ReferenceRel* MoveToRefRel(int32_t refrel_pos, PlanRel* plan_refrel, RelType* rel_op) {
    // UUIDGenerator is static
    uint32_t anchor_id = ++UUIDGenerator;

    // Move the ProjectRel into the PlanRel
    unique_ptr<Rel> anchor_rel { std::make_unique<Rel>() };
    anchor_rel->set_allocated_project(rel_op);

    plan_refrel->set_subtree_anchor(anchor_id);
    plan_refrel->set_allocated_rel(anchor_rel.release());

    // Replace the input to `substrait_rel` with a `ReferenceRel`
    unique_ptr<ReferenceRel> ref_rel { std::make_unique<ReferenceRel>() };
    ref_rel->set_subtree_ordinal(refrel_pos);
    ref_rel->set_subtree_reference(anchor_id);

    return ref_rel.release();
  }


  // >> Reusable function kernels (not templated)
  //! Replace a ReferenceRel with its anchor Rel.
  // NOTE: mergerel should still point to both parts that need to be rejoined
  void InlineRefRel(PlanMessage* plan_msg, MohairOp* mergerel) {
    Rel*          ref_rel    = mergerel->substrait_rel;
    ReferenceRel* ref_op     = ref_rel->release_reference();
    int32_t       refrel_pos = ref_op->subtree_ordinal();
    uint32_t      anchor_id  = ref_op->subtree_reference();

    auto     plan_relations = plan_msg->payload->mutable_relations();
    PlanRel* anchor_planrel = plan_relations->Mutable(refrel_pos);
    Rel*     anchor_rel     = anchor_planrel->mutable_rel();
    if (anchor_id != anchor_planrel->subtree_anchor()) {
      std::cerr << "Unable to inline anchor Rel; mismatching anchor ID" << std::endl;
      return;
    }

    switch (ref_rel->rel_type_case()) {
      // unary operators
      case Rel::RelTypeCase::kProject: {
        ref_rel->set_allocated_project(anchor_rel->release_project());
        break;
      }

      case Rel::RelTypeCase::kFilter: {
        ref_rel->set_allocated_filter(anchor_rel->release_filter());
        break;
      }

      case Rel::RelTypeCase::kFetch: {
        ref_rel->set_allocated_fetch(anchor_rel->release_fetch());
        break;
      }

      case Rel::RelTypeCase::kSort: {
        ref_rel->set_allocated_sort(anchor_rel->release_sort());
        break;
      }

      case Rel::RelTypeCase::kAggregate: {
        ref_rel->set_allocated_aggregate(anchor_rel->release_aggregate());
        break;
      }

      // binary operators
      case Rel::RelTypeCase::kJoin: {
        ref_rel->set_allocated_join(anchor_rel->release_join());
        break;
      }

      case Rel::RelTypeCase::kCross: {
        ref_rel->set_allocated_cross(anchor_rel->release_cross());
        break;
      }

      case Rel::RelTypeCase::kHashJoin: {
        ref_rel->set_allocated_hash_join(anchor_rel->release_hash_join());
        break;
      }

      case Rel::RelTypeCase::kMergeJoin: {
        ref_rel->set_allocated_merge_join(anchor_rel->release_merge_join());
        break;
      }

      // Leaf operators (no-op)
      case Rel::RelTypeCase::kReference:
      case Rel::RelTypeCase::kRead:
      case Rel::RelTypeCase::kExtensionLeaf: {
        throw std::logic_error("Unexpected leaf operator as anchor Rel");
      }

      // Unimplemented operators
      default: {
        throw std::runtime_error("Cannot inline anchor rel: unimplemented type");
      }
    }

    plan_relations->erase(plan_relations->begin() + refrel_pos);
  }

  //! Extracts the source name from a `ReadRel` operator (e.g. table name)
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

  //! Extracts the source name from a custom operator used in `ExtensionLeaf`
  string SourceNameFromExtLeaf(SkyRel* extrel_op) {
    return string { extrel_op->domain() + "-" + extrel_op->partition() };
  }

  //! Extracts the source name from a custom operator used in `ExtensionLeaf`
  string SourceNameFromExtLeaf(SkyPartitionRel* extrel_op) {
    return string { extrel_op->domain() + "-" + extrel_op->partition() };
  }

  //! Extracts the source name from a custom operator used in `ExtensionLeaf`
  string SourceNameFromExtLeaf(SkySliceRel* extrel_op) {
    return string { extrel_op->domain() + "-" + extrel_op->partition() };
  }

  //! Extracts the source name from a custom operator used in `ExtensionLeaf`
  string SourceNameFromExtLeaf(SkyResultRel* extrel_op) {
    return string { extrel_op->result_name() };
  }


} // namespace: mohair


// ------------------------------
// Operator class implementations

namespace mohair {

  // >> Implementations for the base class
  const string MohairOp::ToString() { return "MohairOp";       }
  const string MohairOp::ViewStr()  { return this->ToString(); }
  const string MohairOp::GetName()  { return ""; }

  bool   MohairOp::IsSink()     { return false; }
  bool   MohairOp::IsOrigin()   { return false; }
  size_t MohairOp::GetOpArity() { return 0;     }

  unique_ptr<MohairOp>* MohairOp::GetOpInputs() { return nullptr; }

  unique_ptr<Rel> MohairOp::CopySubstraitRel() {
    return CopyRel(this->substrait_rel);
  }

  // >> ToString implementations for each op type
  // leaf ops
  const string OpErr::ToString()       { return u8"Err()"; }
  const string OpReference::ToString() { return u8"Ref(" + std::to_string(rel_op->subtree_reference()) + ")"; }

  const string OpRead::ToString()          { return u8"Read("             + table_name + u8")"; }
  const string OpSkyRead::ToString()       { return u8"SkyRead("          + table_name + u8")"; }
  const string OpPartitionRead::ToString() { return u8"SkyPartitionRead(" + table_name + u8")"; }
  const string OpSliceRead::ToString()     { return u8"SkySliceRead("     + table_name + u8")"; }
  const string OpViewRead::ToString()      { return u8"SkyResultRead("    + table_name + u8")"; }

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

} // namespace: mohair


// ------------------------------
// Translation functions (Substrait <-> Mohair)

namespace mohair {

  // >> Translation to Substrait (Mohair -> Substrait)

  unique_ptr<SuperPlan> SuperPlanFrom(MohairOp* mohair_op) {
    unique_ptr<Rel>       rel_copy      { mohair_op->CopySubstraitRel() };
    unique_ptr<SuperPlan> superplan_msg { std::make_unique<SuperPlan>() };
    superplan_msg->set_allocated_merge_rel(rel_copy.release());

    return superplan_msg;
  }


  // >> Translation to Mohair (Substrait -> Mohair)
  unique_ptr<MohairOp> MohairFrom(Plan* substrait_plan) {
    auto plan_rels = substrait_plan->mutable_relations();
    auto rel_itr   = plan_rels->begin();
    for (; rel_itr != plan_rels->end() and not rel_itr->has_root(); ++rel_itr) {}

    RelRoot* root     = rel_itr->mutable_root();
    Rel*     root_rel = root->mutable_input();
    if (not root->names().empty()) {
      RelCommon* rel_common = GetRelCommon(root_rel);
      rel_common->mutable_hint()->mutable_output_names()->CopyFrom(root->names());
    }

    return MohairFrom(substrait_plan, root_rel);
  }

  //! Wraps substrait `Rel` messages in `MohairOp` instances
  unique_ptr<MohairOp> MohairFrom(Plan* plan_msg, Rel* rel_msg) {
    switch(rel_msg->rel_type_case()) {
      // Unary streaming operators
      case Rel::RelTypeCase::kProject: {
        return FromUnaryOpMsg<ProjectRel, OpProj>(plan_msg, rel_msg, rel_msg->mutable_project());
      }

      case Rel::RelTypeCase::kFilter: {
        return FromUnaryOpMsg<FilterRel, OpSel>(plan_msg, rel_msg, rel_msg->mutable_filter());
      }

      case Rel::RelTypeCase::kFetch: {
        return FromUnaryOpMsg<FetchRel, OpLimit>(plan_msg, rel_msg, rel_msg->mutable_fetch());
      }

      // Unary sink operators
      case Rel::RelTypeCase::kSort: {
        return FromUnaryOpMsg<SortRel, OpSort>(plan_msg, rel_msg, rel_msg->mutable_sort());
      }

      case Rel::RelTypeCase::kAggregate: {
        return FromUnaryOpMsg<AggregateRel, OpAggr>(plan_msg, rel_msg, rel_msg->mutable_aggregate());
      }

      // Binary sink operators
      case Rel::RelTypeCase::kJoin: {
        return FromBinaryOpMsg<JoinRel, OpJoin>(plan_msg, rel_msg, rel_msg->mutable_join());
      }

      case Rel::RelTypeCase::kCross: {
        return FromBinaryOpMsg<CrossRel, OpCrossJoin>(plan_msg, rel_msg, rel_msg->mutable_cross());
      }

      case Rel::RelTypeCase::kHashJoin: {
        return FromBinaryOpMsg<HashJoinRel, OpHashJoin>(plan_msg, rel_msg, rel_msg->mutable_hash_join());
      }

      case Rel::RelTypeCase::kMergeJoin: {
        return FromBinaryOpMsg<MergeJoinRel, OpMergeJoin>(plan_msg, rel_msg, rel_msg->mutable_merge_join());
      }

      // Leaf operators
      case Rel::RelTypeCase::kReference: {
        ReferenceRel* rel_op = rel_msg->mutable_reference();

        // The whole point of propagating a pointer to the plan is so that
        // we can descend into the subplan this ReferenceRel points to
        unique_ptr<MohairOp> subplan_root;
        for (int plan_relndx = 0; plan_relndx < plan_msg->relations_size(); ++plan_relndx) {
          PlanRel* subplan = plan_msg->mutable_relations(plan_relndx);

          if (subplan->subtree_anchor() == rel_op->subtree_reference()) {
            if (subplan->has_root()) {
              std::cerr << "Can't have reference to plan root relation" << std::endl;
              return nullptr;
            }

            subplan_root = MohairFrom(plan_msg, subplan->mutable_rel());
            break;
          }
        }

        if (subplan_root == nullptr) {
          std::cerr << "Could not find referenced subplan" << std::endl;
          return nullptr;
        }

        auto input_schema = std::make_unique<SubstraitSchema>();
        input_schema->CopyFrom(*(subplan_root->schema));

        return std::make_unique<OpReference>(
           rel_op
          ,rel_msg
          ,std::move(subplan_root)
          ,std::move(input_schema)
        );
      }

      case Rel::RelTypeCase::kRead: {
        ReadRel* rel_op      = rel_msg->mutable_read();
        auto     read_schema = std::make_unique<SubstraitSchema>();
        read_schema->CopyFrom(rel_op->base_schema());

        return std::make_unique<OpRead>(
           rel_op
          ,rel_msg
          ,SourceNameFromReadRel(rel_op)
          ,SchemaFromRel(*rel_op, std::move(read_schema))
        );
      }

      case Rel::RelTypeCase::kExtensionLeaf: {
        ExtensionLeafRel* extleaf_rel = rel_msg->mutable_extension_leaf();

        // ExtensionLeafRel must have `detail` attribute populated with a message
        if (not extleaf_rel->has_detail()) {
          return std::make_unique<OpErr>(
            rel_msg, "ExtensionLeafRel is missing data"
          );
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

        else if (extleaf_rel->detail().Is<SkyResultRel>()) {
          return FromExtensionLeafMsg<SkyResultRel, OpViewRead>(rel_msg, extleaf_rel);
        }

        // TODO: we don't need this yet
        /*
        else if (extleaf_rel->detail().Is<SkyLakeRel>()) {
          return FromExtensionLeafMsg<SkyLakeRel, OpViewRead>(rel_msg, extleaf_rel);
        }
        */

        // Otherwise, fail
        return std::make_unique<OpErr>(
          rel_msg, "Unknown message type in ExtensionLeafRel"
        );
      }

      // Catch all error
      default: {
        return std::make_unique<OpErr>(
          rel_msg, "ParseError: operator not yet supported"
        );
      }
    }
  }

} // namespace: mohair

