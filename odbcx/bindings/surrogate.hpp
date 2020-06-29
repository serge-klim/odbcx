// Copyright (c) 2018-2019 Serge Klimov serge.klim@outlook.com

#pragma once
#include "ttraits.hpp"
#include "odbcx/handle.hpp"
//#include "odbcx/details/cast.hpp"
#include "odbcx/details/diversion.hpp"
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
    constexpr T const& value() const noexcept { return value_; }
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
    constexpr std::size_t size() const noexcept { return is_null() ? 0 : std::size_t(indicator_ / sizeof(T)); }
    constexpr std::size_t capacity() const noexcept { return N; }
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
    constexpr std::vector<T> const& value() const& noexcept { return value_; }
    std::vector<value_type>&& value() && noexcept { return std::move(value_); }
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

namespace details{

template<typename T, SQLSMALLINT SqlType, SQLLEN Size>
T const& cast(Surrogate<T, SqlType, Size> const& src, boost::mp11::mp_identity<T>)
{
    return src.value();
}

template<typename T, SQLSMALLINT SqlType, SQLLEN Size>
T cast(Surrogate<T, SqlType, Size>&& src, boost::mp11::mp_identity<T>)
{
    return src.value();
}

template<typename T, SQLSMALLINT SqlType, SQLLEN Size>
diversion::optional<T> cast(Surrogate<T, SqlType, Size>&& src, boost::mp11::mp_identity<diversion::optional<T>>)
{
    return src.is_null() ? diversion::nullopt : diversion::make_optional(src.value());
}
template<typename T, std::size_t N, SQLSMALLINT SqlType, SQLLEN Size>
std::vector<T> cast(Surrogate<T[N], SqlType, Size>&& src, boost::mp11::mp_identity<std::vector<T>>)
{
    return { src.value(), src.value() + src.size() };
}

template<typename T, std::size_t N, SQLSMALLINT SqlType, SQLLEN Size>
std::basic_string<T> cast(Surrogate<T[N], SqlType, Size>&& src, boost::mp11::mp_identity<std::basic_string<T>>)
{
    return { src.value(), src.value() + src.size() };
}

template<typename T, std::size_t N, SQLSMALLINT SqlType, SQLLEN Size>
diversion::basic_string_view<T> cast(Surrogate<T[N], SqlType, Size>&& src, boost::mp11::mp_identity<diversion::basic_string_view<T>>)
{
    return { src.value(), src.size() };
}

template<typename T, std::size_t N, SQLSMALLINT SqlType, SQLLEN Size>
diversion::optional<std::vector<T>> cast(Surrogate<T[N], SqlType, Size>&& src, boost::mp11::mp_identity< diversion::optional<std::vector<T>>>)
{
    return src.is_null() ? diversion::nullopt : diversion::make_optional(cast(std::forward<Surrogate<T[N], SqlType, Size>>(src), boost::mp11::mp_identity<std::vector<T>>{}));
}

template<typename T, std::size_t N, SQLSMALLINT SqlType, SQLLEN Size>
diversion::optional<std::basic_string<T>> cast(Surrogate<T[N], SqlType, Size>&& src, boost::mp11::mp_identity<diversion::optional<std::basic_string<T>>>)
{
    return src.is_null() ? diversion::nullopt : diversion::make_optional(cast(std::forward<Surrogate<T[N], SqlType, Size>>(src), boost::mp11::mp_identity<std::basic_string<T>>{}));
}

template<typename T, std::size_t N, SQLSMALLINT SqlType, SQLLEN Size>
diversion::optional<diversion::basic_string_view<T>> cast(Surrogate<T[N], SqlType, Size>&& src, boost::mp11::mp_identity<diversion::optional<diversion::basic_string_view<T>>>)
{
    return src.is_null() ? diversion::nullopt : diversion::make_optional(cast(std::forward<Surrogate<T[N], SqlType, Size>>(src), boost::mp11::mp_identity<diversion::basic_string_view<T>>{}));
}

template<typename T, SQLSMALLINT SqlType>
std::vector<T> cast(SurrogateVector<T, SqlType>&& src, boost::mp11::mp_identity<std::vector<T>>) 
{ 
    return std::move(src).value();
}

template<typename T, SQLSMALLINT SqlType>
diversion::optional<std::vector<T>> cast(SurrogateVector<T, SqlType>&& src, boost::mp11::mp_identity<diversion::optional<std::vector<T>>>)
{
    return src.is_null() ? diversion::nullopt : diversion::make_optional(std::move(src).value());
}

template<typename T, SQLSMALLINT SqlType>
std::basic_string<T> cast(SurrogateVector<T, SqlType>&& src, boost::mp11::mp_identity<std::basic_string<T>>)
{
    auto const& value = src.value();
    return { value.data(), value.size() };
}

template<typename T, SQLSMALLINT SqlType>
diversion::basic_string_view<T> cast(SurrogateVector<T, SqlType>&& src, boost::mp11::mp_identity<diversion::basic_string_view<T>>)
{
    auto const& value = src.value();
    return { value.data(), value.size() };
}

template<typename T, SQLSMALLINT SqlType>
diversion::optional<std::basic_string<T>> cast(SurrogateVector<T, SqlType>&& src, boost::mp11::mp_identity<diversion::optional<std::basic_string<T>>>)
{
    return src.is_null() ? diversion::nullopt : diversion::make_optional(cast(std::forward<SurrogateVector<T, SqlType>>(src), boost::mp11::mp_identity<std::basic_string<T>>{}));
}

template<typename T, SQLSMALLINT SqlType>
diversion::optional<diversion::basic_string_view<T>> cast(SurrogateVector<T, SqlType>&& src, boost::mp11::mp_identity<diversion::optional<diversion::basic_string_view<T>>>)
{
    return src.is_null() ? diversion::nullopt : diversion::make_optional(cast(std::forward<SurrogateVector<T, SqlType>>(src), boost::mp11::mp_identity<diversion::basic_string_view<T>>{}));
}

} /*namespace details*/

}/*inline namespace v1*/} //namespace odbcx
