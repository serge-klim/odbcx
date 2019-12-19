// Copyright (c) 2018-2019 Serge Klimov serge.klim@outlook.com

#pragma once
#include "odbcx/details/diversion.hpp"
#include <utility>

namespace odbcx { 

template<typename T> struct Adapter;

inline namespace v1 { namespace details {

template<typename T, typename Adaptee>
class AdapterProxy 
{
public:
    AdapterProxy(Adaptee&& adaptee) :adaptee_(std::forward<Adaptee>(adaptee)) {}
    T value() const { return Adapter<T>{}( adaptee_); }
    operator T() const { return value();}
    AdapterProxy& operator = (T const& value)
    {
        adaptee_ = Adapter<T>{}(value);
        return *this;
    }
    constexpr operator diversion::optional<T>() const noexcept 
    {
        return adaptee_.is_null() ? diversion::nullopt : diversion::make_optional(value());
    }
    AdapterProxy& operator = (diversion::optional<T> const& value)
    {
        adaptee_ = !value ? Adapter<T>{}(*value) : diversion::nullopt;
        return *this;
    }
private:
    Adaptee adaptee_;
};


template<typename T, typename Adaptee>
AdapterProxy<T, Adaptee> adapt(Adaptee&& adaptee)
{
    return {std::forward<Adaptee>(adaptee)};
}


}  /*namespace details*/ }/*inline namespace v1*/} /*namespace odbcx*/
