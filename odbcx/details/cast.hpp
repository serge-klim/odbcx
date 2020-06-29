// Copyright (c) 2018-2019 Serge Klimov serge.klim@outlook.com
#pragma once
#include <boost/mp11/utility.hpp>
#include <utility>

namespace odbcx { inline namespace v1 { namespace details { 

template<typename T> T&& cast(T&& src) { return std::forward<T>(src); }

template<typename T> T&& cast(T&& src, boost::mp11::mp_identity<T>) { return std::forward<T>(src); }

template<typename Result, typename Source>
Result cast(Source&& src) { return cast(std::forward<Source>(src), boost::mp11::mp_identity<Result>{}); }

}  /*namespace details*/ }/*inline namespace v1*/} /*namespace odbcx*/

