// Copyright (c) 2018-2019 Serge Klimov serge.klim@outlook.com

#pragma once

#include "details/cursor.hpp"
#include "details/query.hpp"
#include <utility>
#include <type_traits>


namespace odbcx { inline namespace v1 {

template<typename Sequence, typename Attributes = attribute::None>
using Cursor = typename std::conditional<details::IsForwardOnlyCursor<Attributes>::value
                                            , details::ForwardOnlyCursor<Sequence, Attributes>
                                            , details::ScrollableCursorType<Sequence, Attributes>
                                           >::type;

}/*inline namespace v1*/} /*namespace odbcx*/