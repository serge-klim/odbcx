// Copyright (c) 2018-2019 Serge Klimov serge.klim@outlook.com

#pragma once

#include "odbc.hpp"
#include "attribute.hpp"
#include <memory>
#include <type_traits>
#include <cstdint>

namespace odbcx { inline namespace v1 { 
    
template<typename F, typename Handle, typename ...Args> SQLRETURN call(F* f, Handle const& handle, Args&& ...args);

namespace handle{

using Type = SQLSMALLINT;

template<Type T, SQLINTEGER ID> struct AttributeId;
template<std::uintptr_t Val> using AttributeValue = std::integral_constant<std::uintptr_t, Val>;
template<Type T, SQLINTEGER ID, std::uintptr_t Val> using Attribute = odbcx::v1::attribute::Attribute<AttributeId<T, ID>, AttributeValue<Val>>;


template<Type T, typename Attributes = odbcx::attribute::None> struct Adapter;

namespace details {

template<Type T, typename Attributes = odbcx::attribute::None> struct Deleter
{
    Deleter() = default;
    Deleter& operator= (Deleter const&) noexcept = default;
    Deleter& operator= (Deleter&&) noexcept = default;

    template<typename OtherAttributes, typename Ok = typename std::enable_if<attribute::IsSubset<Attributes, OtherAttributes>::value, int>::type >
    constexpr Deleter(Deleter<T, OtherAttributes> const&) noexcept {}
    template<typename OtherAttributes, typename Ok = typename std::enable_if<attribute::IsSubset<Attributes, OtherAttributes>::value, int>::type >
    constexpr Deleter(Deleter<T, OtherAttributes>&&) noexcept {}

    template<typename OtherAttributes, typename Ok = typename std::enable_if<attribute::IsSubset<Attributes, OtherAttributes>::value, int>::type >
    Deleter& operator= (Deleter<T, OtherAttributes> const&) noexcept { return *this; }
    template<typename OtherAttributes, typename Ok = typename std::enable_if<attribute::IsSubset<Attributes, OtherAttributes>::value, int>::type >
    Deleter& operator= (Deleter<T, OtherAttributes>&&) noexcept { return *this; }

    void operator()(SQLHANDLE handle) { SQLFreeHandle(T, handle); }
};

template<typename ...Atts>
struct Deleter<SQL_HANDLE_DBC, attribute::Attributes<Atts...>>
{
    Deleter() = default;
    Deleter& operator= (Deleter const&) noexcept = default;
    Deleter& operator= (Deleter&&) noexcept = default;

    template<typename OtherAttributes, typename Ok = typename std::enable_if<attribute::IsSubset<attribute::Attributes<Atts...>, OtherAttributes>::value, int>::type >
    constexpr Deleter(Deleter<SQL_HANDLE_DBC, OtherAttributes> const&) noexcept {}
    template<typename OtherAttributes, typename Ok = typename std::enable_if<attribute::IsSubset<attribute::Attributes<Atts...>, OtherAttributes>::value, int>::type >
    constexpr Deleter(Deleter<SQL_HANDLE_DBC, OtherAttributes>&&) noexcept {}

    template<typename OtherAttributes, typename Ok = typename std::enable_if<attribute::IsSubset<attribute::Attributes<Atts...>, OtherAttributes>::value, int>::type >
    Deleter& operator= (Deleter<SQL_HANDLE_DBC, OtherAttributes> const&) noexcept { return *this; }
    template<typename OtherAttributes, typename Ok = typename std::enable_if<attribute::IsSubset<attribute::Attributes<Atts...>, OtherAttributes>::value, int>::type >
    Deleter& operator= (Deleter<SQL_HANDLE_DBC, OtherAttributes>&&) noexcept { return *this; }

    void operator()(SQLHANDLE handle)
    {
        SQLDisconnect(handle);
        SQLFreeHandle(SQL_HANDLE_DBC, handle);
    }
};

//template<typename ...Atts>
//struct Deleter<SQL_HANDLE_STMT, attribute::Attributes<Atts...>>
//{
//    Deleter() = default;
//    Deleter& operator= (Deleter const&) noexcept = default;
//    Deleter& operator= (Deleter&&) noexcept = default;
//
//    template<typename OtherAttributes, typename Ok = typename std::enable_if<attribute::IsSubset<attribute::Attributes<Atts...>, OtherAttributes>::value, int>::type >
//    constexpr Deleter(Deleter<SQL_HANDLE_STMT, OtherAttributes> const&) noexcept {}
//    template<typename OtherAttributes, typename Ok = typename std::enable_if<attribute::IsSubset<attribute::Attributes<Atts...>, OtherAttributes>::value, int>::type >
//    constexpr Deleter(Deleter<SQL_HANDLE_STMT, OtherAttributes>&&) noexcept {}
//
//    template<typename OtherAttributes, typename Ok = typename std::enable_if<attribute::IsSubset<attribute::Attributes<Atts...>, OtherAttributes>::value, int>::type >
//    Deleter& operator= (Deleter<SQL_HANDLE_STMT, OtherAttributes> const&) noexcept { return *this; }
//    template<typename OtherAttributes, typename Ok = typename std::enable_if<attribute::IsSubset<attribute::Attributes<Atts...>, OtherAttributes>::value, int>::type >
//    Deleter& operator= (Deleter<SQL_HANDLE_STMT, OtherAttributes>&&) noexcept { return *this; }
//
//    void operator()(SQLHANDLE handle) 
//    { 
//        SQLFreeStmt(handle, SQL_CLOSE);
//        SQLFreeStmt(handle, SQL_UNBIND);
//        SQLFreeStmt(handle, SQL_RESET_PARAMS);
//        SQLFreeStmt(handle, SQL_DROP); // some drivers still needs it e.g oracle 12.02.00.01
//        SQLFreeHandle(SQL_HANDLE_STMT, handle);
//    }
//};
//


} /*details*/

template<Type T, typename Attributes = odbcx::attribute::None>
using Handle = std::unique_ptr<typename std::remove_pointer<SQLHANDLE>::type, details::Deleter<T, Attributes>>;


namespace details {
template<Type T, typename ...Atts>
constexpr SQLHANDLE sqlhandle(Handle<T, attribute::Attributes<Atts...>>const& h) noexcept { return h.get(); }
template<Type T, typename ...Atts> constexpr SQLHANDLE sqlhandle(Adapter<T, attribute::Attributes<Atts...>> const& h) noexcept;
} /*details*/

template<Type T, typename Attributes /*= odbcx::attribute::None*/>
struct Adapter
{
    //Adapter() = default;
    explicit constexpr Adapter(SQLHANDLE h) noexcept : h{ h } {}
    template<typename OtherAttributes, typename Ok = typename std::enable_if<attribute::IsSubset<Attributes, OtherAttributes>::value, int>::type >
    Adapter(Adapter<T, OtherAttributes> const& h) noexcept : h{ details::sqlhandle(h) } {}
    Adapter(Handle<T, Attributes> const& h) noexcept : h{ details::sqlhandle(h) } {}
    template<typename OtherAttributes, typename Ok = typename std::enable_if<attribute::IsSubset<Attributes, OtherAttributes>::value, int>::type >
    Adapter(Handle<T, OtherAttributes> const& h) noexcept : h{ details::sqlhandle(h) } {}


    //Adapter& operator= (Adapter const&) noexcept = default;
    //Adapter& operator= (Adapter&&) noexcept = default;

    //template<typename OtherAttributes, typename Ok = typename std::enable_if<attribute::IsSubset<Attributes, OtherAttributes>::value, int>::type >
    //constexpr Adapter(Adapter<T, OtherAttributes> const& other) noexcept : h { other.h } {}
    //template<typename OtherAttributes, typename Ok = typename std::enable_if<attribute::IsSubset<Attributes, OtherAttributes>::value, int>::type >
    //constexpr Adapter(Adapter<T, OtherAttributes>&& other) noexcept : h { other.h } {}

    //template<typename OtherAttributes, typename Ok = typename std::enable_if<attribute::IsSubset<Attributes, OtherAttributes>::value, int>::type >
    //Adapter& operator= (Adapter<T, OtherAttributes> const& other) noexcept { h = other.h; return *this; }
    //template<typename OtherAttributes, typename Ok = typename std::enable_if<attribute::IsSubset<Attributes, OtherAttributes>::value, int>::type >
    //Adapter& operator= (Adapter<T, OtherAttributes>&& other) noexcept { h = other.h; return *this; }

private:
    template<Type Tp, typename ...Atts>
    friend constexpr SQLHANDLE details::sqlhandle(Adapter<Tp, attribute::Attributes<Atts...>>const& h) noexcept;

    //friend constexpr SQLHANDLE sqlhandle<>(Adapter<T, Attributes>const& h) noexcept;
    //friend constexpr SQLHANDLE sqlhandle<>(Adapter<T, Attributes>& h) noexcept;
    SQLHANDLE h;
};


template<typename T> struct AttachedType;
template<Type T, typename ...Atts> struct AttachedType< Adapter<T, attribute::Attributes<Atts...>>> : std::integral_constant<Type, T> {};
template<Type T, typename ...Atts> struct AttachedType< Handle<T, attribute::Attributes<Atts...>>> : std::integral_constant<Type, T> {};
template<typename T> struct AttachedAttributes;
template<Type T, typename ...Atts> struct AttachedAttributes< Adapter<T, attribute::Attributes<Atts...>>> { using type = attribute::Attributes<Atts...>; };
template<Type T, typename ...Atts> struct AttachedAttributes< Handle<T, attribute::Attributes<Atts...>>> { using type = attribute::Attributes<Atts...>; };
template<typename Handle> using IsEnv = std::integral_constant<bool, AttachedType<Handle>::value == SQL_HANDLE_ENV>;
template<typename Handle> using IsDbc = std::integral_constant<bool, AttachedType<Handle>::value == SQL_HANDLE_DBC>;
template<typename Handle> using IsStmt = std::integral_constant<bool, AttachedType<Handle>::value == SQL_HANDLE_STMT>;
template<typename Handle> using IsDesc = std::integral_constant<bool, AttachedType<Handle>::value == SQL_HANDLE_DESC>;


namespace details {

template<Type T, typename ...Atts>
constexpr SQLHANDLE sqlhandle(Adapter<T, attribute::Attributes<Atts...>>const& h) noexcept { return h.h; }

template<typename Attributes = attribute::None> using Env = Handle<SQL_HANDLE_ENV, Attributes>;
template<typename Attributes = attribute::None> using Dbc = Handle<SQL_HANDLE_DBC, Attributes>;
template<typename Attributes = attribute::None> using Stmt = Handle<SQL_HANDLE_STMT, Attributes>;

namespace adapter {

template<typename Attributes = attribute::None> using Env = Adapter<SQL_HANDLE_ENV, Attributes>;
template<typename Attributes = attribute::None> using Dbc = Adapter<SQL_HANDLE_DBC, Attributes>;
template<typename Attributes = attribute::None> using Stmt = Adapter<SQL_HANDLE_STMT, Attributes>;

} /*adapter*/
}/*details*/

inline SQLRETURN get_attribute(details::adapter::Env<> const& adapter, SQLINTEGER id, SQLPOINTER value, SQLINTEGER& size)
{
    SQLINTEGER length = size;
    auto res = call(&SQLGetEnvAttr, adapter, id, value, size, &length);
    length = size;
    return res;
}

inline SQLRETURN get_attribute(details::adapter::Dbc<> const& adapter, SQLINTEGER id, SQLPOINTER value, SQLINTEGER& size)
{
    SQLINTEGER length = size;
    auto res = call(&SQLGetConnectAttr, adapter, id, value, size, &length);
    length = size;
    return res;
}

inline SQLRETURN get_attribute(details::adapter::Stmt<> const& adapter, SQLINTEGER id, SQLPOINTER value, SQLINTEGER& size)
{
    SQLINTEGER length = size;
    auto res = call(&SQLGetStmtAttr, adapter, id, value, size, &length);
    length = size;
    return res;
}


template<typename T, typename Handle>
inline auto get_attribute(Handle const& handle, SQLINTEGER id)
    -> typename std::enable_if<std::is_integral<T>::value, T>::type
{
    T res = 0;
    SQLINTEGER size = sizeof(T);
    handle::get_attribute(handle, id, &res, size);
    assert(std::size_t(size) <= sizeof(T));
    return res;
}

inline SQLRETURN set_attribute(details::adapter::Env<> const& adapter, SQLINTEGER id, SQLPOINTER value, SQLINTEGER size = 0)
{
    return call(&SQLSetEnvAttr, adapter, id, value, size);
}

inline SQLRETURN set_attribute(details::adapter::Dbc<> const& adapter, SQLINTEGER id, SQLPOINTER value, SQLINTEGER size = 0)
{
    return call(&SQLSetConnectAttr, adapter, id, value, size);
}

inline SQLRETURN set_attribute(details::adapter::Stmt<> const& adapter, SQLINTEGER id, SQLPOINTER value, SQLINTEGER size = 0)
{
    return call(&SQLSetStmtAttr, adapter, id, value, size);
}

template<Type T, typename ...Atts> /*constexpr*/ void set_attributes(Adapter<T, attribute::Attributes<Atts...>> const&, attribute::Attributes<>) noexcept {}
template<Type T, typename ...Atts, typename AdditionalAttribute>
/*constexpr*/ void set_attribute(Adapter<T, attribute::Attributes<Atts...>>const&, attribute::Attributes<AdditionalAttribute>) noexcept {}

template<handle::Type T, typename ...Atts, handle::Type AttributeType, SQLINTEGER AttributeId, std::uintptr_t AttributeValue>
void set_attribute(Adapter<T, attribute::Attributes<Atts...>> const& adapter, Attribute<AttributeType, AttributeId, AttributeValue>)
{
    static_assert(T == AttributeType, "inapplicable attribute");
    set_attribute(adapter, AttributeId, SQLPOINTER(AttributeValue));
}

template<handle::Type T, typename ...Atts, typename First, typename ...Rest>
void set_attributes(Adapter<T, attribute::Attributes<Atts...>> const& adapter, attribute::Attributes<First, Rest...>)
{
    set_attribute(adapter, attribute::Attributes<First>{});
    set_attributes(adapter, attribute::Attributes<Rest...>{});
}


template<typename Handle, typename Value>
auto set_attribute(Handle const& handle, SQLINTEGER id, Value value)
            -> typename std::enable_if<std::is_integral<Value>::value>::type
{
    set_attribute(handle, id, reinterpret_cast<SQLPOINTER>(value));
}

} /*handle*/


namespace attribute{

template<handle::Type Type, typename ...Atts, typename ...AdditionalAttributes>
auto set(handle::Adapter<Type, Attributes<Atts...>>&& adapter, Attributes<AdditionalAttributes...>)
    -> handle::Adapter<Type, attribute::Merge<Attributes<Atts...>, Attributes<AdditionalAttributes...>>>
{
    handle::set_attributes(adapter, Attributes<AdditionalAttributes...>{});
    return handle::Adapter<Type, attribute::Merge<Attributes<Atts...>, Attributes<AdditionalAttributes...>>>{handle::details::sqlhandle(adapter)};
}

} /*namespace attribute */

 } /*inline namespace v1*/} /*namespace odbcx*/



