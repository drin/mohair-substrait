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
Types specific to query plans and plan-level analysis.
"""


# ------------------------------
# Dependencies

# >> Standard libs
from dataclasses import dataclass, field
from pathlib     import Path

# >> Substrait types
from skyproto.substrait.plan_pb2 import Plan

# >> Internal
from mohair.types import MohairOp, StreamOp, SinkOp, OriginOp

# >> Internal functions
from mohair           import CreateMohairLogger
from mohair.operators import MohairFrom


# ------------------------------
# Module Variables

# >> Logging
logger = CreateMohairLogger(__name__)

# >> Forward references (Type aliases)
type OpPipeline    = 'OpPipeline'
type PipelineStage = 'PipelineStage'
type SystemPlan    = 'SystemPlan'


# ------------------------------
# Classes

@dataclass
class OpPipeline:
    """
    A pipeline is a sequence of operators in the plan that starts with a source and ends
    with a sink.
    """

    pipe_id  : int
    dest_pipe: OpPipeline
    dest_op  : MohairOp
    sink_op  : MohairOp
    source_op: MohairOp       = None
    pipe_ops : list[MohairOp] = field(default_factory=list)

    def __len__(self) -> int:
        """ Length is equal to the size of :pipe_ops: plus the source and sink ops """
        return 2 + len(self.pipe_ops)

    def __str__(self) -> str:
        return self.ViewStr()

    def ViewStr(self, prefix='') -> str:
        next_prefix = prefix + '\t'

        pipe_str = (
              f'{prefix} [ '
            + "".join(op.ViewStr() for op in self.pipe_ops)
            + self.source_op.ViewStr()
            + ' ]'
        )

        return pipe_str

    def AddOp(self, stage_op: MohairOp) -> PipelineStage:
        self.pipe_ops.append(stage_op)
        return self

    def SourceDestOp(self) -> MohairOp:
        """
        Returns the operator that the source operator sends its output to.
        """

        if self.pipe_ops: return self.pipe_ops[-1]
        return self.sink_op


@dataclass
class PipelineStage:
    """
    A stage pipeline stage is a set of OpPipelines that all end at the same sink operator.
    """

    stage_id    : int
    depth       : int
    sink_op     : MohairOp
    next_stage  : PipelineStage
    pipelines   : list[OpPipeline] = field(default_factory=list)
    origin_names: set[str]         = field(default_factory=set)
    length      : int              = 0
    width       : int              = 0

    def __str__(self) -> str:
        return self.ViewStr()

    def __len__(self) -> int:
        """ The length of the longest pipeline. """
        return self.length

    def ViewStr(self, prefix='') -> str:
        next_prefix = prefix + '\t'
        stage_str   = f'{prefix}[{self.stage_id}] {self.sink_op}'
        for op_pipe in self.pipelines:
            stage_str += f'\n{next_prefix}{op_pipe.ViewStr(prefix)}'

        return stage_str

    def CreatePipeline(self, next_pipe: OpPipeline, next_op: MohairOp) -> OpPipeline:
        new_pipeid   = (self.stage_id * 10) + len(self.pipelines)
        new_pipeline = OpPipeline( pipe_id=new_pipeid
                                  ,dest_pipe=next_pipe
                                  ,dest_op=next_op
                                  ,sink_op=self.sink_op)

        self.pipelines.append(new_pipeline)

        self.width += 1
        if len(new_pipeline) > self.length:
            self.length = len(new_pipeline)

        return new_pipeline


@dataclass
class SystemPlan:
    """
    Query plan that propagates through a computational storage system. Also stores
    properties of the plan for later analysis.

    :root_op: The `MohairOp` that is the root of this plan.
    """

    substrait_plan  : Plan
    root_op         : MohairOp = None

    pipeline_stages : list[PipelineStage] = field(default_factory=list)
    origin_pipelines: list[OpPipeline]    = field(default_factory=list)


    @classmethod
    def FromFile(cls, plan_fpath: Path) -> SystemPlan:
        # Parse the plan
        substrait_plan = Plan()
        with open(plan_fpath, 'rb') as plan_fh:
            substrait_plan.ParseFromString(plan_fh.read())

        # Find (and validate) the root Rel
        plan_root = substrait_plan.relations[0]
        if not plan_root.HasField('root'): return None

        substrait_rootop = plan_root.root.input

        # Return the constructed SystemPlan
        return SystemPlan(substrait_plan, MohairFrom(substrait_rootop))


    def __hash__(self):
        plan_hash = hash(self.root_op)
        logger.debug(f'Hash of SystemPlan: {plan_hash}')

        return plan_hash

    def __str__(self) -> str:
        return self.ViewStr()

    def ViewStr(self, prefix='') -> str:
        plan_str = 'System Plan:'
        for stage_ndx, pipe_stage in enumerate(self.pipeline_stages):
            plan_str += (
                f'\n[Stage {stage_ndx}]: ' + pipe_stage.ViewStr(prefix)
            )

        return plan_str

    def CreatePipelineStage( self
                            ,stage_depth: int
                            ,sink_op    : MohairOp
                            ,next_stage : PipelineStage) -> PipelineStage:
        new_stageid   = len(self.pipeline_stages)
        new_pipestage = PipelineStage( stage_id=new_stageid
                                      ,depth=stage_depth
                                      ,sink_op=sink_op
                                      ,next_stage=next_stage)

        self.pipeline_stages.append(new_pipestage)
        return new_pipestage

    def BuildPipelines(self):
        depth       = 1
        final_stage = self.CreatePipelineStage(depth, self.root_op, None)

        for ndx_input, input_op in enumerate(self.root_op.op_inputs):
            stage_pipe = final_stage.CreatePipeline(None, None)
            self.BuildPipelineWithOp(depth + 1, final_stage, stage_pipe, input_op)

            # Update stage statistics after pipeline is constructed 
            if len(stage_pipe) > len(final_stage):
                final_stage.length = len(stage_pipe)

    def BuildPipelineWithOp( self
                            ,depth: int
                            ,stage: PipelineStage
                            ,pipe : OpPipeline
                            ,op   : MohairOp) -> None:
        # For now, assume streamable ops are unary
        while isinstance(op, StreamOp):
            pipe.AddOp(op)
            op = op.op_inputs[0]

        # Seal the current pipeline
        pipe.source_op = op

        # If the source op is a leaf, finish and track the origin pipeline
        if isinstance(op, OriginOp):
            self.origin_pipelines.append(pipe)

            # Propagate source name to "downstream" (closer to root operator)
            stage_itr = stage
            while stage_itr is not None:
                stage_itr.origin_names.add(op.source_name)
                stage_itr = stage_itr.next_stage

        # Otherwise, recurse. Note: "upstream" is closer to leaf operator
        else:
            upstream_stage = self.CreatePipelineStage(depth + 1, op, stage)
            for ndx_input, input_op in enumerate(op.op_inputs):
                dest_op       = pipe.SourceDestOp()
                upstream_pipe = upstream_stage.CreatePipeline(pipe, dest_op)
                self.BuildPipelineWithOp(depth + 1, upstream_stage, upstream_pipe, input_op)

                # Update stage statistics after pipeline is constructed 
                if len(upstream_pipe) > len(upstream_stage):
                    upstream_stage.length = len(upstream_pipe)

