/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2015-2022 Advanced Micro Devices, Inc. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef MIGRAPHX_GUARD_RTGLIB_GPU_GEMM_HPP
#define MIGRAPHX_GUARD_RTGLIB_GPU_GEMM_HPP

#include <migraphx/errors.hpp>
#include <migraphx/operation.hpp>
#include <migraphx/value.hpp>
#include <migraphx/shape.hpp>
#include <migraphx/reflect.hpp>
#include <migraphx/gpu/context.hpp>
#include <migraphx/gpu/gemm_impl.hpp>
#include <migraphx/op/quant_dot.hpp>
#include <migraphx/op/dot.hpp>
#include <migraphx/ranges.hpp>

namespace migraphx {
inline namespace MIGRAPHX_INLINE_NS {
namespace gpu {

struct context;

void blas_shape(const shape& s);
shape transpose_batch(const shape& s, unsigned trans_batch);

template <class Op>
struct rocblas_gemm
{
    Op op;
    float alpha          = 1;
    float beta           = 0;
    bool int8_x4_format  = true;
    bool compute_fp32    = false;
    unsigned trans_batch = 0;

    template <class Self, class F>
    static auto reflect(Self& self, F f)
    {
        return pack_join(migraphx::reflect(self.op, f),
                         pack(f(self.alpha, "alpha"),
                              f(self.beta, "beta"),
                              f(self.int8_x4_format, "int8_x4_format"),
                              f(self.compute_fp32, "compute_fp32"),
                              f(self.trans_batch, "trans_batch")));
    }

    std::string name() const
    {
        if(contains(op.name(), "quant_"))
        {
            return "gpu::quant_gemm";
        }
        return "gpu::gemm";
    }

    shape compute_shape(const std::vector<shape>& inputs) const
    {
        std::vector<shape> in_shapes(inputs);
        in_shapes.pop_back();
        check_shapes{in_shapes, *this}.has(2, 3);
        blas_shape(inputs[0]);
        blas_shape(inputs[1]);
        // if gemm and add are fused
        if(in_shapes.size() > 2)
        {
            auto cmat_shape = in_shapes.back();
            check_shapes{{cmat_shape}, *this}.not_transposed().not_broadcasted();
            in_shapes.pop_back();
            blas_shape(cmat_shape);
            auto op_out_shape = op.compute_shape(in_shapes);
            if(cmat_shape.lens() != op_out_shape.lens())
            {
                MIGRAPHX_THROW(this->name() + " : dimension mismatch, operand C: {" +
                               to_string_range(cmat_shape.lens()) +
                               "}, cannot add to operand A * B: {" +
                               to_string_range(op_out_shape.lens()) + "}");
            }
            if(cmat_shape.type() != op_out_shape.type())
            {
                MIGRAPHX_THROW(this->name() + " : operand C type mismatch, operand C is of type: " +
                               to_string(cmat_shape.type()) +
                               ", it must be: " + to_string(op_out_shape.type()));
            }
            return transpose_batch(op_out_shape, trans_batch);
        }

        return transpose_batch(op.compute_shape(in_shapes), trans_batch);
    }

    argument
    compute(context& ctx, const shape& output_shape, const std::vector<argument>& args) const
    {
        if(this->name() == "gpu::gemm")
        {
            gemm(ctx, output_shape, args, alpha, beta, int8_x4_format, compute_fp32);
        }
        else
        {
            gemm(ctx,
                 output_shape,
                 args,
                 int32_t(alpha),
                 int32_t(beta),
                 int8_x4_format,
                 compute_fp32);
        }
        return args.back();
    }

    std::ptrdiff_t output_alias(const std::vector<shape>& shapes) const
    {
        return shapes.size() - 1;
    }
};

} // namespace gpu
} // namespace MIGRAPHX_INLINE_NS
} // namespace migraphx

#endif
