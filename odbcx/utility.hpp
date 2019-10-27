#pragma once
#ifdef _WIN32
#include <windows.h>
#endif // _WIN32
#include "details/diversion.hpp"
#include <sql.h> 
#include <sqlext.h>
#include <cassert>
#include <type_traits>
#include <stdexcept>
#include <memory>
#include <utility>
#include <vector>
#include <sstream>
#include <tuple>

namespace odbcx { inline namespace v0 {

    namespace handle { using HandleType = SQLSMALLINT; }

    namespace details {

		SQLRETURN IfFailedThrow(SQLRETURN sqlres, SQLHANDLE handle, SQLSMALLINT type);
        template<handle::HandleType T>
        SQLRETURN IfFailedThrow(SQLRETURN sqlres, SQLHANDLE handle) { return IfFailedThrow(sqlres, handle, T); }

		static constexpr size_t bindings_offset = sizeof(SQLLEN) + sizeof(SQLLEN) % alignof(SQLLEN); /*dummy offset to avoid 0 offset in SQLBindCol*/
		static constexpr size_t BinarySizeLimit = 8000;
    }// namespace details

    namespace handle {

        template<HandleType T>
        struct deleter
        {
            void operator()(SQLHANDLE handle) { SQLFreeHandle(T, handle); }
        };

        template<>
        struct deleter<SQL_HANDLE_STMT>
        {
            void operator()(SQLHANDLE handle) 
            { 
                SQLFreeStmt(handle, SQL_CLOSE);
                SQLFreeStmt(handle, SQL_UNBIND);
                SQLFreeStmt(handle, SQL_RESET_PARAMS);
                SQLFreeHandle(SQL_HANDLE_STMT, handle);
            }
        };

        namespace details {
            template<HandleType T>
            using Handle = std::unique_ptr<std::remove_pointer<SQLHANDLE>::type, deleter<T>>;
        } // namespace details

        using Env = details::Handle<SQL_HANDLE_ENV>;
        using Dbc = details::Handle<SQL_HANDLE_DBC>;
        using Stmt = details::Handle<SQL_HANDLE_STMT>;

        template<HandleType T, HandleType IT>
        details::Handle<T> allocate(SQLHANDLE input)
        {
            SQLHANDLE handle;
            odbcx::details::IfFailedThrow<IT>(SQLAllocHandle(T, input, &handle), input);
            return { handle, deleter<T>{} };
        }

		template<HandleType T>
		details::Handle<T> allocate(SQLHANDLE handle)
		{
			return { handle, deleter<T>{} };
		}

        template<HandleType T>
        details::Handle<T> allocate()
        {
            static_assert(T == SQL_HANDLE_ENV, "Only SQL_HANDLE_ENV can be allocated without input handle");
            SQLHANDLE handle;
            if(SQLAllocHandle(T, SQL_NULL_HANDLE, &handle) != SQL_SUCCESS)
                throw std::runtime_error("FATAL DB ERROR: Unable to allocate DB environment handle.");
            return { handle, deleter<T>{} };
        }

        template<HandleType T, HandleType In>
        details::Handle<T> allocate(details::Handle<In> const& input) { return allocate<T,In>(input.get()); }

    } //namespace handle

    template<typename F, handle::HandleType T, typename ...Args>
    SQLRETURN call(F* f, handle::details::Handle<T> const& h, Args&& ...args)
    {
        return details::IfFailedThrow<T>((*f)(h.get(), std::forward<Args>(args)...), h.get());
    }

    inline SQLRETURN connect(handle::Dbc const& dbc, diversion::string_view constring, SQLUSMALLINT driver_completion = SQL_DRIVER_NOPROMPT)
    {
        return odbcx::call(&SQLDriverConnect, dbc, nullptr, reinterpret_cast<SQLCHAR*>(const_cast<char*>(constring.data())), SQLSMALLINT(constring.size()), nullptr, 0, nullptr, driver_completion);
    }
    
    inline SQLRETURN SQLSetEnvAttr(handle::Env const& env, SQLINTEGER Attribute, SQLPOINTER Value, SQLINTEGER StringLength)
    {
        return odbcx::call(&::SQLSetEnvAttr, env, Attribute, Value, StringLength);
    }

    inline SQLRETURN SQLSetEnvAttr(handle::Env const& env, SQLINTEGER Attribute, int value)
    {
        return SQLSetEnvAttr(env, Attribute, SQLPOINTER(value), 0);
    }

	inline SQLRETURN SQLSetStmtAttr(SQLHSTMT sqlstmt, SQLINTEGER attribute, SQLPOINTER value, SQLINTEGER string_length = 0)
	{
		return details::IfFailedThrow(::SQLSetStmtAttr(sqlstmt, attribute, value, string_length), sqlstmt, SQL_HANDLE_STMT);
	}
	
	inline SQLRETURN SQLSetStmtAttr(handle::Stmt const& sqlstmt, SQLINTEGER attribute, SQLPOINTER value, SQLINTEGER string_length = 0)
	{
		return SQLSetStmtAttr(sqlstmt.get(), attribute, value, string_length);
	}
	namespace details {
		inline handle::Stmt allocate_statement_handle(handle::Dbc const& dbc)
		{
			auto stmt = handle::allocate<SQL_HANDLE_STMT>(dbc);
		//  SQLSetStmtAttr(stmt, SQL_ATTR_CURSOR_TYPE, SQLPOINTER(SQL_CURSOR_KEYSET_DRIVEN), SQL_IS_INTEGER);
			SQLSetStmtAttr(stmt, SQL_ATTR_CONCURRENCY, SQLPOINTER(SQL_CONCUR_READ_ONLY), SQL_IS_INTEGER);
			//SQLSetStmtAttr(stmt, SQL_ATTR_CURSOR_SENSITIVITY, SQLPOINTER(SQL_SENSITIVE), SQL_IS_INTEGER);
			//SQLSetStmtAttr(stmt, SQL_ATTR_CURSOR_SCROLLABLE, SQLPOINTER(SQL_SCROLLABLE), SQL_IS_INTEGER);
			return stmt;
		}
	} // namespace details

	void begin_transaction(handle::Dbc const& dbc);
	void end_transaction(handle::Dbc const& dbc, SQLSMALLINT completionType = SQL_ROLLBACK);
	void begin_transaction(SQLHANDLE dbc);
	void end_transaction(SQLHANDLE dbc, SQLSMALLINT completionType = SQL_ROLLBACK);
	bool autocommit_mode(handle::Dbc const& dbc);
	bool autocommit_mode(SQLHANDLE dbc);


	template<typename T>
	std::vector<T> read_data(handle::Stmt const& stmt, SQLUSMALLINT column, SQLSMALLINT type)
	{
		std::size_t chunk_size = 1024;
		auto buffer = std::vector<T>(chunk_size);
		SQLLEN read = 0;
		//https://docs.microsoft.com/en-us/sql/odbc/reference/develop-app/getting-long-data?view=sql-server-2017
		if (call(&SQLGetData, stmt, column, type, buffer.data(), buffer.size() * sizeof(T), &read) == SQL_NO_DATA)
			throw std::runtime_error("SQLGetData can be called once only");
		if (read == SQL_NULL_DATA)
			return {};

		std::size_t total = 0;
		if (read == SQL_NO_TOTAL || std::size_t(read) == chunk_size)
		{
			//well have to read it chunk by chunk:(
			do {
				if (read == SQL_NO_TOTAL)
					read = chunk_size;
				total += read;
				if (read != SQLLEN(chunk_size))
				{
					assert(read < SQLLEN(chunk_size));
					assert(((read = 0), "emulating SQLGetData(...) == SQL_NO_DATA and read == 0 to pass next assert"));
					break;
				}
				buffer.resize(total + chunk_size);
				read = 0;
			} while (call(&SQLGetData, stmt, column, type, buffer.data() + total / sizeof(T), buffer.size() * sizeof(T) - total, &read) != SQL_NO_DATA);
			assert(read == 0);
		}
		else
		{
			total = std::size_t(read);
			if (read > SQLLEN(chunk_size)) // driver just returns size of data, so read it one go
			{
				buffer.resize(total / sizeof(T));
				read = 0;
				call(&SQLGetData, stmt, column, type, buffer.data() + chunk_size / sizeof(T), buffer.size() * sizeof(T) - chunk_size, &read);
				assert(read + chunk_size == total);
			}
			else
			{
				//seems that size of data is less than chunk_size, done here...
				assert(read > 0 && read < SQLLEN(chunk_size));
			}
		}
		buffer.resize(total / sizeof(T));
		return buffer;
	}
}/*inline namespace v0*/} //namespace odbcx
