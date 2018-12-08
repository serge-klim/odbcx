//#include <windows.h>

#include <cstdint>
#include <stdexcept>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <tuple>
#include <cassert>
#include "utility.h"
//#include <odbcss.h>
//#include <boost/format.hpp>
//#include <boost/chrono/process_cpu_clocks.hpp>
//#include <boost/chrono/chrono_io.hpp>
// CliConfg.exe
// https://kb.iu.edu/d/aytk
// https://support.microsoft.com/en-us/help/942861/general-network-error-communication-link-failure-or-a-transport-level

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
	details::IfFailedThrow(::SQLSetConnectAttr(dbc, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_OFF, 0), dbc, SQL_HANDLE_DBC);
}

void odbcx::v0::end_transaction(SQLHANDLE dbc, SQLSMALLINT completionType /*= SQL_ROLLBACK*/)
{
	details::IfFailedThrow(::SQLEndTran(SQL_HANDLE_DBC, dbc, completionType), dbc, SQL_HANDLE_DBC);
	details::IfFailedThrow(::SQLSetConnectAttr(dbc, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_OFF, 0), dbc, SQL_AUTOCOMMIT_ON);
}




//namespace { namespace details{
//
//template<typename TypeMapper>
//odbcx::Bindings bind(odbcx::handle::Stmt& sqlstmt, TypeMapper mapper)
//{
//	SQLSMALLINT ncolumns;
//	odbcx::call(&SQLNumResultCols, sqlstmt, &ncolumns);
//	size_t offset = 0;
//	auto bindings = std::vector<std::tuple<SQLSMALLINT, size_t>>{};
//	if (ncolumns != 0)
//	{
//		bindings.reserve(ncolumns);
//
//		for (SQLSMALLINT i = 0; i != ncolumns; ++i)
//		{
//			SQLSMALLINT name_length = 0;
//			SQLSMALLINT type;
//			SQLULEN data_size;
//			SQLSMALLINT data_digits;
//			SQLSMALLINT nullable;
//
//			odbcx::call(&SQLDescribeCol,
//				sqlstmt,                  // Select Statement (Prepared)
//				i + 1,                    // Columnn Number
//				nullptr,                  // Column Name (returned)
//				0,                        // size of Column Name buffer
//				&name_length,             // Actual size of column name
//				&type,                    // SQL Data type of column
//				&data_size,               // Data size of column in table
//				&data_digits,             // Number of decimal digits
//				&nullable);               // Whether column nullable
//
//			std::tie(type, data_size) = mapper(type, data_size, data_digits);
//			bindings.emplace_back(type, offset);
//			auto& row = bindings[i];
//			std::uint8_t* buffer = nullptr;
//			odbcx::call(&SQLBindCol, sqlstmt, i + 1, type, buffer + odbcx::details::bindings_offset + offset, data_size, nullptr);
//			offset += data_size;
//		}
//	}
//	return std::make_pair(std::move(bindings), offset);
//}
//
//} /*namespace details*/ } // namespace {
//
//odbcx::Bindings odbcx::bind(odbcx::handle::Stmt& sqlstmt)
//{
//	return ::details::bind(sqlstmt, [](SQLSMALLINT type, SQLUINTEGER size, SQLSMALLINT /*digits*/)
//	{
//		switch (type)
//		{
//			case SQL_VARCHAR:
//			case SQL_WVARCHAR:
//			case SQL_LONGVARCHAR:
//			case SQL_WCHAR:
//			//case SQL_WVARCHAR:
//			//case SQL_WLONGVARCHAR:
//				type = SQL_C_CHAR;
//				break;
//			case SQL_INTEGER:
//			case SQL_SMALLINT:
//			case SQL_BIGINT:
//				type = SQL_C_LONG;
//				size = sizeof(long long);
//				break;
//			case SQL_FLOAT:
//			case SQL_DOUBLE:
//			case SQL_REAL:
//				type = SQL_C_DOUBLE;
//				size = sizeof(long double);
//				break;
//			// SQL_DECIMAL
//		}
//		return std::make_pair(type, size);
//	});
//}
//
//odbcx::Bindings odbcx::bind_as_strings(odbcx::handle::Stmt& sqlstmt)
//{
//	return ::details::bind(sqlstmt, [](SQLSMALLINT type, SQLUINTEGER size, SQLSMALLINT digits)
//	{
//		switch (type)
//		{
//			case SQL_VARCHAR:
//			case SQL_WVARCHAR:
//			case SQL_LONGVARCHAR:
//			case SQL_WCHAR:
//			//case SQL_WVARCHAR:
//			//case SQL_WLONGVARCHAR:
//				break;
//			case SQL_NUMERIC:
//			case SQL_DECIMAL:
//				size = digits + 1;
//			case SQL_SMALLINT:
//				size = 5 + 1;
//			case SQL_TINYINT:
//				size = 3 + 1;
//			case SQL_INTEGER:
//				size = 10 + 1;
//				break;
//			default:
//				size = 256;
//		}
//		return std::make_pair(SQL_C_CHAR, size);
//	});
//}
//
//std::vector<char> odbcx::fetch(odbcx::handle::Stmt& sqlstmt, Bindings const& bindings, size_t n /*= 256*/)
//{
//    auto buffer = std::vector<char>(bindings.second * n);
//    if (bindings.second != 0)
//    {
//        assert(!bindings.first.empty());
//        // https://docs.microsoft.com/en-us/sql/odbc/reference/syntax/sqlbindcol-function?view=sql-server-2017#binding-arrays
//        odbcx::call(&SQLSetStmtAttr, sqlstmt, SQL_ATTR_ROW_BIND_TYPE, reinterpret_cast<SQLPOINTER>(bindings.second), 0);
//        //    odbcx::call(&SQLSetStmtAttr, sqlstmt, SQL_ATTR_PARAM_BIND_OFFSET_PTR, buffer.data(), 0);
//        SQLLEN row_offset = SQLLEN(buffer.data()) - details::bindings_offset;
//        odbcx::call(&SQLSetStmtAttr, sqlstmt, SQL_ATTR_ROW_BIND_OFFSET_PTR, &row_offset, 0);
//        odbcx::call(&SQLSetStmtAttr, sqlstmt, SQL_ATTR_ROW_ARRAY_SIZE, reinterpret_cast<SQLPOINTER>(n), 0);
//
//        SQLULEN fetched;
//        odbcx::call(&SQLSetStmtAttr, sqlstmt, SQL_ATTR_ROWS_FETCHED_PTR, &fetched, 0);
//        odbcx::call(&SQLFetchScroll, sqlstmt, SQL_FETCH_NEXT, 0);
//        buffer.resize(bindings.second * fetched);
//    }
//    return buffer;
//}
//
//size_t odbcx::col_count(Bindings const& bindings)
//{
//    return bindings.first.size();
//}
//
//size_t odbcx::col_count(std::vector<char> const& /*buffer*/, Bindings const& bindings)
//{
//    return col_count(bindings);
//}
//
//size_t odbcx::row_count(std::vector<char> const& buffer, Bindings const& bindings)
//{
//    assert(buffer.size() % bindings.second == 0 && "invalid buffer");
//    return buffer.size() / bindings.second;
//}
//
//std::pair<void const*, size_t> odbcx::get(size_t row, size_t col, std::vector<char> const& buffer, Bindings const& bindings)
//{
//    assert(row_count(buffer, bindings) > row && "invalid row");
//    assert(bindings.first.size() > col && "invalid column");
//    auto col_offset = std::get<size_t>(bindings.first[col]);
//    return std::make_pair(buffer.data() + bindings.second * row + col_offset, (col + 1 == bindings.first.size() ? bindings.second : std::get<size_t>(bindings.first[col + 1])) - col_offset);
//}
//
//size_t odbcx::exec_simple_query(odbcx::handle::Dbc& sqlbc, SQLCHAR* query, SQLINTEGER len /*= SQL_NTS*/)
//{
//	auto sqlstmt = odbcx::handle::allocate<SQL_HANDLE_STMT>(sqlbc);
//	odbcx::call(&SQLSetStmtAttr, sqlstmt, SQL_ATTR_ENABLE_AUTO_IPD, (SQLPOINTER)SQL_TRUE, 0);
//	odbcx::call(&SQLExecDirect, sqlstmt, query, len);
//	SQLINTEGER res;
//	SQLLEN resLen;
//	odbcx::call(&SQLBindCol, sqlstmt, 1, SQL_C_ULONG, &res, 0, &resLen);
//	odbcx::call(&SQLFetch, sqlstmt);
//	return res;
//}

