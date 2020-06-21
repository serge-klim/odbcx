// Copyright (c) 2018-2019 Serge Klimov serge.klim@outlook.com

#pragma once
#include "ttraits.hpp"
#include "odbcx/utility.hpp"
#include "odbcx/handle.hpp"
#include "odbcx/details/diversion.hpp"
#include <boost/mp11/tuple.hpp>
#include <boost/mp11/algorithm.hpp>
#include <boost/mp11/function.hpp>
#include <boost/mp11/integral.hpp>
#include <type_traits>
#include <tuple>
#include <string>
#include <vector>
#include <cstdint>
#include <cstddef>

namespace odbcx { inline namespace v1 { namespace details { namespace in {

template<typename T, typename Enabled = boost::mp11::mp_true> struct Bind;

struct BindNull
{
	void operator()(handle::Stmt const& stmt, SQLUSMALLINT column, SQLSMALLINT c_type, SQLSMALLINT sql_type, SQLLEN& length)
	{
		length = SQL_NULL_DATA;
		odbcx::call(&SQLBindParameter, stmt, column, SQL_PARAM_INPUT, c_type, sql_type, 1, 0, nullptr, 0, &length);
	}
};

template<typename T> 
struct Bind<T, typename boost::mp11::mp_or<std::is_integral<T>, std::is_floating_point<T>>::type>
{
	void operator()(handle::Stmt const & stmt, SQLUSMALLINT column, T const& data, SQLLEN& /*len*/)
	{
		odbcx::call(&SQLBindParameter, stmt, column, SQL_PARAM_INPUT, CType<T>::value, SQLType<T>::value, 0, 0, const_cast<T*>(&data), 0, nullptr);
	}

	void operator()(handle::Stmt const & stmt, SQLUSMALLINT column, std::nullptr_t, SQLLEN& length)
	{
		BindNull{}(stmt, column, CType<T>::value, SQLType<T>::value, length);
	}
};

template<typename T>
struct Bind<T, boost::mp11::mp_bool<std::is_enum<T>::value>>
{
    using UnderlyingType = typename std::underlying_type<T>::type;

    void operator()(handle::Stmt const & stmt, SQLUSMALLINT column, T const& data, SQLLEN& /*len*/)
    {
        odbcx::call(&SQLBindParameter, stmt, column, SQL_PARAM_INPUT, CType<UnderlyingType>::value, SQLType<UnderlyingType>::value, 
            0, 0, const_cast<UnderlyingType*>(reinterpret_cast<UnderlyingType const*>(&data)), 0, nullptr);
    }

    void operator()(handle::Stmt const & stmt, SQLUSMALLINT column, std::nullptr_t, SQLLEN& length)
    {
        BindNull{}(stmt, column, CType<UnderlyingType>::value, SQLType<UnderlyingType>::value, length);
    }
};

template<>
struct Bind<SQL_TIMESTAMP_STRUCT, boost::mp11::mp_true>
{
	void operator()(handle::Stmt const & stmt, SQLUSMALLINT column, SQL_TIMESTAMP_STRUCT const& data, SQLLEN& length)
	{
		length = sizeof(SQL_TIMESTAMP_STRUCT);
		call(&SQLBindParameter, stmt, column, SQL_PARAM_INPUT, SQL_C_TYPE_TIMESTAMP, SQL_TIMESTAMP, 27, 7, const_cast<SQL_TIMESTAMP_STRUCT*>(&data), 0, &length);
	}

	void operator()(handle::Stmt const & stmt, SQLUSMALLINT column, std::nullptr_t, SQLLEN& length)
	{
		length = SQL_NULL_DATA;
		call(&SQLBindParameter, stmt, column, SQL_PARAM_INPUT, SQL_C_TYPE_TIMESTAMP, SQL_TIMESTAMP, 19, 0, nullptr, 0, &length);
	}
};


template<std::size_t N>
struct Bind<char[N], boost::mp11::mp_true>
{
	void operator()(handle::Stmt const& stmt, SQLUSMALLINT column, char const (&data)[N], SQLLEN& length)
	{
		auto end = data + N;
		auto i = std::find(data, end, '\0');
		length = i == end ? N : std::distance(data, i);
        odbcx::call(&SQLBindParameter, stmt, column, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, length == 0 ? 1 : length, 0, const_cast<char*>(data), length, &length);
		//if (length == 0)
		//	operator()(stmt, column, nullptr, length);
		//else
		//	odbcx::call(&SQLBindParameter, stmt, column, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, length, 0, const_cast<char*>(data), length, &length);
	}

	void operator()(handle::Stmt const & stmt, SQLUSMALLINT column, std::nullptr_t, SQLLEN& length)
	{
		BindNull{}(stmt, column, SQL_C_CHAR, SQL_CHAR, length);
	}
};



struct BindCString
{
	void operator()(handle::Stmt const & stmt, SQLUSMALLINT column, const char* data, SQLLEN& length)
	{
		length = std::char_traits<char>::length(data);
        call(&SQLBindParameter, stmt, column, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, length == 0 ? 1 : length, 0, const_cast<char*>(data), length, &length);
		//if (length == 0)
		//	operator()(stmt, column, nullptr, length);
		//else
		//	call(&SQLBindParameter, stmt, column, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, length, 0, const_cast<char*>(data), length, &length);
	}

	void operator()(handle::Stmt const & stmt, SQLUSMALLINT column, std::nullptr_t, SQLLEN& length)
	{
		BindNull{}(stmt, column, SQL_C_CHAR, SQL_CHAR, length);
	}
};

template<> struct Bind<char*, boost::mp11::mp_true> : BindCString {};
template<> struct Bind<char const*, boost::mp11::mp_true> : BindCString {};


template<>
struct Bind<std::string, boost::mp11::mp_true>
{
	void operator()(handle::Stmt const & stmt, SQLUSMALLINT column, std::string const &data, SQLLEN& length)
	{
		length = data.length();
        odbcx::call(&SQLBindParameter, stmt, column, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, length == 0 ? 1 : length, 0, const_cast<char*>(data.data()), length, &length);
		//if (length == 0)
		//	operator()(stmt, column, nullptr, length);
		//else
		//	odbcx::call(&SQLBindParameter, stmt, column, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, length, 0, const_cast<char*>(data.data()), length, &length);
	}

	void operator()(handle::Stmt const & stmt, SQLUSMALLINT column, std::nullptr_t, SQLLEN& length)
	{
		BindNull{}(stmt, column, SQL_C_CHAR, SQL_CHAR, length);
	}
};

template<>
struct Bind<std::vector<char>, boost::mp11::mp_true>
{
	void operator()(handle::Stmt const & stmt, SQLUSMALLINT column, std::string const &data, SQLLEN& length)
	{
		length = data.size();
        odbcx::call(&SQLBindParameter, stmt, column, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, length == 0 ? 1 : length, 0, const_cast<char*>(data.data()), length, &length);
		//if (length == 0)
		//	operator()(stmt, column, nullptr, length);
		//else
		//	odbcx::call(&SQLBindParameter, stmt, column, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, length, 0, const_cast<char*>(data.data()), length, &length);
	}

	void operator()(handle::Stmt const & stmt, SQLUSMALLINT column, std::nullptr_t, SQLLEN& length)
	{
		BindNull{}(stmt, column, SQL_C_CHAR, SQL_CHAR, length);
	}

};

template<typename T>
struct Bind<std::vector<T>, boost::mp11::mp_bool<sizeof(T) == sizeof(std::uint8_t)>>
{
	void operator()(handle::Stmt const & stmt, SQLUSMALLINT column, std::vector<T> const &data, SQLLEN& length)
	{
		length = data.size();
		//if (length == 0)
		//	operator()(stmt, column, nullptr, length);
		//else
		//{
		//	SQLSMALLINT sqlType = (std::size_t(length) > max_bindable_data_size ? SQL_LONGVARBINARY : SQL_BINARY);
		//	odbcx::call(&SQLBindParameter, stmt, column, SQL_PARAM_INPUT, SQL_C_BINARY, sqlType, length, 0, const_cast<T*>(data.data()), length, &length);
		//}
        SQLSMALLINT sqlType = (std::size_t(length) > max_bindable_data_size ? SQL_LONGVARBINARY : SQL_BINARY);
        odbcx::call(&SQLBindParameter, stmt, column, SQL_PARAM_INPUT, SQL_C_BINARY, sqlType, length == 0 ? 1 : length, 0, const_cast<T*>(data.data()), length, &length);
	}

	void operator()(handle::Stmt const & stmt, SQLUSMALLINT column, std::nullptr_t, SQLLEN& length)
	{
		BindNull{}(stmt, column, SQL_C_BINARY, SQL_BINARY, length);
	}
};

template<typename T>
struct Bind<diversion::optional<T>, boost::mp11::mp_true>
{
	void operator()(handle::Stmt const & stmt, SQLUSMALLINT column, diversion::optional<T> const& value, SQLLEN& length)
	{
		if (!value)
			Bind<T>{}(stmt, column, nullptr, length);
		else
			Bind<T>{}(stmt, column, value.value(), length);
	}
};

//data at exec
//https://docs.microsoft.com/en-us/sql/relational-databases/native-client-odbc-table-valued-parameters/sending-data-as-a-table-valued-parameter-using-data-at-execution-odbc?view=sql-server-2017

template<typename ...Params>
struct InputParametersVerify;

template<typename T, typename ...Params>
struct InputParametersVerify<T const&, Params...> : InputParametersVerify<Params...> {};

template<typename T, typename ...Params>
struct InputParametersVerify<T&, Params...> : InputParametersVerify<Params...> {};

template<typename T, typename ...Params>
struct InputParametersVerify<T&&, Params...> : std::false_type {};

template<typename T, typename ...Params>
struct InputParametersVerify<T, Params...> : std::false_type {};

template<>
struct InputParametersVerify<> : std::true_type {};


class InputParameters
{
    struct BindParam
    {
        template<typename T>
        void operator()(T const& param)
        {
            Bind<typename std::decay<T>::type>{}(stmt, n+1, param, lengths[n]);
            ++n;
        }
        handle::Stmt const& stmt;
        SQLLEN* lengths;
        SQLUSMALLINT& n;
    };
public:
    template<typename ...Params>
    void bind(SQLLEN(&lengths)[sizeof...(Params)], handle::Stmt const& stmt, std::tuple<Params...> const& params) const
    {
        SQLUSMALLINT n = 0;
        boost::mp11::tuple_for_each(params, BindParam{stmt,lengths, n});
    }

	template<typename ...Params>
	void bind(SQLLEN (&lengths)[sizeof...(Params)], handle::Stmt const& stmt, Params const& ...params) const
	{
        bind<Params const&...>(lengths, stmt, std::forward_as_tuple(params...));
		//bind_<0>(stmt, lengths, std::forward<Params>(params)...);
	}
//private:
//	template<SQLUSMALLINT N, typename T, typename ...Params>
//	void bind_(handle::Stmt const& stmt, SQLLEN* lengths, T const& param, Params&& ...params) const
//	{
//		Bind<typename std::decay<T>::type>{}(stmt, N + 1, param, lengths[N]);
//		bind_<N+1>(stmt, lengths, std::forward<Params>(params)...);
//	}
//
//	template<SQLUSMALLINT N>
//	void bind_(handle::Stmt const& /*stmt*/, SQLLEN* /*lengths*/) const {}
};

}/*namespace in*/ }  /*namespace details*/ }/*inline namespace v1*/} //namespace odbcx
