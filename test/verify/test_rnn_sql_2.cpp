
#include "verify_program.hpp"
#include <migraphx/program.hpp>
#include <migraphx/generate.hpp>
#include <migraphx/make_op.hpp>

#include <migraphx/serialize.hpp>

#include <migraphx/op/common.hpp>

struct test_rnn_sql_2 : verify_program<test_rnn_sql_2>
{
    migraphx::program create_program() const
    {
        std::size_t batch_size  = 2;
        std::size_t seq_len     = 10;
        std::size_t hidden_size = 4;
        std::size_t input_size  = 3;
        std::size_t num_dirct   = 1;
        float clip              = 0.0f;

        migraphx::program p;
        auto* mm = p.get_main_module();
        migraphx::shape in_shape{migraphx::shape::float_type, {seq_len, batch_size, input_size}};
        migraphx::shape w_shape{migraphx::shape::float_type, {num_dirct, hidden_size, input_size}};
        migraphx::shape r_shape{migraphx::shape::float_type, {num_dirct, hidden_size, hidden_size}};
        migraphx::shape b_shape{migraphx::shape::float_type, {num_dirct, 2 * hidden_size}};
        migraphx::shape s_shape{migraphx::shape::int32_type, {batch_size}};
        migraphx::shape ih_shape{migraphx::shape::float_type, {num_dirct, batch_size, hidden_size}};

        auto seq_orig = mm->add_parameter("seq", in_shape);
        auto w        = mm->add_parameter("w", w_shape);
        auto r        = mm->add_parameter("r", r_shape);
        auto bias     = mm->add_parameter("bias", b_shape);
        migraphx::shape pad_s{migraphx::shape::float_type, {2, batch_size, input_size}};
        std::vector<float> pad_data(pad_s.elements(), 0.0f);
        auto seq_pad = mm->add_literal(migraphx::literal{pad_s, pad_data});
        auto seq =
            mm->add_instruction(migraphx::make_op("concat", {{"axis", 0}}), seq_orig, seq_pad);
        std::vector<int> sl_data(batch_size, static_cast<int>(seq_len));
        auto sql = mm->add_literal(migraphx::literal{s_shape, sl_data});
        auto ih  = mm->add_parameter("ih", ih_shape);

        auto hs = mm->add_instruction(
            migraphx::make_op(
                "rnn",
                {{"hidden_size", hidden_size},
                 {"actv_func",
                  migraphx::to_value(std::vector<migraphx::operation>{migraphx::make_op("tanh"),
                                                                      migraphx::make_op("tanh")})},
                 {"direction", migraphx::to_value(migraphx::op::rnn_direction::forward)},
                 {"clip", clip}}),
            seq,
            w,
            r,
            bias,
            sql,
            ih);
        auto last_hs = mm->add_instruction(migraphx::make_op("rnn_last_hs_output"), hs);
        mm->add_return({hs, last_hs});

        return p;
    }
    std::string section() const { return "rnn"; }
};
