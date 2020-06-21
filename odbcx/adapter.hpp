// Copyright (c) 2018-2019 Serge Klimov serge.klim@outlook.com

#pragma once

#include "details/odbc.hpp"
#include <chrono>
#include <time.h>

namespace odbcx {

template<typename T> struct Adapter;

template<>
struct Adapter<std::chrono::system_clock::time_point>
{
    using type = SQL_TIMESTAMP_STRUCT;
    std::chrono::system_clock::time_point operator()(SQL_TIMESTAMP_STRUCT const& value) const noexcept 
    { 
        if (value.month == 0)
            return {};
        struct tm t = {0};
        t.tm_year = value.year - 1900;
        t.tm_mon = value.month - 1;
        t.tm_mday = value.day;
        t.tm_hour = value.hour;
        t.tm_min = value.minute;
        t.tm_sec = value.second;
        return std::chrono::system_clock::from_time_t(mktime(&t));
    }

    type operator()(std::chrono::system_clock::time_point const& value) const /*noexcept*/
    { 
        time_t tt = std::chrono::system_clock::to_time_t(value);
        auto ts = gmtime(&tt);
        auto timestamp = SQL_TIMESTAMP_STRUCT{ 0 };
        timestamp.year = ts->tm_year + 1900;
        timestamp.month = ts->tm_mon + 1;
        timestamp.day = ts->tm_mday;
        timestamp.hour = ts->tm_hour;
        timestamp.minute = ts->tm_min;
        timestamp.second = ts->tm_sec;
        return timestamp;
    }
};

} /*namespace odbcx*/
