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


// >> Internal headers

// Library configuration
#include "mohair-config.hpp"

// Required dependencies
#include "mohair/adapters/adapter_standard.hpp"  // C++ standard library
#include "mohair/adapters/adapter_protobuf.hpp"  // Protobuf framework

#include "mohair/adapters/helpers/helper_prototypes.hpp"

// Mohair extensions (protobuf)
#include "skyproto/mohair/algebra.pb.h"
#include "skyproto/mohair/topology.pb.h"


// ------------------------------
// Aliases

namespace mohair {

  // >> Mohair types
  // Topology level
  using skyproto::mohair::ServiceConfig;
  using skyproto::mohair::DeviceClass;

  // Plan level
  using skyproto::mohair::SuperPlan;
  using skyproto::mohair::SubPlan;

  // Operator level
  using skyproto::mohair::QueryRel;
  using skyproto::mohair::ErrRel;

  using skyproto::mohair::SkyRel;
  using skyproto::mohair::SkyPartitionRel;
  using skyproto::mohair::SkySliceRel;

  using skyproto::mohair::SkyResultRel;
  using skyproto::mohair::SkyLakeRel;

} // namespace: mohair


// ------------------------------
// Variables

namespace mohair {

  // >> Static variables
  static constexpr uint32_t AnchorIDGenerator { 0 };

} // namespace: mohair


// ------------------------------
// Functions

namespace mohair {

  //! Finds the root of the plan and returns it and its index
  std::tuple<PlanRel*, int> FindPlanRoot(Plan& substrait_plan);

} // namespace: mohair


// ------------------------------
// Classes

namespace mohair {

  //! An adapter class for substrait plans
  struct SubstraitPlan {
    unique_ptr<Plan> plan;
    PlanRel*         root_rel;
    int              root_relndx;

    // >> Destructors and constructors
    virtual ~SubstraitPlan() = default;

    SubstraitPlan(unique_ptr<Plan>&& msg, int rel_ndx)
      : plan(std::move(msg)), root_relndx(rel_ndx) {
      this->root_rel = this->plan->mutable_relations(root_relndx);
    }

    // >> Methods
    virtual void Print();
    virtual bool ToString(string* out_str);

    virtual bool SerializeToString(string* out_data);
    virtual bool SerializeToString(string& out_data);

    virtual bool SerializeToFile(const char* out_fpath);
    virtual bool SerializeToFile(const string& out_fpath);

    // >> Static methods
    //! Constructs a SubstraitPlan from existing Plan
    static unique_ptr<SubstraitPlan> FromPlan(unique_ptr<Plan>&& plan);

    //! Constructs a SubstraitPlan from a serialized Plan
    static unique_ptr<SubstraitPlan> FromMsg(const string& plan_str);

    //! Constructs a SubstraitPlan from a file containing a serialized Plan
    static unique_ptr<SubstraitPlan> FromFile(const char* plan_fpath);

    //! Constructs a SubstraitPlan from a file containing a serialized Plan
    static unique_ptr<SubstraitPlan> FromFile(const string& plan_fpath);

  };

  //! Forward type for a container holding pointers to an operator and its inputs
  struct OpTreeItr;


  //! Adapter for substrait operators for (hopefully) performant access
  template <typename RelType>
  struct RelOp : SubstraitOp {
    Rel*                rel;
    RelType*            rel_op;
    unique_ptr<RelType> op_data;

    virtual ~RelOp() = default;

    RelOp(Rel* srel, RelType* sop)
      : rel(srel), rel_op(sop) {}

    RelOp(Rel* srel, unique_ptr<RelType>&& sop)
      : rel(srel), op_data(std::move(sop)) { rel_op = op_data.get(); }

    constexpr bool IsSink()   const override { return is_sink<RelType>::value;   }
    constexpr bool IsOrigin() const override { return is_origin<RelType>::value; }

    OpTreeItr       InputRels() override { return GetInputs(rel, rel_op);        }
    unique_ptr<Rel> CopyRel()   override { return CopySubstraitRel(rel, rel_op); }

    void         Print()    override { PrintMessage(*rel_op);      }
    const string ViewStr()  override { return StringifyOp(rel_op); }

    string ToString() override {
      string op_str;
      if (StringifyMessage(*rel_op, &op_str)) { return op_str; }

      return string {};
    }

    bool operator==()(const RelOp& left, const RelOp& right) {
      MessageDifferencer* differ = GetComparator(left.rel_op);
      return differ->Compare(*(left.rel_op), *(right.rel_op));
    }
  };

  using RelOpVariant = std::variant< RelOp<ProjectRel>
                                    ,RelOp<FilterRel>
                                    ,RelOp<FetchRel>
                                    ,RelOp<SortRel>
                                    ,RelOp<AggregateRel>
                                    ,RelOp<JoinRel>
                                    ,RelOp<CrossRel>
                                    ,RelOp<HashJoinRel>
                                    ,RelOp<MergeJoinRel>
                                    ,RelOp<ReferenceRel>
                                    ,RelOp<ExtensionLeafRel>
                                    ,RelOp<ReadRel>>;

  //! A simple interface to an operator's inputs
  struct OpTreeItr {
    RelOpVariant         parent_op;
    vector<RelOpVariant> input_ops;

    OpTreeItr(RelOpVariant rel, vector<RelOpVariant> inputs)
      : parent_op(rel), input_ops(inputs) {}

    size_t size() const { return input_ops.size(); }

    RelOpVariant parent()               const { return parent_op;        }
    RelOpVariant operator[](size_t ndx) const { return input_ops[ndx];   }

    vector<RelOpVariant>::iterator begin() { return input_ops.begin(); }
    vector<RelOpVariant>::iterator end()   { return input_ops.end();   }

    vector<RelOpVariant>::const_iterator cbegin() const { return input_ops.begin(); }
    vector<RelOpVariant>::const_iterator cend()   const { return input_ops.end();   }
  };

} // namespace: mohair
