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
// Overview
// "External" dependencies from the C++ standard library


// ------------------------------
// Dependencies
#pragma once

// >> Configuration
#include "mohair-config.hpp"

// >> Memory and data type support
#include <memory>
#include <optional>
#include <variant>
#include <string>
#include <array>
#include <vector>
#include <unordered_map>

// >> I/O support
#include <iostream>
#include <sstream>
#include <fstream>

// >> Logging support
#include <chrono>
#include <ctime>
#include <iomanip>


// ------------------------------
// Type aliases and macros

namespace mohair {

  using std::unique_ptr;
  using std::shared_ptr;
  using std::optional;
  using std::string;

    using std::array;
  using std::vector;
  using std::unordered_map;

  using std::stringstream;
  using std::fstream;


  #if MOHAIR_DEBUG
    constexpr const char* MOHAIR_LOGFILE = "mohair.debug.log";
    static std::fstream MOHAIR_LOGSTREAM {
      MOHAIR_LOGFILE, std::ios::out | std::ios::app
    };

    #define MohairLogMsgFull(log_stream, msg_str) { \
      do {                                          \
        log_stream << "[" << NowAsNano() << "] "    \
                   << __LINE__ << " | " << msg_str  \
                   << std::endl                     \
        ;                                           \
      } while (0);                                  \
    }

    #define MohairLogMsg(msg_str) {                \
      MohairLogMsgFull(MOHAIR_LOGSTREAM, msg_str); \
    }

  #endif

} // namespace: mohair
