// Copyright (c) 2018-2019 Serge Klimov serge.klim@outlook.com

#include "utility.hpp"
#include <sstream>
#include <vector>
#include <stdexcept>
#include <cassert>

SQLRETURN odbcx::v1::if_failed_throw(SQLRETURN sqlres, SQLHANDLE handle, SQLSMALLINT type)
{
    switch (sqlres)
    {
        case SQL_SUCCESS:
        case SQL_SUCCESS_WITH_INFO:
        case SQL_NO_DATA:
            return sqlres;
        case SQL_INVALID_HANDLE:
            throw std::runtime_error{ "invalid handle" };
    }

    std::ostringstream message;

    for (SQLSMALLINT rec = 1; ; ++rec)
    {
        SQLCHAR state[SQL_SQLSTATE_SIZE + 1];
        SQLINTEGER  error;
        SQLSMALLINT len;
        switch (auto res = SQLGetDiagRec(type, handle, rec, state, &error, nullptr, 0, &len))
        {
            case SQL_NO_DATA:
                throw std::runtime_error{ message.str() };
            case SQL_SUCCESS:
			case SQL_SUCCESS_WITH_INFO:
                break;
            default:
                message << "can't obtain error rec " << rec << ':' << res << '\n';
                continue;
        }
        auto buffer = std::vector<SQLCHAR>(std::size_t(len) + 1);
        auto res = SQLGetDiagRec(type, handle, rec, state, &error, buffer.data(), SQLSMALLINT(buffer.size()), &len);
        if (res != SQL_SUCCESS)
        {
            assert(res != SQL_NO_DATA && "SQL_NO_DATA is unexpected here");
            message << "can't obtaine error rec " << rec << ':' << res << '\n';
        }
        else
            message << state << ' ' << buffer.data() << '(' << error << ")\n";
    }
}

