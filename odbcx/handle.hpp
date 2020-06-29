// Copyright (c) 2018-2019 Serge Klimov serge.klim@outlook.com

#pragma once

#include "utility.hpp"
#include "details/handle.hpp"
#include <stdexcept>

namespace odbcx { inline namespace v1 { namespace handle {

using Env = details::Env<>;
using Dbc = details::Dbc<>;
using Stmt = details::Stmt<>;

namespace adapter {

using Env = details::adapter::Env<>;
using Dbc = details::adapter::Dbc<>;
using Stmt = details::adapter::Stmt<>;

} /*adapter*/

template<Type T, typename ...Atts>
/*constexpr*/ Adapter<T, attribute::Attributes<Atts...>> adapt(SQLHANDLE h, attribute::Attributes<Atts...> attributes = {}) /*noexcept*/
{
    set_attributes<T>(Adapter<T>{ h }, attributes);
    return Adapter<T, attribute::Attributes<Atts...>>{h};
}

template<Type T, typename Attributes = attribute::None>
Handle<T, Attributes> make(SQLHANDLE h) noexcept { return{ h, details::Deleter<T, Attributes>{} }; }

template<Type T, typename ...Atts>
Handle<T, attribute::Attributes<Atts...>> make(Adapter<T, attribute::Attributes<Atts...>> adapter)  noexcept
{
    return make<T, attribute::Attributes<Atts...>>(details::sqlhandle(adapter));
}

template<Type T, typename Attributes = attribute::None>
constexpr Adapter<T, Attributes> cast(SQLHANDLE h) noexcept { return Adapter<T, Attributes>{ h }; }

template<Type T, typename ...Attributes>
constexpr Adapter<T, attribute::Attributes<Attributes...>> cast(SQLHANDLE h, attribute::Attributes<Attributes...>) noexcept { return Adapter<T, attribute::Attributes<Attributes...>>{ h };}

template<Type T, typename ...Attributes>
Handle<T, attribute::Attributes<Attributes...>> allocate(attribute::Attributes<Attributes...> attributes = {})
{
    static_assert(T == SQL_HANDLE_ENV, "Only SQL_HANDLE_ENV can be allocated without input handle");
    SQLHANDLE handle;
    if (SQLAllocHandle(T, SQL_NULL_HANDLE, &handle) != SQL_SUCCESS)
        throw std::runtime_error("FATAL DB ERROR: Unable to allocate DB environment handle.");
    return make(adapt<T>(handle, attributes));
}

template<Type T, Type InputType, typename ...Attributes, typename ...AdditionalAttributes>
auto allocate(Adapter<InputType, attribute::Attributes<Attributes...>> input, attribute::Attributes<AdditionalAttributes...> attributes = {})
    -> Handle<T, attribute::Merge<attribute::Attributes<Attributes...>, attribute::Attributes<AdditionalAttributes...>>>
{
    SQLHANDLE handle;
    if_failed_throw<InputType>(SQLAllocHandle(T, details::sqlhandle(input), &handle), handle::details::sqlhandle(input));
    return make(attribute::set(cast<T, attribute::Attributes<Attributes...>>(handle), attributes));
}

template<Type T, Type InputType, typename ...Attributes, typename ...AdditionalAttributes>
auto allocate(Handle<InputType, attribute::Attributes<Attributes...>> const& input, attribute::Attributes<AdditionalAttributes...> attributes = {})
    -> Handle<T, attribute::Merge<attribute::Attributes<Attributes...>, attribute::Attributes<AdditionalAttributes...>>>
{
    return allocate<T>(Adapter<InputType, attribute::Attributes<Attributes...>>{input}, attributes);
}

} /*handle*/} /*inline namespace v1*/} //namespace odbcx
