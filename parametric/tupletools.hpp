#ifndef TUPLETOOLS_HPP
#define TUPLETOOLS_HPP

#include <utility>
#include <tuple>

namespace TupleTools_private
{
    //! helper template function for the actual implementation of a compile-time for loop
    template<typename GenericLambda, std::size_t ... Is>
    constexpr void static_for_impl(GenericLambda&& f, std::index_sequence<Is...>)
    {
        // unpack into std::initializer list for "looping" in correct order without recursion
        (void)std::initializer_list<char> {((void)f(std::integral_constant<unsigned, Is>()), '0')...};
    }

    template <class Tuple, class F, typename ElemFun, std::size_t... Is>
    constexpr auto apply_impl(F f, Tuple t, ElemFun ef,
                              std::index_sequence<Is...>)
    {
        return f(ef(std::get<Is>(t))...);
    }

} // namespace tupletools private

template<unsigned N, typename GenericLambda>
constexpr void static_for(GenericLambda&& f)
{
    TupleTools_private::static_for_impl(std::forward<GenericLambda>(f), std::make_index_sequence<N>());
}

template<typename... _Elements, typename GenericLambda>
constexpr void static_foreach(std::tuple<_Elements...>& aTuple, GenericLambda&& f)
{
    constexpr auto N = std::tuple_size<std::tuple<_Elements...>>::value;
    static_for<N>([&aTuple, &f](auto i) {
        f(std::get<i>(aTuple));
    });
}

template <class F, class Tuple, typename ElemFun>
constexpr auto apply(F f, Tuple t, ElemFun ef)
{
    return TupleTools_private::apply_impl(
        f, t, ef, std::make_index_sequence < std::tuple_size<Tuple> {} > {});
}


#endif // TUPLETOOLS_HPP
