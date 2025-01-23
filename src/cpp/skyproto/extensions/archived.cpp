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
// Archived (started implementing but don't yet need)

// >> mohair.hpp
/*

  // #include "skyproto/extensions/functions.hpp"

  // >> Extension type aliases
  using skyproto::extensions::SubstraitFunction;
  using skyproto::extensions::SubstraitFunctionExt;

  using FunctionExtMap = unordered_map<string, unique_ptr<SubstraitFunctionExt>>;

  // >> Static variables
  static FunctionExtMap fn_extensions;

*/

// >> plans.hpp
/*

  // >> Aliases
  // using FunctionExtAnchorMap = unordered_map<uint64_t, SubstraitFunctionExt*>;
  // using FunctionAnchorMap    = unordered_map<uint64_t, SubstraitFunction*>;

  // >> SystemPlan attributes
  // FunctionExtAnchorMap    ext_anchors;

*/

// >> plans.cpp
/*

  SubstraitFunction* SystemPlan::FnForAnchor(uint64_t anchor_id) {
    const auto& anchor_entry = fn_anchors.find(anchor_id);
    if (anchor_entry == fn_anchors.end()) {
      throw std::out_of_range(
        "Extension function anchor not registered: " + std::to_string(anchor_id)
      );
    }

    return anchor_entry->second;
  }

  bool SystemPlan::AddFnAnchor(uint64_t anchor_id, SubstraitFunction* new_fn) {
    const auto& anchor_entry = fn_anchors.find(anchor_id);
    if (anchor_entry == fn_anchors.end()) { return false; }

    fn_anchors[anchor_id] = new_fn;
    return true;
  }

  //! Create mapping of function anchors in the query plan
  void SystemPlan::RegisterExtensionFunctions() {
    // First, populate our FunctionExtensionMap
    for (auto &ext_info : plan_msg->payload->extension_uris()) {
      const auto    anchor_id = ext_info.extension_uri_anchor();
      const string& ext_uri   = ext_info.uri();

      const auto& ext_entry = mohair::fn_extensions.find(ext_uri);
      if (ext_entry == mohair::fn_extensions.end()) {
        std::cerr << "Unable to find information for extension" << std::endl
                  << "\t[" << ext_uri << "]"                    << std::endl
        ;
        throw std::out_of_range("Unknown function extension: " + ext_uri);
      }

      ext_anchors[anchor_id] = ext_entry->second.get();
    }

    // Then, populate our FunctionMap
    for (auto &plan_ext : plan_msg->payload->extensions()) {
      if (!plan_ext.has_extension_function()) { continue; }

      const auto    ext_anchor = plan_ext.extension_function().extension_uri_reference();
      const auto    fn_anchor  = plan_ext.extension_function().function_anchor();
      const string& fn_name    = plan_ext.extension_function().name();

      const auto& ext_entry = ext_anchors.find(ext_anchor);
      if (ext_entry == ext_anchors.end()) {
        std::cerr << "Unable to find Extension for anchor ["
                  << std::to_string(ext_anchor)
                  << "]"
                  << std::endl
        ;

        throw std::out_of_range(
          "Unknown Function Extension with ID" + std::to_string(ext_anchor)
        );
      }

      fn_anchors[fn_anchor] = ext_entry->second->GetFunction(fn_name);
    }
  }

*/
