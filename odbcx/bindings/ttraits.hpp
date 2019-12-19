// Copyright (c) 2018-2019 Serge Klimov serge.klim@outlook.com

#pragma once
#include "odbcx/details/odbc.hpp"
#include <boost/mp11/function.hpp>
#include <boost/mp11/algorithm.hpp>
#include <boost/mp11/integral.hpp>
#include <type_traits>
#include <string>
#include <cstdint>


namespace odbcx { inline namespace v1 {

//using NoBind = boost::mpl::void_;
struct NoBind {};

namespace details {

template<typename T, typename Enabled = boost::mp11::mp_true> struct CType;
template<typename T, typename Enabled = boost::mp11::mp_true> struct SQLType;
template<typename T, typename Enabled = boost::mp11::mp_true> struct CTypeSizeOf : std::integral_constant<SQLLEN,sizeof(T)> {};


template<typename T> struct CType<T, typename boost::mp11::mp_and<std::is_signed<T>, std::is_integral<T>, boost::mp11::mp_bool<sizeof(T) == sizeof(std::uint16_t)>>::type>
	: std::integral_constant<SQLSMALLINT, SQL_C_SHORT>{};

template<typename T> struct CType<T, typename boost::mp11::mp_and<boost::mp11::mp_not<std::is_signed<T>>, std::is_integral<T>, boost::mp11::mp_bool<sizeof(T) == sizeof(std::uint16_t)>>::type>
	: std::integral_constant<SQLSMALLINT, SQL_C_USHORT>{};

template<typename T> struct SQLType<T, typename boost::mp11::mp_and<std::is_integral<T>, boost::mp11::mp_bool<sizeof(T) == sizeof(std::uint16_t)>>::type>
	: std::integral_constant<SQLSMALLINT, SQL_SMALLINT>{};

template<typename T> struct CType<T, typename boost::mp11::mp_and<std::is_signed<T>, std::is_integral<T>, boost::mp11::mp_bool<sizeof(T) == sizeof(std::uint32_t)>>::type>
	: std::integral_constant<SQLSMALLINT, SQL_C_LONG>{};

template<typename T> struct CType<T, typename boost::mp11::mp_and<boost::mp11::mp_not<std::is_signed<T>>, std::is_integral<T>, boost::mp11::mp_bool<sizeof(T) == sizeof(std::uint32_t)>>::type>
	: std::integral_constant<SQLSMALLINT, SQL_C_ULONG>{};

template<typename T> struct SQLType<T, typename boost::mp11::mp_and<std::is_integral<T>, boost::mp11::mp_bool<sizeof(T) == sizeof(std::uint32_t)>>::type>
	: std::integral_constant<SQLSMALLINT, SQL_INTEGER>{};

template<typename T> struct CType<T, typename boost::mp11::mp_and<std::is_signed<T>, std::is_integral<T>, boost::mp11::mp_bool<(sizeof(T) > sizeof(std::uint32_t))>>::type>
	: std::integral_constant<SQLSMALLINT, SQL_C_SBIGINT>{};

template<typename T> struct CType<T, typename boost::mp11::mp_and<boost::mp11::mp_not<std::is_signed<T>>, std::is_integral<T>, boost::mp11::mp_bool<(sizeof(T) > sizeof(std::uint32_t))>>::type>
	: std::integral_constant<SQLSMALLINT, SQL_C_UBIGINT>{};

template<typename T> struct SQLType<T, typename boost::mp11::mp_and<std::is_integral<T>, boost::mp11::mp_bool<(sizeof(T) > sizeof(std::uint32_t))>>::type>
	: std::integral_constant<SQLSMALLINT, SQL_BIGINT>{};

template<typename T> struct CType<T, typename boost::mp11::mp_and<std::is_floating_point<T>, boost::mp11::mp_bool<sizeof(T) == sizeof(float)>>::type>
	: std::integral_constant<SQLSMALLINT, SQL_C_FLOAT>{};

template<typename T> struct SQLType<T, typename boost::mp11::mp_and<std::is_floating_point<T>, boost::mp11::mp_bool<sizeof(T) == sizeof(float)>>::type>
	: std::integral_constant<SQLSMALLINT, SQL_FLOAT>{};

template<typename T> struct CType<T, typename boost::mp11::mp_and<std::is_floating_point<T>, boost::mp11::mp_bool<sizeof(T) == sizeof(double)>>::type>
	: std::integral_constant<SQLSMALLINT, SQL_C_DOUBLE>{};

template<typename T> struct SQLType<T, typename boost::mp11::mp_and<std::is_floating_point<T>, boost::mp11::mp_bool<sizeof(T) == sizeof(double)>>::type>
	: std::integral_constant<SQLSMALLINT, SQL_DOUBLE>{};

template<> struct CType<SQL_TIMESTAMP_STRUCT, boost::mp11::mp_true>
    : std::integral_constant<SQLSMALLINT, SQL_C_TYPE_TIMESTAMP> {};



}  /*namespace details*/ }/*inline namespace v1*/} //namespace odbcx