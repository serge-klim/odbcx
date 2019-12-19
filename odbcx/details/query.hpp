// Copyright (c) 2018-2019 Serge Klimov serge.klim@outlook.com

#pragma once
#include "odbcx/attribute.hpp"
#include "odbcx/bindings/in.hpp"
#include "handle.hpp"
#include "diversion.hpp"
#include <string>
#include <tuple>
#include <type_traits>

namespace odbcx { inline namespace v1 {

template<typename Stmt>
auto query(Stmt const& stmt, std::string const& text, std::tuple<> const& = {})
    -> typename std::enable_if<handle::IsStmt<Stmt>::value>::type
{
	call(&SQLPrepare, stmt, const_cast<SQLCHAR*>(reinterpret_cast<SQLCHAR const*>(text.data())), SQLINTEGER(text.size()));
	call(&SQLExecute, stmt);
}

template<typename Stmt, typename ...Params>
auto query(Stmt const& stmt, std::string const& text, Params&& ...params)
    -> typename std::enable_if<handle::IsStmt<Stmt>::value && sizeof...(params)!=0>::type
{
    assert(std::count(begin(text), end(text), '?') == sizeof...(params) && "hmm! seems parameters number mismatches placeholders!");
    SQLLEN lengths[sizeof...(params)];
    details::in::InputParameters{}.bind(lengths, stmt, std::forward<Params>(params)...);
    query(stmt, text);
}


template<typename Dbc, typename ...Atts, typename ...Params>
auto query(Dbc const& dbc, attribute::Attributes<Atts...> attributes, std::string const& text, Params&& ...params)
    -> typename std::enable_if<handle::IsDbc<Dbc>::value, decltype(odbcx::handle::allocate<SQL_HANDLE_STMT>(dbc, attributes))>::type
{
    auto stmt = odbcx::handle::allocate<SQL_HANDLE_STMT>(dbc, attributes);
    query(stmt, text, std::forward<Params>(params)...);
    return stmt;
}

template<typename Dbc, typename ...Params>
auto query(Dbc const& dbc, std::string const& text, Params&& ...params)
    -> typename std::enable_if<handle::IsDbc<Dbc>::value, decltype(odbcx::handle::allocate<SQL_HANDLE_STMT>(dbc, odbcx::attribute::none))>::type
{
    return query(dbc, odbcx::attribute::none, text, std::forward<Params>(params)...);
}

}/*inline namespace v1*/} //namespace odbcx
