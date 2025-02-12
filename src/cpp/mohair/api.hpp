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
// Overview
//
// This is a single header that includes all other headers necessary to use all of the
// mohair library. The intention is so that users don't have to know which headers are
// necessary for which independent modules of the library (e.g. that "mohair/plans.hpp" is
// what provides query processing types).


// ------------------------------
// Dependencies
#pragma once

// Library configuration
#include "mohair.hpp"

// Internal query processing
#include "mohair/plans.hpp"
