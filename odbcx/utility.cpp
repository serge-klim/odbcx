#include <cstdint>
#include <stdexcept>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <tuple>
#include <cassert>
#include "utility.hpp"

SQLRETURN odbcx::v0::details::IfFailedThrow(SQLRETURN sqlres, SQLHANDLE handle, SQLSMALLINT type)
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
        auto buffer = std::vector<SQLCHAR>(len + 1);
        auto res = SQLGetDiagRec(type, handle, rec, state, &error, buffer.data(), SQLSMALLINT(buffer.size()), &len);
        if (res != SQL_SUCCESS)
        {
            assert(res != SQL_NO_DATA && "SQL_NO_DATA is unexpected here");
            message << "can't obtaine error rec " << rec << ':' << res << '\n';
        }
        else
            message << state << ' ' << buffer.data() << '(' << error << ")\n";
    }
    //throw std::runtime_error{ message.str() };
    //return sqlres;
}

void odbcx::v0::begin_transaction(handle::Dbc const& dbc) { return begin_transaction(dbc.get()); }
void odbcx::v0::end_transaction(handle::Dbc const& dbc, SQLSMALLINT completionType /*= SQL_ROLLBACK*/) { end_transaction(dbc.get(), completionType); }

void odbcx::v0::begin_transaction(SQLHANDLE dbc)
{
	assert(autocommit_mode(dbc));
	details::IfFailedThrow(::SQLSetConnectAttr(dbc, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_OFF, 0), dbc, SQL_HANDLE_DBC);
}

void odbcx::v0::end_transaction(SQLHANDLE dbc, SQLSMALLINT completionType /*= SQL_ROLLBACK*/)
{
	assert(!autocommit_mode(dbc));
	details::IfFailedThrow(::SQLEndTran(SQL_HANDLE_DBC, dbc, completionType), dbc, SQL_HANDLE_DBC);
	details::IfFailedThrow(::SQLSetConnectAttr(dbc, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_ON, 0), dbc, SQL_HANDLE_DBC);
}

bool odbcx::v0::autocommit_mode(handle::Dbc const& dbc) { return autocommit_mode(dbc.get()); }
bool odbcx::v0::autocommit_mode(SQLHANDLE dbc)
{
	SQLULEN val = 0;
	details::IfFailedThrow(::SQLGetConnectAttr(dbc, SQL_ATTR_AUTOCOMMIT, &val, sizeof(val), nullptr), dbc, SQL_HANDLE_DBC);
	return val == SQL_AUTOCOMMIT_ON;
}


