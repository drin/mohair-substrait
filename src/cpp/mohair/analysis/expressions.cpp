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

#include "mohair/analysis/expressions.hpp"


// ------------------------------
// Functions

namespace mohair {

  // >> Type analysis functions
  SubstraitType*
  AnalyzeExprLiteral(SubstraitSchema* input_schema, const SubstraitLiteral& lit) {
    if (lit.has_null()) {
      SubstraitType* new_type = input_schema->mutable_struct_()->add_types();
      new_type->CopyFrom(lit.null());

      return new_type;
    }

    // return a pre-constructed type with static duration
    SubstraitType literal_type;
    switch (lit.literal_type_case()) {
      case LiteralType::kBoolean:
        literal_type.set_allocated_bool_(new SubstraitType::Boolean);
        break;

      case LiteralType::kI8:
        literal_type.set_allocated_i8(new SubstraitType::I8);
        break;

      case LiteralType::kI16:
        literal_type.set_allocated_i16(new SubstraitType::I16);
        break;

      case LiteralType::kI32:
        literal_type.set_allocated_i32(new SubstraitType::I32);
        break;

      case LiteralType::kI64:
        literal_type.set_allocated_i64(new SubstraitType::I64);
        break;

      case LiteralType::kFp32:
        literal_type.set_allocated_fp32(new SubstraitType::FP32);
        break;

      case LiteralType::kFp64:
        literal_type.set_allocated_fp64(new SubstraitType::FP64);
        break;

      case LiteralType::kString:
        literal_type.set_allocated_string(new SubstraitType::String);
        break;

      case LiteralType::kBinary:
        literal_type.set_allocated_binary(new SubstraitType::Binary);
        break;

      case LiteralType::kPrecisionTimestamp:
        literal_type.set_allocated_precision_timestamp(
          new SubstraitType::PrecisionTimestamp
        );
        break;

      case LiteralType::kDate:
        literal_type.set_allocated_date(new SubstraitType::Date);
        break;

      case LiteralType::kTime:
        literal_type.set_allocated_time(new SubstraitType::Time);
        break;

      case LiteralType::kIntervalYearToMonth:
        literal_type.set_allocated_interval_year(
          new SubstraitType::IntervalYear
        );
        break;

      case LiteralType::kIntervalDayToSecond:
        literal_type.set_allocated_interval_day(
          new SubstraitType::IntervalDay
        );
        break;

      case LiteralType::kIntervalCompound:
        literal_type.set_allocated_interval_compound(
          new SubstraitType::IntervalCompound
        );
        break;

      case LiteralType::kPrecisionTimestampTz:
        literal_type.set_allocated_precision_timestamp_tz(
          new SubstraitType::PrecisionTimestampTZ
        );
        break;

      case LiteralType::kDecimal:
        literal_type.set_allocated_decimal(new SubstraitType::Decimal);
        break;

      default:
        throw std::runtime_error(std::to_string(lit.literal_type_case()));
    }

    SubstraitType* new_type = input_schema->mutable_struct_()->add_types();
    new_type->CopyFrom(literal_type);

    return new_type;
  }

  SubstraitType*
  AnalyzeExprFieldRef(SubstraitSchema* input_schema, const FieldRef& field_ref) {
    // resolved Type of the referenced field
    SubstraitType* field_type;

    // We don't yet support masked references
    if (field_ref.has_masked_reference()) {
      throw std::runtime_error("Masked FieldReference not yet supported");
    }

    // A direct reference must be against the input schema (RootReference)
    if (not field_ref.has_root_reference()) {
      throw std::logic_error("Direct FieldRef must refer to input schema");
    }

    int field_ndx = field_ref.direct_reference().struct_field().field();
    field_type    = input_schema->mutable_struct_()->mutable_types(field_ndx);

    // Add a new type that is just a copy of the referenced type
    SubstraitType* new_type = input_schema->mutable_struct_()->add_types();
    new_type->CopyFrom(*field_type);

    return new_type;
  }

  SubstraitType*
  AnalyzeExprScalarFn(SubstraitSchema* input_schema, const ScalarFn& scalar_fn) {
    if (not scalar_fn.has_output_type()) {
      throw std::runtime_error("Scalar function is missing output type");
    }

    // Add a new type that matches the output type of the expression
    SubstraitType* new_type = input_schema->mutable_struct_()->add_types();
    new_type->CopyFrom(scalar_fn.output_type());

    return new_type;
  }

  SubstraitType*
  AnalyzeExprIfThen( [[maybe_unused]] SubstraitSchema*  input_schema
                    ,[[maybe_unused]] const IfThenExpr& expr_ifthen) {
    throw std::runtime_error("'IfThen' expression not yet supported");
  }

  SubstraitType*
  AnalyzeExprCast( [[maybe_unused]] SubstraitSchema* input_schema
                  ,[[maybe_unused]] const CastExpr&  expr_cast) {
    throw std::runtime_error("'Cast' expression not yet supported");
  }

  SubstraitType*
  AnalyzeExprIn( [[maybe_unused]] SubstraitSchema* input_schema
                ,[[maybe_unused]] const InClause&  expr_in) {
    throw std::runtime_error("'IN' expression not yet supported");
  }


  //! Entry point to type analysis of a substrait expression
  SubstraitType*
  AnalyzeSubstraitExpr(SubstraitSchema* input_schema, const SubstraitExpr& expr) {
    switch (expr.rex_type_case()) {
      case ExprType::kLiteral:
        return AnalyzeExprLiteral (input_schema, expr.literal());

      case ExprType::kSelection:
        return AnalyzeExprFieldRef(input_schema, expr.selection());

      case ExprType::kScalarFunction:
        return AnalyzeExprScalarFn(input_schema, expr.scalar_function());

      case ExprType::kIfThen:         return AnalyzeExprIfThen(input_schema, expr.if_then());
      case ExprType::kCast:           return AnalyzeExprCast(input_schema, expr.cast());
      case ExprType::kSingularOrList: return AnalyzeExprIn(input_schema, expr.singular_or_list());
      case ExprType::kSubquery:
      default:
        throw std::runtime_error(
          "Expression type not yet supported: " + std::to_string(expr.rex_type_case())
        );
    }

    return nullptr;
  }

} // namespace: mohair
