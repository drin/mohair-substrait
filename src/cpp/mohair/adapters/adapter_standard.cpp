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

// >> Internal API
#include "mohair/adapters/adapter_standard.hpp"


// ------------------------------
// >> Internal API (utility functions)

namespace mohair {

  //! Returns the current time as a nanoseconds value
  std::chrono::nanoseconds NowAsNano() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
      std::chrono::system_clock::now().time_since_epoch()
    );
  }

  //! Returns a binary output stream for the given file path
  fstream OutputStreamForFile(const char* out_fpath) {
    return fstream { out_fpath, std::ios::out | std::ios::trunc | std::ios::binary };
  }

  //! Returns a binary input stream for the given file path
  fstream InputStreamForFile(const char* in_fpath) {
    return fstream { in_fpath, std::ios::in | std::ios::binary };
  }

  //! Reads data from the given file path into an output string as binary
  bool ReadFileIntoString(const char* in_fpath, string& out_filedata) {
    // create an IO stream for the file
    auto file_stream = InputStreamForFile(in_fpath);
    if (!file_stream) {
      MohairLogMsg("Failed to open IO stream for file");
      return false;
    }

    // go to end of stream, read the position, then reset position
    file_stream.seekg(0, std::ios_base::end);
    auto size = file_stream.tellg();
    file_stream.seekg(0);

    // Resize the output and read the file data into it
    out_filedata.resize(size);
    auto output_ptr = &(out_filedata[0]);
    file_stream.read(output_ptr, size);

    // On success, the number of characters read will match size
    return file_stream.gcount() == size;
  }

} // namespace: mohair
