// Copyright (c) 2018-2019 Serge Klimov serge.klim@outlook.com

#pragma once
#include "details/handle.hpp"
#include "details/odbc.hpp"
#include "details/diversion.hpp"

namespace odbcx { inline namespace v1 {

SQLRETURN if_failed_throw(SQLRETURN sqlres, SQLHANDLE handle, SQLSMALLINT type);

template<handle::Type T>
SQLRETURN if_failed_throw(SQLRETURN sqlres, SQLHANDLE handle) { return if_failed_throw(sqlres, handle, T); }

template<typename F, typename Handle, typename ...Args>
SQLRETURN call(F* f, Handle const& handle, Args&& ...args)
{
    return if_failed_throw<handle::AttachedType<Handle>::value>((*f)(handle::details::sqlhandle(handle), std::forward<Args>(args)...), handle::details::sqlhandle(handle));
}

inline SQLRETURN connect(handle::details::adapter::Dbc<> const& dbc, diversion::string_view constring, SQLUSMALLINT driver_completion = SQL_DRIVER_NOPROMPT)
{
    return call(&SQLDriverConnect, dbc, nullptr, reinterpret_cast<SQLCHAR*>(const_cast<char*>(constring.data())), SQLSMALLINT(constring.size()), nullptr, SQLSMALLINT(0), nullptr, driver_completion);
}

inline SQLRETURN disconnect(handle::details::adapter::Dbc<> const& dbc) { return call(&SQLDisconnect, dbc); }


} /*inline namespace v1*/} //namespace odbcx
