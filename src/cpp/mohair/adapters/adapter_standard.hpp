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

// External headers and their internal adapters
#include "mohair/apidep_standard.hpp"


// ------------------------------ 
// Functions

namespace mohair {

  //! Returns the current time as a nanoseconds value
  std::chrono::nanoseconds NowAsNano();

  //! Returns a binary output stream for the given file path
  fstream OutputStreamForFile(const char* out_fpath);

  //! Returns a binary input stream for the given file path
  fstream InputStreamForFile(const char* in_fpath);

  //! Reads data from the given file path into an output string as binary
  bool FileToString(const char* in_fpath, string& file_data);

} // namespace: mohair