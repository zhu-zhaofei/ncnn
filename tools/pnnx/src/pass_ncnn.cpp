// Tencent is pleased to support the open source community by making ncnn available.
//
// Copyright (C) 2021 THL A29 Limited, a Tencent company. All rights reserved.
//
// Licensed under the BSD 3-Clause License (the "License"); you may not use this file except
// in compliance with the License. You may obtain a copy of the License at
//
// https://opensource.org/licenses/BSD-3-Clause
//
// Unless required by applicable law or agreed to in writing, software distributed
// under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
// CONDITIONS OF ANY KIND, either express or implied. See the License for the
// specific language governing permissions and limitations under the License.

#include "pass_ncnn.h"

#include "pass_ncnn/convert_custom_op.h"
#include "pass_ncnn/convert_input.h"
#include "pass_ncnn/eliminate_output.h"
#include "pass_ncnn/expand_expression.h"
#include "pass_ncnn/insert_split.h"
#include "pass_level4/dead_code_elimination.h"
#include "pass_level4/canonicalize.h"
#include "pass_level5/unroll_rnn_op.h"

namespace pnnx {

static std::map<int, std::vector<const GraphRewriterPass*> > g_global_pnnx_ncnn_graph_rewriter_passes;

NcnnGraphRewriterPassRegister::NcnnGraphRewriterPassRegister(const GraphRewriterPass* pass, int priority)
{
    if (g_global_pnnx_ncnn_graph_rewriter_passes.find(priority) == g_global_pnnx_ncnn_graph_rewriter_passes.end())
    {
        g_global_pnnx_ncnn_graph_rewriter_passes[priority] = std::vector<const GraphRewriterPass*>();
    }

    g_global_pnnx_ncnn_graph_rewriter_passes[priority].push_back(pass);
}

void pass_ncnn(Graph& g)
{
    unroll_rnn_op(g);

    ncnn::insert_split(g);

    ncnn::expand_expression(g);

    int opindex = 0;
    for (auto x : g_global_pnnx_ncnn_graph_rewriter_passes)
    {
        for (auto rewriter : x.second)
        {
            pnnx_graph_rewrite(g, rewriter, opindex);
        }
    }

    dead_code_elimination(g);

    canonicalize(g);

    ncnn::convert_custom_op(g);

    ncnn::convert_input(g);

    ncnn::eliminate_output(g);
}

} // namespace pnnx