#!/usr/bin/env python

# ------------------------------
# License

# Copyright 2023-2025 Aldrin Montana
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
Convenience classes and functions for processing relational operators.
"""


# ------------------------------
# Dependencies

# >> Standard libs
from typing import Any

from operator    import attrgetter
from functools   import singledispatch
from dataclasses import dataclass

# >> Substrait types
# Base and common types
from skyproto.substrait.algebra_pb2 import Rel, RelCommon

# Leaf operators
from skyproto.substrait.algebra_pb2 import ReadRel, ExtensionLeafRel

# Unary operators
from skyproto.substrait.algebra_pb2 import ProjectRel, FilterRel, FetchRel
from skyproto.substrait.algebra_pb2 import AggregateRel, SortRel

# N-ary operators
from skyproto.substrait.algebra_pb2 import CrossRel, JoinRel, HashJoinRel, MergeJoinRel

#       |> Mohair extensions
from skyproto.mohair.algebra_pb2 import SkyRel, SkySliceRel, SkyPartitionRel

# >> Mohair types
from mohair.types import MohairOp, StreamOp, SinkOp, OriginOp

# >> Internal functions
from mohair import CreateMohairLogger


# ------------------------------
# Module Variables

# >> Logging
logger = CreateMohairLogger(__name__)


# ------------------------------
# Classes

# >> Query operators that can stream data
@dataclass
class Projection(StreamOp):
    substrait_op: ProjectRel

    def __str__(self): return f'π'

@dataclass
class Selection(StreamOp):
    substrait_op: FilterRel

    def __str__(self): return f'σ'

@dataclass
class Limit(StreamOp):
    substrait_op: FetchRel

    def __str__(self): return f'⌈n⌉'


# >> Leaf query operators (can stream data and are base cases)
@dataclass
class Read(OriginOp):
    substrait_op: ReadRel
    op_inputs   : tuple[()] = ()

    def __str__(self) -> str:
        return f'Read({self.source_name})'

    def __post_init__(self):
        # grab the name of the table if it's available
        if self.substrait_op.HasField('named_table'):
            self.source_name = '/'.join(self.substrait_op.named_table.names)

        else:
            self.source_name = self.read_type()

    def __str__(self):
        read_type = self.read_type()

        return f'Read({read_type}:{self.source_name})'

    def read_type(self):
        return self.substrait_op.WhichOneof('read_type')


# >> Skytether operators
@dataclass
class ReadSkyRel(OriginOp):
    """
    A custom query operator to be used in a substrait query plan. This provides a way for
    the schema to be resolved at a remote query engine instead of having to know it up
    front.
    """

    substrait_op: SkyRel
    op_inputs   : tuple[()] = ()

    def __str__(self) -> str:
        return f'SkyRel({self.source_name})'

    def __post_init__(self):
        dname = self.domain_key()
        pname = self.partition_key()

        self.source_name = f'{dname}/{pname}'

    def domain_key(self):
        return self.substrait_op.domain

    def partition_key(self):
        return self.substrait_op.partition

@dataclass
class ReadSkySlice(OriginOp):
    """
    A custom query operator to be used in a substrait query plan. This provides a way for
    the schema to be resolved at a remote query engine instead of having to know it up
    front.
    """

    substrait_op: SkySliceRel
    op_inputs   : tuple[()] = ()

    def __str__(self) -> str:
        # return f'SkySlice({self.source_name})'
        return f'Scan'

    def __post_init__(self):
        self.source_name = f'{self.substrait_op.slice_key}'

    def domain_key(self):
        return self.substrait_op.domain

    def partition_key(self):
        return self.substrait_op.partition

    def slice_key(self):
        return self.substrait_op.slice_key

@dataclass
class ReadSkyPartition(OriginOp):
    """
    A custom query operator to be used in a substrait query plan. This provides a way for
    the schema to be resolved at a remote query engine instead of having to know it up
    front.
    """

    substrait_op: SkyPartitionRel
    op_inputs   : tuple[()] = ()

    def __str__(self) -> str:
        return f'SkyPartition({self.source_name})'

    def __post_init__(self):
        dname = self.domain_key()
        pname = self.partition_key()

        self.source_name = f'{dname}/{pname}'

    def domain_key(self):
        return self.substrait_op.domain

    def partition_key(self):
        return self.substrait_op.partition


# >> Query operators that cannot stream data
@dataclass
class Sort(SinkOp):
    substrait_op: SortRel

    def __str__(self): return f'⊕'

@dataclass
class Aggregation(SinkOp):
    substrait_op: AggregateRel

    def __str__(self): return f'Γ'

@dataclass
class Join(SinkOp):
    substrait_op: JoinRel
    op_inputs   : tuple[MohairOp, MohairOp]

    def __str__(self): return f'⋈'

@dataclass
class HashJoin(Join):
    substrait_op: HashJoinRel

    def __str__(self): return f'⋈→'

@dataclass
class MergeJoin(Join):
    substrait_op: MergeJoinRel

    def __str__(self): return f'⋈⊕'


# ------------------------------
# Functions for parsing a substrait query plan

@singledispatch
def MohairFrom(substrait_op) -> Any:
    """
    Recursive function to parse the query plan. This handler executes if no other
    registered handler matches the argument type.
    """

    raise NotImplementedError(f'No implementation for operator: {substrait_op}')


@MohairFrom.register
def _from_rel(substrait_op: Rel) -> Any:
    """
    Translation function that propagates through the generic 'Rel' message.
    """

    op_rel = getattr(substrait_op, substrait_op.WhichOneof('rel_type'))
    return MohairFrom(op_rel)


# >> Translations for unary relations
@MohairFrom.register
def _from_project(project_op: ProjectRel) -> Any:
    logger.debug('translating <Project>')

    op_inputs = (MohairFrom(project_op.input),)
    return Projection(project_op, op_inputs)


@MohairFrom.register
def _from_filter(filter_op: FilterRel) -> Any:
    logger.debug('translating <Filter>')

    op_inputs = (MohairFrom(filter_op.input),)
    return Selection(filter_op, op_inputs)


@MohairFrom.register
def _from_fetch(fetch_op: FetchRel) -> Any:
    logger.debug('translating <Fetch>')

    op_inputs = (MohairFrom(fetch_op.input),)
    return Limit(fetch_op, op_inputs)


@MohairFrom.register
def _from_sort(sort_op: SortRel) -> Any:
    logger.debug('translating <Sort>')

    op_inputs = (MohairFrom(sort_op.input),)
    return Sort(sort_op, op_inputs)


@MohairFrom.register
def _from_aggregate(aggregate_op: AggregateRel) -> Any:
    logger.debug('translating <Aggregate>')

    op_inputs = (MohairFrom(aggregate_op.input),)
    return Aggregation(aggregate_op, op_inputs)


# >> Translations for leaf relations
@MohairFrom.register
def _from_readrel(read_op: ReadRel) -> Any:
    logger.debug('translating <ReadRel>')

    return Read(read_op)

@MohairFrom.register
def _from_extleaf(leaf_op: ExtensionLeafRel) -> Any:
    logger.debug('translating <ExtensionLeafRel>')

    if leaf_op.detail.Is(SkyRel.DESCRIPTOR):
        sky_rel = SkyRel()
        leaf_op.detail.Unpack(sky_rel)
        return MohairFrom(sky_rel)

    elif leaf_op.detail.Is(SkySliceRel.DESCRIPTOR):
        sky_slicerel = SkySliceRel()
        leaf_op.detail.Unpack(sky_slicerel)
        return MohairFrom(sky_slicerel)

    elif leaf_op.detail.Is(SkyPartitionRel.DESCRIPTOR):
        sky_partrel = SkyPartitionRel()
        leaf_op.detail.Unpack(sky_partrel)
        return MohairFrom(sky_partrel)

    raise NotImplementedError(
        f'Unknown extension type: {leaf_op.detail.TypeName()}'
    )

@MohairFrom.register
def _from_skyrel(sky_op: SkyRel) -> Any:
    logger.debug('translating <SkyRel>')

    return ReadSkyRel(sky_op)

@MohairFrom.register
def _from_skyslicerel(sky_op: SkySliceRel) -> Any:
    logger.debug('translating <SkySliceRel>')

    return ReadSkySlice(sky_op)

@MohairFrom.register
def _from_skypartitionrel(sky_op: SkyPartitionRel) -> Any:
    logger.debug('translating <SkyPartitionRel>')

    return ReadSkyPartition(sky_op)


# >> Translations for join and n-ary relations
@MohairFrom.register
def _from_joinrel(join_op: JoinRel) -> Any:
    logger.debug('translating <JoinRel>')

    op_inputs = (MohairFrom(join_op.left), MohairFrom(join_op.right))
    return Join(join_op, op_inputs)


# ------------------------------
# Functions for reconstructing a substrait Rel

@singledispatch
def SubstraitFrom(mohair_op: Any) -> Rel:
    """ Recursive function to convert a MohairOp wrapper to a substrait Rel. """

    raise NotImplementedError(f'No implementation for type: {type(mohair_op)}')

# >> Translations for unary relations
@SubstraitFrom.register
def _to_project(mohair_op: Projection) -> Rel:
    return Rel(project=mohair_op.substrait_op)

@SubstraitFrom.register
def _to_filter(mohair_op: Selection) -> Rel:
    return Rel(filter=mohair_op.substrait_op)

@SubstraitFrom.register
def _to_fetch(mohair_op: Limit) -> Rel:
    return Rel(fetch=mohair_op.substrait_op)

@SubstraitFrom.register
def _to_sort(mohair_op: Sort) -> Rel:
    return Rel(sort=mohair_op.substrait_op)

@SubstraitFrom.register
def _to_aggregate(mohair_op: Aggregation) -> Rel:
    return Rel(aggregate=mohair_op.substrait_op)

@SubstraitFrom.register
def _to_readrel(mohair_op: Read) -> Rel:
    return Rel(read=mohair_op.substrait_op)

@SubstraitFrom.register
def _to_skyrel(mohair_op: ReadSkyPartition) -> Rel:
    sky_rel = Rel(extension_leaf=ExtensionLeafRel(
         common=RelCommon(direct=RelCommon.Direct())
    ))

    # because of how this property is defined, we have to explicitly
    # pack our custom substrait message
    sky_rel.extension_leaf.detail.Pack(mohair_op.substrait_op)

    return sky_rel

# >> Translations for join and n-ary relations
@SubstraitFrom.register
def _to_joinrel(mohair_op: Join) -> Rel:
    return Rel(join=mohair_op.substrait_op)

@SubstraitFrom.register
def _to_hash_joinrel(mohair_op: HashJoin) -> Rel:
    return Rel(hash_join=mohair_op.substrait_op)

@SubstraitFrom.register
def _to_merge_joinrel(mohair_op: MergeJoin) -> Rel:
    return Rel(merge_join=mohair_op.substrait_op)

