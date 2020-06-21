// Copyright (c) 2018-2019 Serge Klimov serge.klim@outlook.com

#pragma once
#include "odbcx/handle.hpp"
//#include "odbcx/details/cast.hpp"
#include "odbcx/details/diversion.hpp"
#include <boost/range/iterator_range.hpp>
#include <boost/mp11/utility.hpp>
#include <vector>
#include <string>
#include <algorithm>
#include <iterator>
#include <type_traits>
#include <cstring>
#include <cstddef>
#include <cassert>

namespace odbcx { inline namespace v1 { namespace details {

template<typename T>
struct DynamicLayout
{
    static constexpr std::size_t mask() noexcept { return ~alignof(T) + 1; }
    static 
#ifdef _NDEBUG
        constexpr 
#endif // !_NDEBUG
        std::size_t value_offset(std::size_t indicator_offset) noexcept
    {
#ifndef _NDEBUG
        assert(indicator_offset % alignof(SQLLEN) == 0);
#endif // !_NDEBUG
        return (indicator_offset + sizeof(SQLLEN) + alignof(T) - 1) & mask();
    }
    static std::size_t bind(handle::adapter::Stmt const& stmt, SQLUSMALLINT column, SQLSMALLINT type, std::size_t offset, std::size_t size)
    {
        offset *= sizeof(SQLLEN);
        offset += binding_offset____();
        auto val_offset = value_offset(offset);
        call(&SQLBindCol, stmt, column, type, reinterpret_cast<SQLPOINTER>(val_offset), size, reinterpret_cast<SQLLEN*>(offset));
        return (val_offset + size + sizeof(SQLLEN) - 1) / sizeof(SQLLEN);
    }

    static void rebind(handle::adapter::Stmt const& stmt, SQLUSMALLINT column, SQLSMALLINT type, std::size_t begin, std::size_t end)
    {
        auto val_offset = value_offset(begin * sizeof(SQLLEN));
        assert(end * sizeof(SQLLEN) >= val_offset);
        bind(stmt, column, type, begin, end * sizeof(SQLLEN) - val_offset);
    }
    static constexpr SQLLEN binding_offset____() noexcept { return sizeof(SQLLEN); }
};

template<typename T> struct Flyweight;

template<typename T>
class FlyweightBase
{
    static_assert(std::is_standard_layout<T>::value, "Oops! below is not going to work:(");
    struct ConstPolicy
    {
        using ValueType = typename std::remove_const<T>::type;
        using IndicatorPointer = SQLLEN const*;
        using ValuePointer = T const*;
    };
    struct NonConstPolicy
    {
        using ValueType = T;
        using IndicatorPointer = SQLLEN * ;
        using ValuePointer = T*;
    };
protected:
    using Policy = typename std::conditional<std::is_const<T>::value, ConstPolicy, NonConstPolicy>::type;
    constexpr FlyweightBase(typename Policy::IndicatorPointer indicator) noexcept : indicator_(indicator){}
    constexpr typename Policy::ValuePointer value() const noexcept { return value_ptr(indicator_); }
    constexpr SQLLEN size() const noexcept { return is_null() ? 0 : *indicator_; }    
    void assign(diversion::nullopt_t) noexcept
    {
        *indicator_ = SQL_NULL_DATA;
    }
    void assign(std::nullptr_t) noexcept { assign(diversion::nullopt); }
    void assign(void const* val, std::size_t size) noexcept
    {
        *indicator_ = SQLLEN(size);
        std::memcpy(value(), val, size);
    }
public:
    using value_type = typename Policy::ValueType;
    constexpr bool is_null() const noexcept { return *indicator_ == SQL_NULL_DATA; }
    /*explicit*/ constexpr bool operator!() const noexcept { return is_null(); }
private:
    static constexpr typename Policy::ValuePointer value_ptr(typename Policy::IndicatorPointer indicator) noexcept
    {
        return reinterpret_cast<typename Policy::ValuePointer>(DynamicLayout<T>::value_offset(reinterpret_cast<std::size_t>(indicator)));
    }
    template<typename U, std::size_t N> 
    friend void copy2array(U(&dest)[N], Flyweight<const U[N]> const& src);
private:
    typename Policy::IndicatorPointer indicator_;
};

template<typename T>
struct Flyweight : FlyweightBase<T>
{
    using Base = FlyweightBase<T>;
    using value_type = typename Base::value_type;
    constexpr Flyweight(typename Base::Policy::IndicatorPointer begin, typename Base::Policy::IndicatorPointer end) noexcept : FlyweightBase<T>{ begin } 
    {
        //assert(reinterpret_cast<std::uintptr_t>(Base::value()) + sizeof(T) <= reinterpret_cast<std::uintptr_t>(end));
    }

    using Base::is_null;

    value_type value() const noexcept
    {
        assert(is_null() || Base::size() == sizeof(value_type));
        value_type res;
        std::memcpy(&res, Base::value(), sizeof(value_type));
        return res;
    }
    
    template<typename U, typename Enable = typename std::enable_if<!std::is_const<T>::value, U>::type>
    Flyweight& operator=(U const& value) noexcept { assign(value); return *this; }
private:
    using Base::assign;
    void assign(value_type const& value) noexcept { Base::assign(&value, sizeof(value_type)); }
    void assign(diversion::optional<value_type> const& value)  noexcept
    {
        if (!value)
            assign(diversion::nullopt);
        else
            assign(*value);
    }
};

template<typename T, typename Result>
Result flyweight_cast(T const& src, boost::mp11::mp_identity<Result>) noexcept { return src.value(); }

template<typename T, typename Result>
diversion::optional<Result> flyweight_cast(T const& src, boost::mp11::mp_identity<diversion::optional<Result>>)
{
    return src.is_null() ? diversion::nullopt : diversion::make_optional(flyweight_cast(src, boost::mp11::mp_identity<Result>{}));
}
template<typename T, typename Result>
Result cast(Flyweight<T> const& src, boost::mp11::mp_identity<Result>) { return flyweight_cast(src, boost::mp11::mp_identity<Result>{}); }

template<typename T, std::size_t N>
struct Flyweight<T[N]> : FlyweightBase<T>
{
    using Base = FlyweightBase<T>;
    using value_type = typename Base::value_type;
    constexpr Flyweight(typename Base::Policy::IndicatorPointer begin, typename Base::Policy::IndicatorPointer end) noexcept : FlyweightBase<T>{ begin } 
    {
        //assert(reinterpret_cast<std::uintptr_t>(Base::value()) + sizeof(T[N]) <= reinterpret_cast<std::uintptr_t>(end));
    }

    using Base::is_null;

    constexpr std::size_t size() const noexcept { return Base::size()/sizeof(value_type); }
    constexpr std::size_t capacity() const noexcept { return N; }
    boost::iterator_range<value_type const*> value() const { return { Base::value(), Base::value() + size() }; }
    template<typename U, typename Enable = typename std::enable_if<!std::is_const<T>::value, U>::type>
    Flyweight& operator=(U&& value) { assign(std::forward<U>(value)); return *this; }
private:
    using Base::assign;
    void assign(std::vector<value_type> const& value)
    {
        assert(capacity() >= value.size());
        Base::assign(value.data(), (std::min)(capacity(), value.size()));
    }
    void assign(std::basic_string<value_type> const& value)
    {
        assert(capacity() >= value.size());
        Base::assign(value.data(), (std::min)(capacity(), value.size()));
    }

    template<typename U>
    void assign(diversion::optional<U> const& value)  noexcept
    {
        if (!value)
            Base::assign(diversion::nullopt);
        else
            assign(*value);
    }
};

template<typename T, std::size_t N, typename Result>
Result flyweight_cast(Flyweight<T[N]> const& src, boost::mp11::mp_identity<Result>)
{
    auto const& range = src.value();
    return { range.begin(), range.end() };
}

template<typename T, std::size_t N, typename Result>
diversion::optional<Result> flyweight_cast(Flyweight<T[N]> const& src, boost::mp11::mp_identity<diversion::optional<Result>>)
{
    return src.is_null() ? diversion::nullopt : diversion::make_optional(flyweight_cast(src, boost::mp11::mp_identity<Result>{}));
}

template<typename T, std::size_t N>
void copy2array(T(&dest)[N], Flyweight<const T[N]> const& src)
{
    auto size = src.size();
    assert(size <= src.capacity());
    std::memcpy(dest, src.value_ptr(src.indicator_), size * sizeof(T));
    std::memset(dest + size, 0, (src.capacity() - size) * sizeof(T));
}

template<typename T>
Flyweight<typename std::add_const<T>::type> make_flyweight(SQLLEN const* begin, SQLLEN const* end) { return { begin, end }; }

template<typename T>
Flyweight<T> make_flyweight(SQLLEN* begin, SQLLEN* end) { return { begin, end };}

template<typename T>
struct FlyweightArray : FlyweightBase<T>
{
    static_assert(alignof(SQLLEN) % alignof(T) == 0 , "Oops! not going to work:(");
    using Base = FlyweightBase<T>;
    using value_type = typename Base::value_type;
    constexpr FlyweightArray(typename Base::Policy::IndicatorPointer begin, typename Base::Policy::IndicatorPointer end) noexcept
        : FlyweightBase<T>{ begin }
        , capacity_{((end - begin - 1) * sizeof(SQLLEN))/sizeof(T) }
    {
    }

    using Base::is_null;

    constexpr std::size_t size() const noexcept { return Base::size()/sizeof(value_type); }
    constexpr std::size_t capacity() const noexcept { return capacity_; }
    boost::iterator_range<value_type const*> value() const { return { Base::value(), Base::value() + size() }; }
    template<typename U, typename Enable = typename std::enable_if<!std::is_const<T>::value, U>::type>
    FlyweightArray& operator=(U&& value) { assign(std::forward<U>(value)); return *this; }
private:
    using Base::assign;
    void assign(std::vector<value_type> const& value)
    {
        assert(capacity() >= value.size());
        Base::assign(value.data(), (std::min)(capacity(), value.size()));
    }
    void assign(std::basic_string<value_type> const& value)
    {
        assert(capacity() >= value.size());
        Base::assign(value.data(), (std::min)(capacity(), value.size()));
    }

    template<typename U>
    void assign(diversion::optional<U> const& value)  noexcept
    {
        if (!value)
            Base::assign(diversion::nullopt);
        else
            assign(*value);
    }
private:
    std::size_t capacity_;
};

template<typename T, typename Result>
Result flyweight_cast(FlyweightArray<T> const& src, boost::mp11::mp_identity<Result>)
{
    auto const& range = src.value();
    return { range.begin(), range.end() };
}

template<typename T, typename Result>
diversion::optional<Result> flyweight_cast(FlyweightArray<T> const& src, boost::mp11::mp_identity<diversion::optional<Result>>)
{
    return src.is_null() ? diversion::nullopt : diversion::make_optional(flyweight_cast(src, boost::mp11::mp_identity<Result>{}));
}

template<typename T, typename Result>
Result cast(FlyweightArray<T> const& src, boost::mp11::mp_identity<Result>) { return flyweight_cast(src, boost::mp11::mp_identity<Result>{}); }

template<typename T>
FlyweightArray<typename std::add_const<T>::type> make_flyweight_array(SQLLEN const* begin, SQLLEN const* end) { return { begin, end }; }

template<typename T>
FlyweightArray<T> make_flyweight_array(SQLLEN* begin, SQLLEN* end) { return { begin, end }; }



} /*namespace details*/ }/*inline namespace v1*/} //namespace odbcx

