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
#pragma once


// ------------------------------
// Overview
//
// Dependencies from standard library that are common throughout this library.


// ------------------------------
// Dependencies

// >> Memory and data type support
#include <stdexcept>
#include <memory>
#include <string>
#include <optional>
#include <vector>
#include <unordered_map>

// >> I/O support
#include <iostream>
#include <sstream>
#include <fstream>

// >> Performance and timing support
#include <chrono>

// ------------------------------
// Type aliases

namespace mohair {

  using std::unique_ptr;
  using std::optional;
  using std::string;
  using std::stringstream;

  using std::array;
  using std::vector;
  using std::unordered_map;

  using std::chrono::system_clock;
  using std::chrono::steady_clock;

} // namespace: mohair
