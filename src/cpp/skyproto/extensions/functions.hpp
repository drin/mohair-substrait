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
#pragma once

#include "mohair.hpp"


// ------------------------------
// Aliases

namespace skyproto::extensions {

  // >> Forwarded types
  struct SubstraitFunction;

  // >> Standard types
  using ::mohair::unique_ptr;
  using ::mohair::string;
  using ::mohair::vector;
  using ::mohair::unordered_map;

  // >> Substrait types
  using ::mohair::SubstraitType;

  // >> Templated types
  using ParamTypeVector = vector<SubstraitType>;
  using ParamTypeMatrix = vector<vector<string>>;

  using FunctionMap = unordered_map<string, unique_ptr<SubstraitFunction>>;

} // namespace: skyproto::extensions


// ------------------------------
// Classes

namespace skyproto::extensions {

  // >> Base classes to represent substrait functions and function extensions

  //! An individual substrait function
  struct SubstraitFunction {
    string          name;
    ParamTypeVector ptypes;
    SubstraitType   rtype;
    string          extension_uri;

    SubstraitFunction(string fn_name, ParamTypeVector p_types, SubstraitType r_type)
        :  name(fn_name), ptypes(std::move(p_types)), rtype(r_type) {}

    bool operator==(const SubstraitFunction &other) const;

    string Signature();
  };

  //! An extension describing a set of substrait functions
  struct SubstraitFunctionExt {
    string      extension_uri;
    FunctionMap fn_map;

    SubstraitFunctionExt(string ext_uri): extension_uri(ext_uri) {}

    bool AddFunction(unique_ptr<SubstraitFunction>&& fn);
    bool AddFunction(string fn_name, vector<string> ptypes, string rtype);

    SubstraitFunction* GetFunction(const string& fn_sig);
  };

} // namespace skyproto::extensions
