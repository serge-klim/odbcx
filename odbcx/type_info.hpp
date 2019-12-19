// Copyright (c) 2018-2019 Serge Klimov serge.klim@outlook.com

#pragma once

#include "odbcx/handle.hpp"
#include "odbcx/utility.hpp"
#include "odbcx/cursor.hpp"
#include <boost/fusion/include/adapt_struct.hpp>
#include <algorithm>
#include <vector>
#include <iterator>

namespace odbcx { inline namespace v1 {

struct TypeInfo
{
    char  type_name[128];
    int   data_type;
    int   column_size;
    char  literal_prefix[128];
    char  literal_suffix[128];
    char  create_params[128];
    short nullable;
    short case_sensitive;
    short searchable;
    short unsigned_attribute;
    short fixed_prec_scale;
    short auto_increment;
    char  local_type_name[128];
    short minimum_scale;
    short maximum_scale;
    short sql_data_type;
    short sql_datetime_sub;
    int num_prec_radix;
    short interval_precision;
};

}/*inline namespace v1*/} /*namespace odbcx*/

BOOST_FUSION_ADAPT_STRUCT(
    odbcx::v1::TypeInfo,
    type_name,
    data_type,
    column_size,
    literal_prefix,
    literal_suffix,
    create_params,
    nullable,
    case_sensitive,
    searchable,
    unsigned_attribute,
    fixed_prec_scale,
    auto_increment,
    local_type_name,
    minimum_scale,
    maximum_scale,
    sql_data_type,
    sql_datetime_sub,
    num_prec_radix,
    interval_precision
)

namespace odbcx { inline namespace v1 {

std::vector<TypeInfo> get_type_info(handle::adapter::Dbc const& dbc)
{
    auto stmt = odbcx::handle::allocate<SQL_HANDLE_STMT>(dbc);
    odbcx::call(&SQLGetTypeInfo, stmt, SQL_ALL_TYPES);
    auto cursor = odbcx::Cursor<TypeInfo>{ std::move(stmt) };
    auto range = cursor.fetch();
    std::vector<TypeInfo> res;
    res.reserve(39);
    std::copy(range.begin(), range.end(), std::back_inserter(res));
    return res;
}


}/*inline namespace v1*/} /*namespace odbcx*/


