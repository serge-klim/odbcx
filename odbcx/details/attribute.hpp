// Copyright (c) 2018-2019 Serge Klimov serge.klim@outlook.com

#pragma once

#include <boost/mp11/algorithm.hpp>
#include <boost/mp11/map.hpp>
#include <boost/mp11/list.hpp>
#include <boost/mp11/integral.hpp>
#include <type_traits>

namespace odbcx { inline namespace v1 { namespace attribute{

namespace details 
{ 
template<typename Id, typename Value> using Attribute = boost::mp11::mp_list<Id, Value>; 
template <typename Left, typename Right> struct Merge;
}/*details*/

template<typename ...Atts> struct Attributes 
{
    template<typename ...More>
    constexpr auto operator | (Attributes<More...>) const noexcept
        -> typename details::Merge<Attributes<Atts...>, Attributes<More...>>::type
    {
        return {};
    }
};

using None = Attributes<>;
template<typename Id, typename Value, typename ...More> using Attribute = Attributes<details::Attribute<Id, Value>, More...>;

namespace details {

template<typename Attr> struct Id;
template<typename ID, typename Val> struct Id<Attribute<ID, Val>>{using type = Id;};
template<typename ID, typename Val, typename ...More> struct Id<odbcx::v1::attribute::Attribute<ID, Val, More...>> { using type = Id; };

template<typename Attr> struct Value { using type = void; };
template<typename ID, typename Val> struct Value<Attribute<ID, Val>> { using type = Val; };
template<typename ID, typename Val, typename ...More> struct Value<odbcx::v1::attribute::Attribute<ID, Val, More...>> { using type = Val; };

template <typename ...Left, typename ...Right>
struct Merge<Attributes<Left...>, Attributes<Right...>>
{
    using type = boost::mp11::mp_fold<Attributes<Right...>, Attributes<Left...>, boost::mp11::mp_map_replace>;
};

template <typename Set, typename Attribute> struct Contains;
template <typename ...Atts, typename Id, typename Value> struct Contains<Attributes<Atts...>, odbcx::v1::attribute::Attribute<Id, Value>> : boost::mp11::mp_contains<Attributes<Atts...>, Attribute<Id, Value>> {};

template <typename Left, typename Right> struct IsSubset;
template <typename ...Left, typename ...Right>
struct IsSubset<Attributes<Left...>, Attributes<Right...>>
{
    template<typename T> using Pred = boost::mp11::mp_not<boost::mp11::mp_contains< Attributes<Right...>, T>>;
    using type = typename std::is_same<boost::mp11::mp_find_if< Attributes<Left...>, Pred>, boost::mp11::mp_size_t<sizeof...(Left)>>::type;
};

template <typename Atts, typename Id> struct Remove;
template <typename ...Atts, typename Id> struct Remove<Attributes<Atts...>, Id>{ using type = boost::mp11::mp_map_erase<Attributes<Atts...>, Id>; };

template<typename Set, typename Id> struct ValueOf;
template <typename ...Atts, typename Id>
struct ValueOf<Attributes<Atts...>, Id>
{
    using type = typename details::Value<boost::mp11::mp_map_find<Attributes<Atts...>, Id>>::type;
};

} /*details*/

template <typename Left, typename Right> using Merge = typename details::Merge<typename std::remove_cv<Left>::type, typename std::remove_cv<Right>::type>::type;
template <typename Set, typename Attribute> using Contains = details::Contains<typename std::remove_cv<Set>::type, typename std::remove_cv<Attribute>::type>;
template <typename Left, typename Right> using IsSubset = typename details::IsSubset<typename std::remove_cv<Left>::type, typename std::remove_cv<Right>::type>::type;
template <typename Attributes, typename Id> using Remove = typename details::Remove<typename std::remove_cv<Attributes>::type, typename std::remove_cv<Id>::type>::type;
template <typename Attributes, typename Id> using ValueOf = typename details::ValueOf<typename std::remove_cv<Attributes>::type, typename std::remove_cv<Id>::type>::type;

 }/*attribute*/}/*inline namespace v1*/} /*namespace odbcx*/



