#include <migraphx/gpu/acos.hpp>
#include <migraphx/operators.hpp>
#include <migraphx/manage_ptr.hpp>
#include <migraphx/config.hpp>
#include <migraphx/gpu/miopen.hpp>
#include <utility>

namespace migraphx {
inline namespace MIGRAPH_INLINE_NS {
namespace gpu {

shape hip_acos::compute_shape(const std::vector<shape>& inputs) const
{
    check_shapes{inputs, *this}.has(2);
    return inputs.at(0);
}

argument hip_acos::compute(context& ctx, const shape&, const std::vector<argument>& args) const
{
    device::acos(ctx.get_stream().get(), args[1], args[0]);
    return args[1];
}

} // namespace gpu
} // namespace MIGRAPH_INLINE_NS
} // namespace migraphx
