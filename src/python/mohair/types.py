#!/usr/bin/env python

# ------------------------------
# License

# Copyright 2022-2025 Aldrin Montana
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.


# ------------------------------
# Module Docstring
"""
Types for processing Substrait and Mohair.

The overall library is named mohair and mohair-specific extensions for Substrait are under
skyproto/mohair. This is a bit confusing but we'll have to deal with it for now.
"""


# ------------------------------
# Dependencies

# >> Standard types
from dataclasses import dataclass, field
from typing import Any

# >> Substrait types
from skyproto.substrait.plan_pb2 import Plan


# >> Internal functions
from mohair import CreateMohairLogger


# ------------------------------
# Module Variables

# >> Logging
logger = CreateMohairLogger(__name__)

# >> Forward references (Type aliases)
type MohairOp     = 'MohairOp'


# ------------------------------
# Classes

# >> Operator classes
@dataclass
class MohairOp:
    """
    A single operator in a query plan that primarily maintains references to operators in
    the substrait plan.
    """

    substrait_op: Any
    op_inputs   : tuple[MohairOp, ...]

    def __str__(self):
        # op_name = self.substrait_op.DESCRIPTOR.name

        # op_str = f'{op_name}()\t{op_inputs[0]}'
        # for input_op in op_inputs[1:]:
        #     op_str += f'\n\t{input_op}'

        return f'UnknownOp()'

@dataclass
class StreamOp(MohairOp):
    """ A trait representing an operator that tuples can stream through. """

    def ViewStr(self, prefix=''): return f'{prefix}← {self}'

@dataclass
class SinkOp(MohairOp):
    """ A trait representing a stateful operator that cannot stream tuples. """

    def ViewStr(self, prefix=''): return f'{prefix}↤ {self}'

@dataclass
class OriginOp(MohairOp):
    """ A trait representing an operator that reads from a data source. """

    source_name : str = ''

    def ViewStr(self, prefix=''): return f'{prefix}↤ {self}'

    def __str__(self):
        op_name = self.substrait_op.DESCRIPTOR.name
        return f'{op_name}({self.source_name})'

