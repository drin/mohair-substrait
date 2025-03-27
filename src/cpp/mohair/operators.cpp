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

  unique_ptr<Rel> CopyRel(Rel* src_rel) {
    unique_ptr<Rel> rel_copy { std::make_unique<Rel>() };
    rel_copy->CopyFrom(*src_rel);

    return rel_copy;
  }


  // >> Reusable function kernels (not templated)

  //! Replace a ReferenceRel with its anchor Rel.
  // NOTE: mergerel should still point to both parts that need to be rejoined
  void InlineRefRel(SubstraitPlan* plan, SubstraitOp* mergerel) {
    Rel*          ref_rel    = mergerel->substrait_rel;
    ReferenceRel* ref_op     = ref_rel->release_reference();
    int32_t       refrel_pos = ref_op->subtree_ordinal();
    uint32_t      anchor_id  = ref_op->subtree_reference();

    auto     plan_relations = plan->payload->mutable_relations();
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
  string SourceNameFromReadRel(ReadRel *srel_op) {
    switch (srel_op->read_type_case()) {
      case ReadRel::ReadTypeCase::kNamedTable: {
        auto& catalog_name = srel_op->named_table();

        // Grab the parts of the table name, but ibis only encodes 1.
        std::stringstream tname_stream;
        tname_stream << catalog_name.names(0);
        for (int tname_ndx = 1; tname_ndx < catalog_name.names_size(); ++tname_ndx) {
          tname_stream << "." << catalog_name.names(tname_ndx);
        }

        return tname_stream.str();
      }

      case ReadRel::ReadTypeCase::kLocalFiles: {
        auto& src_files = srel_op->local_files();

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

} // namespace: mohair


// ------------------------------
// Translation functions (Substrait -> Mohair)

namespace mohair {

  // >> Forward declarations for translation functions
  template <typename UnaryRelType>
  unique_ptr<SubstraitOp>  FromUnaryOpMsg(Plan* plan, Rel* srel, UnaryRelType*  srel_op);

  template <typename BinaryRelType>
  unique_ptr<SubstraitOp> FromBinaryOpMsg(Plan* plan, Rel* srel, BinaryRelType* srel_op);
  unique_ptr<SubstraitOp> FromReferenceOp(Plan* plan, Rel* srel, ReferenceRel*  srel_op);

  unique_ptr<SubstraitOp>           FromReadOp(Rel* srel, ReadRel*          srel_op);
  unique_ptr<SubstraitOp> FromExtensionLeafMsg(Rel* srel, ExtensionLeafRel* srel_op);

  // >> Translation logic
  //! Wraps substrait `Rel` messages in `SubstraitOp` instances
  unique_ptr<SubstraitOp> ParsePlanOps(SubstraitPlan* plan, Rel* rel) {
    switch(rel->rel_type_case()) {
      // Unary streaming operators
      case Rel::RelTypeCase::kProject: {
        return FromUnaryOpMsg<ProjectRel>(plan, rel, rel->mutable_project());
      }

      case Rel::RelTypeCase::kFilter: {
        return FromUnaryOpMsg<FilterRel>(plan, rel, rel->mutable_filter());
      }

      case Rel::RelTypeCase::kFetch: {
        return FromUnaryOpMsg<FetchRel>(plan, rel, rel->mutable_fetch());
      }

      // Unary sink operators
      case Rel::RelTypeCase::kSort: {
        return FromUnaryOpMsg<SortRel>(plan, rel, rel->mutable_sort());
      }

      case Rel::RelTypeCase::kAggregate: {
        return FromUnaryOpMsg<AggregateRel>(plan, rel, rel->mutable_aggregate());
      }

      // Binary sink operators
      case Rel::RelTypeCase::kJoin: {
        return FromBinaryOpMsg<JoinRel>(plan, rel, rel->mutable_join());
      }

      case Rel::RelTypeCase::kCross: {
        return FromBinaryOpMsg<CrossRel>(plan, rel, rel->mutable_cross());
      }

      case Rel::RelTypeCase::kHashJoin: {
        return FromBinaryOpMsg<HashJoinRel>(plan, rel, rel->mutable_hash_join());
      }

      case Rel::RelTypeCase::kMergeJoin: {
        return FromBinaryOpMsg<MergeJoinRel>(plan, rel, rel->mutable_merge_join());
      }

      // Leaf operators
      case Rel::RelTypeCase::kReference: {
        // Propagating the plan allows us to recurse into the referenced subplan
        return FromReferenceOp(plan, rel, rel->mutable_reference());
      }

      case Rel::RelTypeCase::kRead: {
        return FromReadOp(rel, rel->mutable_read());
      }

      case Rel::RelTypeCase::kExtensionLeaf: {
        return FromExtensionLeafMsg(rel, rel->mutable_extension_leaf());
      }

      default: break;
    }

    // Catch all error
    std::cerr << "Parsing for operator not yet implemented: "
              << rel->rel_type_case()
              << std::endl
    ;
    return nullptr;
  }

  //! Wrap the unary Rel message, `UnaryRelType`, in a SubstraitOpImpl
  template <typename UnaryRelType>
  unique_ptr<SubstraitOp> FromUnaryOpMsg(Plan* plan, Rel* srel, UnaryRelType* srel_op) {
    // Recurse on input relation
    unique_ptr<SubstraitOp> input_op = ParsePlanOps(plan, srel_op->mutable_input());

    // Construct current operator
    auto unary_op = std::make_unique<SubstraitOpImpl<UnaryRelType>>(srel, srel_op);
    unary_op->input_ops[0] = std::move(input_op);
    unary_op->BindLogicalSchema();

    return unary_op;
  }

  //! Wrap the binary Rel message, `BinaryRelType`, in a SubstraitOpImpl
  template <typename BinaryRelType>
  unique_ptr<SubstraitOp> FromBinaryOpMsg(Plan* plan, Rel* srel, BinaryRelType* srel_op) {
    // recurse on input relations
    unique_ptr<SubstraitOp> left_input  = ParsePlanOps(plan, srel_op->mutable_left());
    unique_ptr<SubstraitOp> right_input = ParsePlanOps(plan, srel_op->mutable_right());

    // Construct current operator
    auto binary_op = std::make_unique<SubstraitOpImpl<BinaryRelType>>(srel, srel_op);
    binary_op->input_ops[0] = std::move(left_input);
    binary_op->input_ops[1] = std::move(right_input);
    binary_op->BindLogicalSchema();

    return binary_op;
  }

  unique_ptr<SubstraitOp> FromReferenceOp(Plan* plan, Rel* srel, ReferenceRel* srel_op) {
    // Find the subplan's root Rel
    PlanRel* subplan_rel    { nullptr };
    int      subplan_relndx { 0       };
    for (; subplan_relndx < plan->relations_size(); ++subplan_relndx) {
      subplan_rel = plan->mutable_relations(subplan_relndx);
      if (subplan_rel->subtree_anchor() == srel_op->subtree_reference()) { break; }
    }

    // Error if we didn't find the PlanRel or if it is the root PlanRel
    if (subplan_relndx >= plan->relations_size()) {
      std::cerr << "Could not find referenced PlanRel" << std::endl;
      return nullptr;
    }

    else if (subplan_rel->has_root()) {
      std::cerr << "Can't have reference to plan root relation" << std::endl;
      return nullptr;
    }

    // Recurse into the found PlanRel
    unique_ptr<SubstraitOp> subplan_root = ParsePlanOps(plan, subplan_rel->mutable_rel());
    auto subplan_schema = std::make_unique<SubstraitSchema>();
    subplan_schema->CopyFrom(*(subplan_root->schema));

    auto ref_op = std::make_unique<SubstraitOpImpl<ReferenceRel>>(srel, srel_op);
    ref_op->subplan_root = std::move(subplan_root);
    ref_op->schema       = std::move(subplan_schema);

    return ref_op;
  }

  unique_ptr<SubstraitOp> FromReadOp(Rel* srel, ReadRel* srel_op) {
    auto read_schema = std::make_unique<SubstraitSchema>();
    read_schema->CopyFrom(srel_op->base_schema());

    auto read_op = std::make_unique<SubstraitOpImpl<ReadRel>>(srel, srel_op);
    read_op->schema      = SchemaFromRel(*srel_op, std::move(read_schema));
    read_op->source_name = SourceNameFromReadRel(srel_op);

    return read_op;
  }

  // Forward declarations for translating custom operators (extension ops)
  template <typename SkyRelType>
  unique_ptr<SubstraitOp> FromCustomRelMsg(Rel* srel, ExtensionLeafRel* srel_op);
  unique_ptr<SubstraitOp>    FromResultMsg(Rel* srel, ExtensionLeafRel* srel_op);

  //! Templated translation function for leaf relational operators (no inputs).
  unique_ptr<SubstraitOp> FromExtensionLeafMsg(Rel *srel, ExtensionLeafRel *srel_op) {
    // ExtensionLeafRel must have `detail` attribute populated with a message
    if (not srel_op->has_detail()) {
      std::cerr << "ExtensionLeafRel has no details" << std::endl;
      return nullptr;
    }

    if (srel_op->detail().Is<SkyRel>()) {
      return FromCustomRelMsg<SkyRel>(srel, srel_op);
    }
    else if (srel_op->detail().Is<SkyPartitionRel>()) {
      return FromCustomRelMsg<SkyPartitionRel>(srel, srel_op);
    }
    else if (srel_op->detail().Is<SkySliceRel>()) {
      return FromCustomRelMsg<SkySliceRel>(srel, srel_op);
    }
    else if (srel_op->detail().Is<SkyResultRel>()) {
      return FromResultMsg(srel, srel_op);
    }

    std::cerr << "Unknown message type in ExtensionLeafRel" << std::endl;
    return nullptr;
  }


  // >> Helper functions
  //! Construct a source name for a custom read operator
  template <typename SkyRelType>
  unique_ptr<SubstraitOp>
  FromCustomRelMsg(Rel* srel, ExtensionLeafRel* srel_op) {
    // Unpack the contained custom operator
    auto extrel_op = std::make_unique<SkyRelType>();
    srel_op->detail().UnpackTo(extrel_op.get());

    // Make a modifiable copy of the schema
    auto leaf_schema = std::make_unique<SubstraitSchema>();
    leaf_schema->CopyFrom(extrel_op->schema());

    auto custom_op = std::make_unique<SubstraitOpImpl<SkyRelType>>(srel, srel_op);
    custom_op->schema      = std::move(leaf_schema);
    custom_op->source_name = string {
      extrel_op->domain() + "-" + extrel_op->partition()
    };
  }

  //! Construct a source name for a materialized result
  unique_ptr<SubstraitOp>
  FromResultMsg(Rel* srel, ExtensionLeafRel* srel_op) {
    // Unpack the contained custom operator
    auto result_rel = std::make_unique<SkyResultRel>();
    srel_op->detail().UnpackTo(result_rel.get());

    // Make a modifiable copy of the schema
    auto result_schema = std::make_unique<SubstraitSchema>();
    result_schema->CopyFrom(result_rel->schema());

    auto result_op = std::make_unique<SubstraitOpImpl<SkyRelType>>(srel, srel_op);
    result_op->schema      = std::move(result_schema);
    result_op->source_name = string { result_op->result_name() };

    return result_op;
  }

} // namespace: mohair


// ------------------------------
// Translation functions (Mohair -> Substrait)

namespace mohair {

  // >> Translation to Substrait (Mohair -> Substrait)
  unique_ptr<SuperPlan> SuperPlanFrom(SubstraitOp* rel_op) {
    unique_ptr<Rel> rel_copy { rel_op->CopyRelOp() };

    unique_ptr<SuperPlan> superplan_msg { std::make_unique<SuperPlan>() };
    superplan_msg->set_allocated_merge_rel(rel_copy.release());

    return superplan_msg;
  }

} // namespace: mohair


// ------------------------------
// Implementations for member functions

namespace mohair {

  // >> Implementations for templated class SubstraitOpImpl<RelType>
  template <typename RelType>
  optional<bool>
  SubstraitOpImpl<RelType>::IsSink() const { return is_sink<RelType>::value; }

  template <typename RelType>
  optional<bool>
  SubstraitOpImpl<RelType>::IsSource() const { return is_source<RelType>::value; }

  template <typename RelType>
  optional<bool>
  SubstraitOpImpl<RelType>::IsOrigin() const { return is_origin<RelType>::value; }

  template <typename RelType>
  optional<bool>
  SubstraitOpImpl<RelType>::IsStream() const { return is_stream<RelType>::value; }

  template <typename RelType>
  optional<size_t> SubstraitOpImpl<RelType>::GetArity() const { return OpTraits::Arity; }

  template <typename RelType>
  string_view SubstraitOpImpl<RelType>::Stringify() const { return OpTraits::OpSymbol; }

  template <typename RelType>
  unique_ptr<Rel> SubstraitOpImpl<RelType>::CopyRelOp() const {
    unique_ptr<Rel> rel_copy { std::make_unique<Rel>() };
    rel_copy->CopyFrom(*rel);

    return rel_copy;
  }

  template <typename RelType>
  MessageDifferencer* SubstraitOpImpl<RelType>::GetComparator() const {
    static unique_ptr<MessageDifferencer> msg_differ { nullptr };

    if (msg_differ == nullptr) {
      if constexpr (OpTraits::Arity == 2) {
        msg_differ = BinaryRelComparator<RelType>();
      }

      else if (OpTraits::Arity == 1) {
        msg_differ = UnaryRelComparator<RelType>();
      }

      else {
        msg_differ = std::make_unique<MessageDifferencer>();
      }
    }

    return msg_differ.get();
  }

  template <typename RelType>
  bool SubstraitOpImpl<RelType>::SetAliases(const RepeatedPtrField<string>& aliases) {
    if constexpr (not OpTraits::HasCommon) { return false; }
    else {
      RelCommon* common = rel_op->mutable_common();
      common->mutable_hint()->mutable_output_names()->CopyFrom(aliases);

      return true;
    }
  }

  //! A visitor function to build plan pipelines with this operator as the final sink
  template <typename RelType>
  void BuildPipelines(PlanPipeline& plan_pipe, PipelineStage& final_stage) {
    // Create a pipeline for every input into the sink
    for (size_t child_ndx = 0; child_ndx < OpTraits::Arity; ++child_ndx) {
      OpPipeline& stage_pipe = final_stage.CreatePipeline(nullptr);

      // Recurse through the input operator
      input_ops[child_ndx]->AddToPipeline(plan_pipe, final_stage, stage_pipe);
    }
  }

  template <typename RelType>
  bool SubstraitOpImpl<RelType>::BindLogicalSchema() {
    // TODO: maybe rename `output_schema` to be `logical_schema`
    // The logical schema is cached in the plan, so check there first
    if (rel_op->has_common() and rel_op->common().hint().has_output_schema()) {
      schema = std::make_unique<SubstraitSchema>();
      schema->CopyFrom(rel_op->common().hint().output_schema());
      return true;
    }

    // Otherwise, binding depends on the logical schemas of our inputs
    auto input_schema = std::make_unique<SubstraitSchema>();
    input_schema->CopyFrom(*(input_ops[0]->schema));

    // If we have a second input (e.g. for a Join), we extend our input schema
    if constexpr (OpTraits::Arity == 2) {
      const SubstraitSchema& right_schema = *(input_ops[1]->schema);

      int rndx { 0 };
      for (const SubstraitType& r_type : right_schema.struct_().types()) {
        SubstraitType* join_rtype = input_schema->mutable_struct_()->add_types();
        join_rtype->CopyFrom(r_type);

        if (rndx < right_schema.names_size()) {
          input_schema->add_names(right_schema.names(rndx++));
        }
      }
    }

    // Set our in-memory schema
    schema = SchemaFromRel(*rel_op, std::move(input_schema));

    // Then update the logical schema in the plan
    rel_op->mutable_common()
          ->mutable_hint()
          ->mutable_output_schema()
          ->CopyFrom(*schema);

    return true;
  }


  // >> Implementations for SubstraitPlan (need visibility of this translation unit)
  //! Parses the contained substrait plan, `plan`, into a tree of `SubstraitOp`.
  void SubstraitPlan::ParseOperators() {
    RelRoot* plan_rootrel = plan->mutable_relations(root_relndx)->mutable_root();
    Rel*     root_rel     = plan_rootrel->mutable_input();

    // Parse the plan operators then move result aliases to make the plan split-friendly
    plan_root = ParsePlanOps(this, root_rel);
    PushdownResultAliases();
  }

} // namespace: mohair

