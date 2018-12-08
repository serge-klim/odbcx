#pragma once
#include "odbcx/utility.h"
#include "ttraits.h"
#include <boost/mpl/bool_fwd.hpp>
#include <boost/mpl/not.hpp>
#include <type_traits>
#include <string>
#include <vector>
#include <cstdint>


namespace odbcx { inline namespace v0 { namespace details { namespace in {


template<typename T, typename Enabled = boost::mpl::true_> struct Bind;


template<typename T> 
struct Bind<T, typename boost::mpl::or_<std::is_integral<T>, std::is_floating_point<T>>::type>
{
	void operator()(handle::Stmt const & stmt, SQLUSMALLINT column, T const& data, SQLLEN& /*len*/)
	{
		odbcx::call(&SQLBindParameter, stmt, column, SQL_PARAM_INPUT, CType<T>::value, SQLType<T>::value, 0, 0, const_cast<T*>(&data), 0, nullptr);
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
};


template<std::size_t N>
struct Bind<char[N], boost::mpl::true_>
{
	void operator()(handle::Stmt const& stmt, SQLUSMALLINT column, char const (&data)[N], SQLLEN& length)
	{
		//length = std::char_traits<char>::length(data);
		auto end = data + N;
		auto i = std::find(data, end, '\0');
		length = i == end ? N : std::distance(data, i);
		odbcx::call(&SQLBindParameter, stmt, column, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, length, 0, const_cast<char*>(data), length, &length);
	}
};

template<>
struct Bind<char*, boost::mpl::true_>
{
	void operator()(handle::Stmt const & stmt, SQLUSMALLINT column, const char* data, SQLLEN& length)
	{
		length = std::char_traits<char>::length(data);
		call(&SQLBindParameter, stmt, column, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, length, 0, const_cast<char*>(data), length, &length);
	}
};

template<>
struct Bind<std::string, boost::mpl::true_>
{
	void operator()(handle::Stmt const & stmt, SQLUSMALLINT column, std::string const &data, SQLLEN& length)
	{
		length = data.length();
		odbcx::call(&SQLBindParameter, stmt, column, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, length, 0, const_cast<char*>(data.data()), length, &length);
	}
};

template<>
struct Bind<std::vector<char>, boost::mpl::true_>
{
	void operator()(handle::Stmt const & stmt, SQLUSMALLINT column, std::string const &data, SQLLEN& length)
	{
		length = data.size();
		odbcx::call(&SQLBindParameter, stmt, column, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, length, 0, const_cast<char*>(data.data()), length, &length);
	}
};

template<typename T>
struct Bind<std::vector<T>, boost::mpl::bool_<sizeof(T) == sizeof(std::uint8_t)>>
{
	void operator()(handle::Stmt const & stmt, SQLUSMALLINT column, std::vector<T> const &data, SQLLEN& length)
	{
		length = data.size();
		SQLSMALLINT sqlType = (length > BinarySizeLimit ? SQL_LONGVARBINARY : SQL_BINARY);
		odbcx::call(&SQLBindParameter, stmt, column, SQL_PARAM_INPUT, SQL_C_BINARY, sqlType, length, 0, const_cast<T*>(data.data()), length, &length);
	}
};

//data at exec
//https://docs.microsoft.com/en-us/sql/relational-databases/native-client-odbc-table-valued-parameters/sending-data-as-a-table-valued-parameter-using-data-at-execution-odbc?view=sql-server-2017


struct InputParameters
{
	template<typename ...Params>
	struct Check;

	template<typename T, typename ...Params>
	struct Check<T const&, Params...> : Check<Params...> {};

	template<typename T, typename ...Params>
	struct Check<T&, Params...> : Check<Params...> {};

	template<typename T, typename ...Params>
	struct Check<T&&, Params...> : std::false_type {};

	template<typename T, typename ...Params>
	struct Check<T, Params...> : std::false_type {};

	template<>
	struct Check<> : std::true_type {};

	template<typename ...Params>
	void bind(SQLLEN (&lengths)[sizeof...(Params)], handle::Stmt const& stmt, Params&& ...params) const
	{
		static_assert(Check<Params...>::value, "input parameter can't be temporary!");
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
