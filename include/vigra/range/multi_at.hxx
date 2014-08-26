#ifndef MULTI_AT_HXX_
#define MULTI_AT_HXX_

#include <utility>

namespace multi_at_detail
{

template <unsigned int N>
struct multi_at_impl
{
    template <class Range, class Shape>
    static auto make(Range range, Shape shape) -> decltype(multi_at_impl<N - 1>::make(range.front(), shape))
    {
        range.pop_front(shape[N - 1]);
        return multi_at_impl<N - 1>::make(range.front(), shape);
    }
};
template <>
struct multi_at_impl<0>
{
    template <class Value, class Shape>
    static auto make(Value&& val, Shape&&) -> decltype(std::forward<Value>(val))
    {
        return std::forward<Value>(val);
    }
};

}

template <class Range, class Shape>
auto multi_at(Range range, Shape shape) -> decltype(multi_at_detail::multi_at_impl<Range::dimension>::make(range, shape))
{
    return multi_at_detail::multi_at_impl<Range::dimension>::make(range, shape);
}

#endif
