// ------------------------------
// License(s)
//
// >> For original duckdb code
// Copyright 2018-2024 Stichting DuckDB Foundation
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy of this
// software and associated documentation files (the "Software"), to deal in the Software
// without restriction, including without limitation the rights to use, copy, modify,
// merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to the following
// conditions:
//
// The above copyright notice and this permission notice shall be included in all copies
// or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
// PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
// CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
// THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
// >> For my modifications
//    (figure out what amount of modifications allows me to change the license at all)
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

#include "skyproto/extensions/functions.hpp"


// ------------------------------
// Functions

namespace skyproto::extensions {

  string StringifyType(SubstraitType& type) {
    switch (type.kind_case()) {
      case SubstraitType::KindCase::kBool:        return "bool"
      case SubstraitType::KindCase::kI8:          return "i8";
      case SubstraitType::KindCase::kI16:         return "i16";
      case SubstraitType::KindCase::kI32:         return "i32";
      case SubstraitType::KindCase::kI64:         return "i64";
      case SubstraitType::KindCase::kFp32:        return "fp32";
      case SubstraitType::KindCase::kFp64:        return "fp64";
      case SubstraitType::KindCase::kString:      return "string";
      case SubstraitType::KindCase::kUuid:        return "uuid";
      case SubstraitType::KindCase::kDecimal:     return "decimal";
      case SubstraitType::KindCase::kVarchar:     return "varchar";
      case SubstraitType::KindCase::kBinary:      return "binary";
      case SubstraitType::KindCase::kFixedChar:   return "fixed_char";
      case SubstraitType::KindCase::kFixedBinary: return "fixed_binary";

      case SubstraitType::KindCase::kStruct: return "struct";
      case SubstraitType::KindCase::kList:   return "list";
      case SubstraitType::KindCase::kMap:    return "map";

      case SubstraitType::KindCase::kUserDefined:
      case SubstraitType::KindCase::kUserDefinedTypeReference: return "user_defined";

      case SubstraitType::KindCase::kDate:                 return "date";
      case SubstraitType::KindCase::kTime:                 return "time";
      case SubstraitType::KindCase::kIntervalDay:          return "interval_day";
      case SubstraitType::KindCase::kIntervalYear:         return "interval_year";
      case SubstraitType::KindCase::kIntervalCompound:     return "interval_compound";
      case SubstraitType::KindCase::kTimestamp:            return "precision_timestamp";
      case SubstraitType::KindCase::kTimestampTz:          return "precision_timestamp_tz";
      case SubstraitType::KindCase::kPrecisionTimestamp:   return "precision_timestamp";
      case SubstraitType::KindCase::kPrecisionTimestampTz: return "precision_timestamp_tz";

      default:
        std::cerr << "Unable to stringify unknown data type:"     << std::endl
                  << "\t Kind case: [" << type.kind_case() << "]" << std::endl
        ;

        return ""; 
    }
  }

  SubstraitType TypeForString(string& type_str) {
    SubstraitType data_type;

    // TODO: can't call this, need to call something else
    if      (type_str == "bool")         { data_type.set_has_bool_();       }
    else if (type_str == "i8")           { data_type.set_has_i8();          }
    else if (type_str == "i16")          { data_type.set_has_i16();         }
    else if (type_str == "i32")          { data_type.set_has_i32();         }
    else if (type_str == "i64")          { data_type.set_has_i64();         }
    else if (type_str == "fp32")         { data_type.set_has_fp32();        }
    else if (type_str == "fp64")         { data_type.set_has_fp64();        }
    else if (type_str == "string")       { data_type.set_has_string();      }
    else if (type_str == "uuid")         { data_type.set_has_uuid();        }
    else if (type_str == "decimal")      { data_type.set_has_decimal();     }
    else if (type_str == "varchar")      { data_type.set_has_varchar();     }
    else if (type_str == "binary")       { data_type.set_has_binary();      }
    else if (type_str == "fixed_char")   { data_type.set_has_fixedChar();   }
    else if (type_str == "fixed_binary") { data_type.set_has_fixedBinary(); }

    else if (type_str == "struct")       { data_type.set_has_struct_()      }
    else if (type_str == "list")         { data_type.set_has_list()         }
    else if (type_str == "map")          { data_type.set_has_map()          }

    else if (type_str == "user_defined") { data_type.set_has_user_defined() }

    else if (type_str == "date")                    { data_type.set_has_date()                 }
    else if (type_str == "time")                    { data_type.set_has_time()                 }
    else if (type_str == "interval_day")            { data_type.set_has_intervalDay()          }
    else if (type_str == "interval_year")           { data_type.set_has_intervalYear()         }
    else if (type_str == "interval_compound")       { data_type.set_has_intervalCompound()     }
    else if (type_str == "precision_timestamp")     { data_type.set_has_precisionTimestamp()   }
    else if (type_str == "precision_timestamp_tz")  { data_type.set_has_precisionTimestampTz() }

    return data_type;
  }

} // namespace: skyproto::extensions


// ------------------------------
// Classes

namespace skyproto::extensions {

  // >> Method implementations for SubstraitFunction
  bool SubstraitFunction::operator==(const SubstraitFunction &other) const {
    return name == other.name && ptypes == other.ptypes && rtype  == other.rtype;
  }

  string SubstraitFunction::Signature() {
    stringstream sig_stream;

    sig_stream << name << ":";

    if (not ptypes.empty()) { sig_stream << StringifyType(ptypes[0]); }
    for (size_t param_ndx = 1; param_ndx < ptypes.size(); ++param_ndx) {
      sig_stream << "_" << StringifyType(ptypes[param_ndx]);
    }

    return sig_stream.str();
  }

  // >> Method implementations for SubstraitFunctionExt

  bool SubstraitFunctionExt::AddFunction(unique_ptr<SubstraitFunction>&& fn) {
    string      fn_sig   { fn->Signature() };
    const auto& fn_entry = fn_map.find(fn_sig);
    if (fn_entry != fn_map.end()) { return false; }

    fn->extension_uri = this->extension_uri;
    fn_map[fn_sig] = std::move(fn);

    return true;
  }

  bool SubstraitFunctionExt::AddFunction( string         fn_name
                                         ,vector<string> ptypes
                                         ,string         rtype) {
    ParamTypeVector substrait_ptypes;
    substrait_ptypes.reserve(ptypes.size());

    for (size_t param_ndx = 0; param_ndx < ptypes.size(); ++param_ndx) {
      substrait_ptypes.push_back(TypeForString(ptypes[param_ndx]));
    }

    return AddFunction(
      std::make_unique<SubstraitFunction>(
        fn_name, substrait_ptypes, TypeForString(rtype)
      )
    );
  }

  SubstraitFunction* SubstraitFunctionExt::GetFunction(string& fn_sig) {
    const auto& fn_entry = fn_map.find(fn_sig);
    if (fn_entry == fn_map.end()) {
      throw std::out_of_range("Unknown function signature: " + fn_sig);
    }

    return fn_entry->second;
  }


} // namespace skyproto::extensions
