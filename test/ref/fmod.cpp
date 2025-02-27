/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2015-2023 Advanced Micro Devices, Inc. All rights reserved.
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
#include <migraphx/instruction.hpp>
#include <migraphx/literal.hpp>
#include <migraphx/make_op.hpp>
#include <migraphx/onnx.hpp>
#include <migraphx/register_target.hpp>
#include <migraphx/verify.hpp>

#include <test.hpp>

TEST_CASE(fmod_test)
{
    migraphx::program p;
    auto* mm = p.get_main_module();
    migraphx::shape s{migraphx::shape::int32_type, {3}};
    auto l0       = mm->add_literal(migraphx::literal{s, {-7, 8, -3}});
    auto l1       = mm->add_literal(migraphx::literal{s, {2, 4, 6}});
    auto l2       = mm->add_literal(migraphx::literal{s, {7, 5, 9}});
    auto curr_mod = mm->add_instruction(migraphx::make_op("fmod"), l0, l1);
    mm->add_instruction(migraphx::make_op("fmod"), curr_mod, l2);
    p.compile(migraphx::make_target("ref"));
    auto result = p.eval({}).back();
    std::vector<float> results_vector(4);
    result.visit([&](auto output) { results_vector.assign(output.begin(), output.end()); });
    std::vector<float> gold{-1, 0, -3};
    EXPECT(migraphx::verify::verify_range(results_vector, gold));
}

TEST_CASE(fmod_dyn_test)
{
    migraphx::program p;
    auto* mm = p.get_main_module();
    std::vector<migraphx::shape::dynamic_dimension> dd{{2, 6}};
    migraphx::shape s{migraphx::shape::float_type, dd};
    auto x        = mm->add_parameter("x", s);
    auto y        = mm->add_parameter("y", s);
    auto z        = mm->add_parameter("z", s);
    auto curr_mod = mm->add_instruction(migraphx::make_op("fmod"), x, y);
    mm->add_instruction(migraphx::make_op("fmod"), curr_mod, z);
    p.compile(migraphx::make_target("ref"));

    std::vector<float> x_data{-7, 8, -3};
    std::vector<float> y_data{2, 4, 6};
    std::vector<float> z_data{7, 5, 9};
    migraphx::parameter_map params0;
    migraphx::shape input_fixed_shape0{migraphx::shape::float_type, {3}};
    params0["x"] = migraphx::argument(input_fixed_shape0, x_data.data());
    params0["y"] = migraphx::argument(input_fixed_shape0, y_data.data());
    params0["z"] = migraphx::argument(input_fixed_shape0, z_data.data());
    auto result  = p.eval(params0).back();
    std::vector<float> results_vector(4);
    result.visit([&](auto output) { results_vector.assign(output.begin(), output.end()); });
    std::vector<float> gold{-1, 0, -3};
    EXPECT(migraphx::verify::verify_range(results_vector, gold));
}

TEST_CASE(fmod_float_test)
{
    migraphx::program p;
    auto* mm = p.get_main_module();
    migraphx::shape s{migraphx::shape::float_type, {3}};
    auto l0       = mm->add_literal(migraphx::literal{s, {-7.2f, 8.5f, -3.3f}});
    auto l1       = mm->add_literal(migraphx::literal{s, {2.0f, 4.0f, 6.0f}});
    auto l2       = mm->add_literal(migraphx::literal{s, {7.0f, 5.0f, 9.0f}});
    auto curr_mod = mm->add_instruction(migraphx::make_op("fmod"), l0, l1);
    mm->add_instruction(migraphx::make_op("fmod"), curr_mod, l2);

    p.compile(migraphx::make_target("ref"));
    auto result = p.eval({}).back();
    std::vector<float> results_vector(4);
    result.visit([&](auto output) { results_vector.assign(output.begin(), output.end()); });
    std::vector<float> gold{-1.2f, 0.5f, -3.3f};
    EXPECT(migraphx::verify::verify_range(results_vector, gold));
}
