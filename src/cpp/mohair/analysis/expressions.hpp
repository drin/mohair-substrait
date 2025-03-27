// ------------------------------
// License
//
// Copyright 2025 Aldrin Montana
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
// Aliases

namespace mohair {

  using ExprType    = SubstraitExpr::RexTypeCase;
  using LiteralType = SubstraitExpr::Literal::LiteralTypeCase;

  using SubstraitLiteral = SubstraitExpr::Literal;
  using FieldRef         = SubstraitExpr::FieldReference;
  using RefSegment       = SubstraitExpr::ReferenceSegment;
  using ScalarFn         = SubstraitExpr::ScalarFunction;
  using IfThenExpr       = SubstraitExpr::IfThen;
  using CastExpr         = SubstraitExpr::Cast;
  using InClause         = SubstraitExpr::SingularOrList;

} // namespace: mohair


// ------------------------------
// Functions

namespace mohair {

  // >> Type analysis functions
  SubstraitType*
  AnalyzeSubstraitExpr(SubstraitSchema* input_schema, const SubstraitExpr& expr);

} // namespace: mohair
