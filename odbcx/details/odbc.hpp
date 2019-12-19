// Copyright (c) 2018-2019 Serge Klimov serge.klim@outlook.com

#pragma once
#ifdef _WIN32
#include <windows.h>
#endif // _WIN32
#include <sql.h> 
#include <sqlext.h>
#include <cstddef>

namespace odbcx { inline namespace v1 { namespace details {

static constexpr std::size_t max_bindable_data_size = 8000;
static constexpr std::size_t binding_offset = sizeof(SQLLEN);

}  /*namespace details*/ }/*inline namespace v1*/} /*namespace odbcx*/