// Copyright (c) 2018-2019 Serge Klimov serge.klim@outlook.com

#pragma once
#include "odbcx/utility.hpp"
#include <boost/fusion/include/mpl.hpp>
#include <boost/fusion/include/mpl.hpp>
#include "boost/mpl/range_c.hpp"
#include <boost/mpl/void.hpp>
#include <boost/mpl/bool_fwd.hpp>
#include <boost/mpl/begin_end.hpp>  
#include <boost/mpl/find_if.hpp>
#include <boost/mpl/not.hpp>
#include <type_traits>
#include <cstdint>
#include <string>



namespace odbcx { inline namespace v0 {

using NoBind = boost::mpl::void_;

namespace details {

template<typename T, typename Enabled = boost::mpl::true_> struct CType;
template<typename T, typename Enabled = boost::mpl::true_> struct SQLType;
template<typename T, typename Enabled = boost::mpl::true_> struct CTypeSizeOf : std::integral_constant<SQLLEN,sizeof(T)> {};


template<typename T> struct CType<T, typename boost::mpl::and_<std::is_signed<T>, std::is_integral<T>, boost::mpl::bool_<sizeof(T) == sizeof(std::uint16_t)>>::type>
	: std::integral_constant<SQLSMALLINT, SQL_C_SHORT>{};

template<typename T> struct CType<T, typename boost::mpl::and_<boost::mpl::not_<std::is_signed<T>>, std::is_integral<T>, boost::mpl::bool_<sizeof(T) == sizeof(std::uint16_t)>>::type>
	: std::integral_constant<SQLSMALLINT, SQL_C_USHORT>{};

template<typename T> struct SQLType<T, typename boost::mpl::and_<std::is_integral<T>, boost::mpl::bool_<sizeof(T) == sizeof(std::uint16_t)>>::type>
	: std::integral_constant<SQLSMALLINT, SQL_SMALLINT>{};

template<typename T> struct CType<T, typename boost::mpl::and_<std::is_signed<T>, std::is_integral<T>, boost::mpl::bool_<sizeof(T) == sizeof(std::uint32_t)>>::type>
	: std::integral_constant<SQLSMALLINT, SQL_C_LONG>{};

template<typename T> struct CType<T, typename boost::mpl::and_<boost::mpl::not_<std::is_signed<T>>, std::is_integral<T>, boost::mpl::bool_<sizeof(T) == sizeof(std::uint32_t)>>::type>
	: std::integral_constant<SQLSMALLINT, SQL_C_ULONG>{};

template<typename T> struct SQLType<T, typename boost::mpl::and_<std::is_integral<T>, boost::mpl::bool_<sizeof(T) == sizeof(std::uint32_t)>>::type>
	: std::integral_constant<SQLSMALLINT, SQL_INTEGER>{};

template<typename T> struct CType<T, typename boost::mpl::and_<std::is_signed<T>, std::is_integral<T>, boost::mpl::bool_<(sizeof(T) > sizeof(std::uint32_t))>>::type>
	: std::integral_constant<SQLSMALLINT, SQL_C_SBIGINT>{};

template<typename T> struct CType<T, typename boost::mpl::and_<boost::mpl::not_<std::is_signed<T>>, std::is_integral<T>, boost::mpl::bool_<(sizeof(T) > sizeof(std::uint32_t))>>::type>
	: std::integral_constant<SQLSMALLINT, SQL_C_UBIGINT>{};

template<typename T> struct SQLType<T, typename boost::mpl::and_<std::is_integral<T>, boost::mpl::bool_<(sizeof(T) > sizeof(std::uint32_t))>>::type>
	: std::integral_constant<SQLSMALLINT, SQL_BIGINT>{};

template<typename T> struct CType<T, typename boost::mpl::and_<std::is_floating_point<T>, boost::mpl::bool_<sizeof(T) == sizeof(float)>>::type>
	: std::integral_constant<SQLSMALLINT, SQL_C_FLOAT>{};

template<typename T> struct SQLType<T, typename boost::mpl::and_<std::is_floating_point<T>, boost::mpl::bool_<sizeof(T) == sizeof(float)>>::type>
	: std::integral_constant<SQLSMALLINT, SQL_FLOAT>{};

template<typename T> struct CType<T, typename boost::mpl::and_<std::is_floating_point<T>, boost::mpl::bool_<sizeof(T) == sizeof(double)>>::type>
	: std::integral_constant<SQLSMALLINT, SQL_C_DOUBLE>{};

template<typename T> struct SQLType<T, typename boost::mpl::and_<std::is_floating_point<T>, boost::mpl::bool_<sizeof(T) == sizeof(double)>>::type>
	: std::integral_constant<SQLSMALLINT, SQL_DOUBLE>{};


}  /*namespace details*/ }/*inline namespace v0*/} //namespace odbcx