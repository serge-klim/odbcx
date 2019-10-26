#pragma once
#include <boost/fusion/include/std_tuple.hpp>
#include <boost/fusion/include/std_pair.hpp>
#include <boost/fusion/include/is_sequence.hpp>
#include <type_traits>

namespace odbcx { inline namespace v0 { namespace details {

template<typename ...Params>
struct MakeSequence {  using type = std::tuple<Params...>; };

template<typename T>
struct MakeSequence<T> { using type = typename std::conditional<boost::fusion::traits::is_sequence<T>::value, T, std::tuple<T>>::type; };

}/*namespace details*/ }/*inline namespace v0*/} //namespace odbcx

