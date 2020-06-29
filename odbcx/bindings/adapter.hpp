// Copyright (c) 2018-2019 Serge Klimov serge.klim@outlook.com

#pragma once

#include "flyweight.hpp"
#include "surrogate.hpp"
#include "odbcx/details/cast.hpp"
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
#if !defined(BOOST_GCC_VERSION) || BOOST_GCC_VERSION >= 40902 // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=60943
    constexpr Adaptee const& adaptee() const& noexcept { return adaptee_; }
    /*constexpr*/ Adaptee&& adaptee() && noexcept { return std::move(adaptee_); }
#else
    Adaptee&& adaptee() noexcept { return std::move(adaptee_); }
#endif
    AdapterProxy& operator = (T const& value)
    {
        adaptee_ = Adapter<T>{}(value);
        return *this;
    }
    AdapterProxy& operator = (diversion::optional<T> const& value)
    {
        adaptee_ = !value ? Adapter<T>{}(*value) : diversion::nullopt;
        return *this;
    }
private:
    Adaptee adaptee_;
};


template<typename T, typename Adaptee, typename Result>
Result cast(AdapterProxy<T, Adaptee>&& src, boost::mp11::mp_identity<Result>) 
{ 
    auto addopted = odbcx::details::cast(std::move(src).adaptee(), boost::mp11::mp_identity<typename Adapter<T>::type>{});
    return cast(Adapter<T>{}(std::move(addopted)), boost::mp11::mp_identity<Result>{});
}

template<typename T, typename Adaptee, typename Result>
diversion::optional<Result> cast(AdapterProxy<T, Adaptee>&& src, boost::mp11::mp_identity<diversion::optional<Result>>)
{
    return src.adaptee().is_null() ? diversion::nullopt : diversion::make_optional(cast( std::forward<AdapterProxy<T, Adaptee>>(src), boost::mp11::mp_identity<Result>{}));
}


template<typename T, typename Adaptee>
AdapterProxy<T, Adaptee> adapt(Adaptee&& adaptee) { return {std::forward<Adaptee>(adaptee)}; }


}  /*namespace details*/ }/*inline namespace v1*/} /*namespace odbcx*/
