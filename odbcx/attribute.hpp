// Copyright (c) 2018-2019 Serge Klimov serge.klim@outlook.com

#pragma once

#include "details/attribute.hpp"
#include "details/handle.hpp"
#include "utility"

namespace odbcx { inline namespace v1 { namespace attribute{

static constexpr auto none = None{};

template<SQLINTEGER Id, std::uintptr_t Value> using EnviromentAttribute = handle::Attribute<SQL_HANDLE_ENV, Id, Value>;
template<SQLINTEGER Id, std::uintptr_t Value> using ConnectionAttribute = handle::Attribute<SQL_HANDLE_DBC, Id, Value>;
template<SQLINTEGER Id, std::uintptr_t Value> using StatementAttribute = handle::Attribute<SQL_HANDLE_STMT, Id, Value>;

template<int Version> using OdbcVersion = EnviromentAttribute<SQL_ATTR_ODBC_VERSION, Version>;

static constexpr auto odbc2 = OdbcVersion<SQL_OV_ODBC2>{};
static constexpr auto odbc3 = OdbcVersion<SQL_OV_ODBC3>{};
#if (ODBCVER >= 0x0380)
static constexpr auto odbc3_80 = OdbcVersion<SQL_OV_ODBC3_80>{};
#endif  /* ODBCVER >= 0x0380*/

static constexpr auto cp_off = EnviromentAttribute<SQL_ATTR_CONNECTION_POOLING, SQL_CP_OFF>{};
static constexpr auto cp_one_per_driver = EnviromentAttribute<SQL_ATTR_CONNECTION_POOLING, SQL_CP_ONE_PER_DRIVER>{};
static constexpr auto cp_one_per_henv = EnviromentAttribute<SQL_ATTR_CONNECTION_POOLING, SQL_CP_ONE_PER_HENV>{};
//static constexpr auto cp_driver_aware = EnviromentAttribute<SQL_ATTR_CONNECTION_POOLING, SQL_CP_DRIVER_AWARE>{};

static constexpr auto mode_read_write = ConnectionAttribute<SQL_ATTR_ACCESS_MODE, SQL_MODE_READ_WRITE>{};
static constexpr auto mode_read_only = ConnectionAttribute<SQL_ATTR_ACCESS_MODE, SQL_MODE_READ_ONLY>{};
static constexpr auto mode_default = ConnectionAttribute<SQL_ATTR_ACCESS_MODE, SQL_MODE_DEFAULT>{};

static constexpr auto concur_read_only = StatementAttribute<SQL_ATTR_CONCURRENCY, SQL_CONCUR_READ_ONLY>{};
static constexpr auto concur_lock = StatementAttribute<SQL_ATTR_CONCURRENCY, SQL_CONCUR_LOCK>{};
static constexpr auto concur_rowver = StatementAttribute<SQL_ATTR_CONCURRENCY, SQL_CONCUR_ROWVER>{};
static constexpr auto concur_values = StatementAttribute<SQL_ATTR_CONCURRENCY, SQL_CONCUR_VALUES>{};
static constexpr auto concur_default = StatementAttribute<SQL_ATTR_CONCURRENCY, SQL_CONCUR_DEFAULT>{};

static constexpr auto nonscrollable = StatementAttribute<SQL_ATTR_CURSOR_SCROLLABLE, SQL_NONSCROLLABLE>{};
static constexpr auto scrollable = StatementAttribute<SQL_ATTR_CURSOR_SCROLLABLE, SQL_SCROLLABLE>{};

static constexpr auto cursor_forward_only = StatementAttribute<SQL_ATTR_CURSOR_TYPE, SQL_CURSOR_FORWARD_ONLY>{};
static constexpr auto cursor_keyset_driven = StatementAttribute<SQL_ATTR_CURSOR_TYPE, SQL_CURSOR_KEYSET_DRIVEN>{};
static constexpr auto cursor_dynamic  = StatementAttribute<SQL_ATTR_CURSOR_TYPE, SQL_CURSOR_DYNAMIC>{};
static constexpr auto cursor_static = StatementAttribute<SQL_ATTR_CURSOR_TYPE, SQL_CURSOR_STATIC>{};
static constexpr auto cursor_default = StatementAttribute<SQL_ATTR_CURSOR_TYPE, SQL_CURSOR_TYPE_DEFAULT>{};

//static constexpr auto ub_fixed = StatementAttribute<SQL_ATTR_USE_BOOKMARKS, SQL_UB_FIXED> {};
//static constexpr auto ub_variable = StatementAttribute<SQL_ATTR_USE_BOOKMARKS, SQL_UB_VARIABLE> {};
static constexpr auto bookmarks = StatementAttribute<SQL_ATTR_USE_BOOKMARKS, SQL_UB_VARIABLE>{};


static constexpr auto unspecified = StatementAttribute<SQL_ATTR_CURSOR_SENSITIVITY, SQL_UNSPECIFIED> {};
static constexpr auto insensitive = StatementAttribute<SQL_ATTR_CURSOR_SENSITIVITY, SQL_INSENSITIVE> {};
static constexpr auto sensitive = StatementAttribute<SQL_ATTR_CURSOR_SENSITIVITY, SQL_SENSITIVE> {};


namespace synthetic{

enum class id {BufferSizeHint, Bookmarks, SharedHandle};
template<id ID> using Id = std::integral_constant<id, ID>;
template<id ID, typename Value> using Attribute = odbcx::v1::attribute::Attribute<Id<ID>, Value>;

namespace value{

template<std::size_t N> struct BufferSizeHintRecords : std::integral_constant<std::size_t, N> {};
template<std::size_t N> struct BufferSizeHintBytes : std::integral_constant<std::size_t, N> {};

} //namespace value

template<std::size_t N> using BufferSizeHintRecords = Attribute<id::BufferSizeHint, value::BufferSizeHintRecords<N>>;
template<std::size_t N> using BufferSizeHintBytes = Attribute<id::BufferSizeHint, value::BufferSizeHintBytes<N>>;

using SharedHandle = Attribute<id::SharedHandle, std::true_type>;

} /*namespace synthetic*/

template<std::size_t N> constexpr synthetic::BufferSizeHintBytes<N> bytes_per_buffer_hint() noexcept { return{}; }
template<std::size_t N> constexpr synthetic::BufferSizeHintRecords<N> records_per_buffer_hint() noexcept { return{}; }

template<handle::Type Type, typename ...Atts, typename ...AdditionalAttributes>
auto set(handle::Handle<Type, Attributes<Atts...>>&& handle, Attributes<AdditionalAttributes...> attributes)
    -> handle::Handle<Type, attribute::Merge<Attributes<Atts...>, Attributes<AdditionalAttributes...>>>
{
    auto res = handle::make(set(handle::Adapter<Type, Attributes<Atts...>>{handle}, attributes));
    handle.release();
    return res;
}


}/*namespace attribute*/ }/*inline namespace v1*/} /*namespace odbcx*/
