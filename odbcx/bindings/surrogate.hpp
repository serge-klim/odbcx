// Copyright (c) 2018-2019 Serge Klimov serge.klim@outlook.com

#pragma once
#include "odbcx/utility.hpp"
#include "ttraits.hpp"
#include "odbcx/details/diversion.hpp"
#include <boost/range/iterator_range.hpp>
#include <vector>
#include <string>
#include <iterator>
#include <stdexcept>
#include <cstring>
#include <cassert>

namespace odbcx { inline namespace v1 { 

template<typename T, SQLSMALLINT SqlType = details::CType<T>::value, SQLLEN Size = details::CTypeSizeOf<T>::value>
class Surrogate
{
public:
    Surrogate(handle::adapter::Stmt stmt, SQLUSMALLINT column)
    {
        static_assert(sizeof(T) >= Size, "Oops!");
        if (call(&SQLGetData, stmt, column, SqlType, &value_, Size, &indicator_) == SQL_NO_DATA)
            throw std::runtime_error{ "SQLGetData can be called once only" };
        if(indicator_ == SQL_NO_TOTAL)
            throw std::length_error{ "" };
    }

    /*explicit*/ constexpr bool operator!() const noexcept { return is_null(); }
    constexpr bool is_null() const noexcept { return indicator_ == SQL_NULL_DATA; }
    constexpr operator T const& () const noexcept { return value_; }
    constexpr operator diversion::optional<T>() const noexcept { return is_null() ? diversion::nullopt : diversion::make_optional(value_); }
private:
    SQLLEN indicator_ = 0;
    T value_ = {};
};

template<typename T, std::size_t N, SQLSMALLINT SqlType, SQLLEN Size>
class Surrogate<T[N], SqlType, Size>
{
public:
    using value_type = T;
    Surrogate(handle::adapter::Stmt stmt, SQLUSMALLINT column)
    {
        static_assert(sizeof(T)*N >= Size, "Oops!");
        if (call(&SQLGetData, stmt, column, SqlType, &value_, Size, &indicator_) == SQL_NO_DATA)
            throw std::runtime_error{ "SQLGetData can be called once only" };
        if (indicator_ == SQL_NO_TOTAL)
            throw std::length_error{ "" };
    }

    /*explicit*/ constexpr bool operator!() const noexcept { return is_null(); }
    constexpr bool is_null() const noexcept { return indicator_ == SQL_NULL_DATA; }
    constexpr value_type const* value() const noexcept { return value_; }
    constexpr operator value_type const* () const noexcept { return value(); }
    constexpr std::size_t size() const noexcept { return is_null() ? 0 : std::size_t(indicator_ / sizeof(T)); }
    constexpr std::size_t capacity() const noexcept { return N; }

    operator std::vector<value_type>() const { return { value(), value() + size() }; }
    operator std::basic_string<value_type>() const { return { value(), value() + size() }; }
    operator diversion::basic_string_view<value_type>() const { return { value(), size() }; }
    operator boost::iterator_range<value_type const*>() const { return { value(), value() + size() }; }

    operator diversion::optional<std::vector<value_type>>() const { return is_null() ? diversion::nullopt : diversion::make_optional(static_cast<std::vector<value_type>>(*this)); }
    operator diversion::optional<std::basic_string<value_type>>() const { return is_null() ? diversion::nullopt : diversion::make_optional(static_cast<std::basic_string<value_type>>(*this)); }
    operator diversion::optional<diversion::basic_string_view<value_type>>() const { return is_null() ? diversion::nullopt : diversion::make_optional(static_cast<diversion::basic_string_view<value_type>>(*this)); }
    operator diversion::optional< boost::iterator_range<typename std::vector<value_type>::const_iterator>>() const
    {
        return is_null() ? diversion::nullopt : diversion::make_optional(static_cast<boost::iterator_range<typename std::vector<value_type>::const_iterator>>(*this));
    }
private:
    SQLLEN indicator_ = 0;
    value_type value_[N];
};


template<typename T, SQLSMALLINT SqlType>
class SurrogateVector
{
public:
    using value_type = T;
    SurrogateVector(handle::adapter::Stmt stmt, SQLUSMALLINT column)
    {
	    std::size_t chunk_size = 1024;
	    auto buffer = std::vector<T>(chunk_size);
	    //https://docs.microsoft.com/en-us/sql/odbc/reference/develop-app/getting-long-data?view=sql-server-2017
	    if (call(&SQLGetData, stmt, column, SqlType, buffer.data(), buffer.size() * sizeof(T), &indicator_) == SQL_NO_DATA)
                throw std::runtime_error{ "SQLGetData can be called once only" };
        auto total = std::size_t{ 0 };
        if (indicator_ != SQL_NULL_DATA)
        {
            if (indicator_ == SQL_NO_TOTAL || std::size_t(indicator_) == chunk_size * sizeof(T))
            {
                //well have to read it chunk by chunk:(
                do {
                    if (indicator_ == SQL_NO_TOTAL)
                        indicator_ = SQLLEN(chunk_size * sizeof(T));
                    assert(indicator_ % sizeof(T) == 0);
                    total += std::size_t(indicator_) / sizeof(T);
                    if (indicator_ != SQLLEN(chunk_size * sizeof(T)))
                    {
                        assert(indicator_ < SQLLEN(chunk_size * sizeof(T)));
                        assert(((indicator_ = 0), "emulating SQLGetData(...) == SQL_NO_DATA and read == 0 to pass next assert"));
                        break;
                    }
                    buffer.resize(total + chunk_size);
                    indicator_ = 0;
                } while (call(&SQLGetData, stmt, column, SqlType, buffer.data() + total / sizeof(T), buffer.size() * sizeof(T) - total, &indicator_) != SQL_NO_DATA);
                assert(indicator_ == 0);
            }
            else
            {
                assert(indicator_ % sizeof(T) == 0);
                total = std::size_t(indicator_) / sizeof(T);
                if (indicator_ > SQLLEN(chunk_size * sizeof(T))) // driver just returns size of data, so read it one go
                {
                    buffer.resize(total);
                    assert(buffer.size() > chunk_size);
                    indicator_ = 0;
                    call(&SQLGetData, stmt, column, SqlType, buffer.data() + chunk_size, (buffer.size() - chunk_size) * sizeof(T), &indicator_);
                    assert(indicator_ % sizeof(T) == 0);
                    assert(std::size_t(indicator_) / sizeof(T) + chunk_size == total);
                }
            }
            buffer.resize(total);
            value_ = std::move(buffer);
        }
    }

    /*explicit*/ constexpr bool operator!() const noexcept { return is_null(); }
    constexpr bool is_null() const noexcept { return indicator_ == SQL_NULL_DATA; }
    /*constexpr*/ std::size_t size() const noexcept { return is_null() ? 0 : value_.size(); }

    constexpr operator std::vector<T> const& () const& noexcept { return value_; }
#if !defined(BOOST_GCC_VERSION) || BOOST_GCC_VERSION > 50000
    operator std::vector<value_type>() const&& noexcept { return std::move(value_); }
#endif
    operator diversion::optional<std::vector<value_type>>() const& { return is_null() ? diversion::nullopt : diversion::make_optional(value_); }
    operator diversion::optional<std::vector<value_type>>()&& { return is_null() ? diversion::nullopt : diversion::make_optional(std::move(value_)); }

    operator std::basic_string<value_type>() const { return { value_.data(), value_ .size() }; }
    operator diversion::basic_string_view<value_type>() const { return { value_.data(), value_.size() }; }
    operator boost::iterator_range<typename std::vector<value_type>::const_iterator>() const { return { std::begin(value_), std::end(value_) }; }


    operator diversion::optional<std::basic_string<value_type>>() const { return is_null() ? diversion::nullopt : diversion::make_optional(static_cast<std::basic_string<value_type>>(*this)); }
    operator diversion::optional<diversion::basic_string_view<value_type>>() const { return is_null() ? diversion::nullopt : diversion::make_optional(static_cast<diversion::basic_string_view<value_type>>(*this)); }
    operator diversion::optional< boost::iterator_range<typename std::vector<value_type>::const_iterator>>() const
    { 
        return is_null() ? diversion::nullopt : diversion::make_optional(static_cast<boost::iterator_range<typename std::vector<value_type>::const_iterator>>(*this));
    }
private:
    SQLLEN indicator_ = 0;
    std::vector<value_type> value_;
};

template<typename T, std::size_t N, SQLSMALLINT SqlType, SQLLEN Size>
void copy2array(T(&dest)[N], Surrogate<T[N], SqlType, Size> const& src)
{
    auto size = src.size();
    assert(size <= N);
    std::memcpy(dest, src.value(), size * sizeof(T));
    std::memset(dest + size, 0, (N - size) * sizeof(T));
}


}/*inline namespace v1*/} //namespace odbcx
