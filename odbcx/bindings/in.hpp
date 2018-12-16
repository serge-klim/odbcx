#pragma once
#include "odbcx/details/diversion.hpp"
#include "odbcx/utility.hpp"
#include "ttraits.hpp"
#include <boost/mpl/bool_fwd.hpp>
#include <boost/mpl/not.hpp>
#include <type_traits>
#include <string>
#include <vector>
#include <cstdint>
#include <cstddef>

namespace odbcx { inline namespace v0 { namespace details { namespace in {


template<typename T, typename Enabled = boost::mpl::true_> struct Bind;

struct BindNull
{
	void operator()(handle::Stmt const& stmt, SQLUSMALLINT column, SQLSMALLINT c_type, SQLSMALLINT sql_type, SQLLEN& length)
	{
		length = SQL_NULL_DATA;
		odbcx::call(&SQLBindParameter, stmt, column, SQL_PARAM_INPUT, c_type, sql_type, 1, 0, nullptr, 0, &length);
	}
};

template<typename T> 
struct Bind<T, typename boost::mpl::or_<std::is_integral<T>, std::is_floating_point<T>>::type>
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

template<>
struct Bind<SQL_TIMESTAMP_STRUCT, boost::mpl::true_>
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
struct Bind<char[N], boost::mpl::true_>
{
	void operator()(handle::Stmt const& stmt, SQLUSMALLINT column, char const (&data)[N], SQLLEN& length)
	{
		auto end = data + N;
		auto i = std::find(data, end, '\0');
		length = i == end ? N : std::distance(data, i);
		if (length == 0)
			operator()(stmt, column, nullptr, length);
		else
			odbcx::call(&SQLBindParameter, stmt, column, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, length, 0, const_cast<char*>(data), length, &length);
	}

	void operator()(handle::Stmt const & stmt, SQLUSMALLINT column, std::nullptr_t, SQLLEN& length)
	{
		BindNull{}(stmt, column, SQL_C_CHAR, SQL_CHAR, length);
	}
};

template<>
struct Bind<char*, boost::mpl::true_>
{
	void operator()(handle::Stmt const & stmt, SQLUSMALLINT column, const char* data, SQLLEN& length)
	{
		length = std::char_traits<char>::length(data);
		if (length == 0)
			operator()(stmt, column, nullptr, length);
		else
			call(&SQLBindParameter, stmt, column, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, length, 0, const_cast<char*>(data), length, &length);
	}

	void operator()(handle::Stmt const & stmt, SQLUSMALLINT column, std::nullptr_t, SQLLEN& length)
	{
		BindNull{}(stmt, column, SQL_C_CHAR, SQL_CHAR, length);
	}
};

template<>
struct Bind<std::string, boost::mpl::true_>
{
	void operator()(handle::Stmt const & stmt, SQLUSMALLINT column, std::string const &data, SQLLEN& length)
	{
		length = data.length();
		if (length == 0)
			operator()(stmt, column, nullptr, length);
		else
			odbcx::call(&SQLBindParameter, stmt, column, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, length, 0, const_cast<char*>(data.data()), length, &length);
	}

	void operator()(handle::Stmt const & stmt, SQLUSMALLINT column, std::nullptr_t, SQLLEN& length)
	{
		BindNull{}(stmt, column, SQL_C_CHAR, SQL_CHAR, length);
	}
};

template<>
struct Bind<std::vector<char>, boost::mpl::true_>
{
	void operator()(handle::Stmt const & stmt, SQLUSMALLINT column, std::string const &data, SQLLEN& length)
	{
		length = data.size();
		if (length == 0)
			operator()(stmt, column, nullptr, length);
		else
			odbcx::call(&SQLBindParameter, stmt, column, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, length, 0, const_cast<char*>(data.data()), length, &length);
	}

	void operator()(handle::Stmt const & stmt, SQLUSMALLINT column, std::nullptr_t, SQLLEN& length)
	{
		BindNull{}(stmt, column, SQL_C_CHAR, SQL_CHAR, length);
	}

};

template<typename T>
struct Bind<std::vector<T>, boost::mpl::bool_<sizeof(T) == sizeof(std::uint8_t)>>
{
	void operator()(handle::Stmt const & stmt, SQLUSMALLINT column, std::vector<T> const &data, SQLLEN& length)
	{
		length = data.size();
		if (length == 0)
			operator()(stmt, column, nullptr, length);
		else
		{
			SQLSMALLINT sqlType = (std::size_t(length) > BinarySizeLimit ? SQL_LONGVARBINARY : SQL_BINARY);
			odbcx::call(&SQLBindParameter, stmt, column, SQL_PARAM_INPUT, SQL_C_BINARY, sqlType, length, 0, const_cast<T*>(data.data()), length, &length);
		}
	}

	void operator()(handle::Stmt const & stmt, SQLUSMALLINT column, std::nullptr_t, SQLLEN& length)
	{
		BindNull{}(stmt, column, SQL_C_BINARY, SQL_BINARY, length);
	}
};

template<typename T>
struct Bind<diversion::optional<T>, boost::mpl::true_>
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


struct InputParameters
{
	template<typename ...Params>
	void bind(SQLLEN (&lengths)[sizeof...(Params)], handle::Stmt const& stmt, Params&& ...params) const
	{
		bind_<0>(stmt, lengths, std::forward<Params>(params)...);
	}
private:
	template<SQLUSMALLINT N, typename T, typename ...Params>
	void bind_(handle::Stmt const& stmt, SQLLEN* lengths, T const& param, Params&& ...params) const
	{
		Bind<std::decay_t<T>>{}(stmt, N + 1, param, lengths[N]);
		bind_<N+1>(stmt, lengths, std::forward<Params>(params)...);
	}


	template<SQLUSMALLINT N>
	void bind_(handle::Stmt const& /*stmt*/, SQLLEN* /*lengths*/) const {}
};

}/*namespace in*/ }  /*namespace details*/ }/*inline namespace v0*/} //namespace odbcx
