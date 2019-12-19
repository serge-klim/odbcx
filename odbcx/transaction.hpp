// Copyright (c) 2018-2019 Serge Klimov serge.klim@outlook.com

#pragma once
#include "handle.hpp"
#include <cassert>

namespace odbcx { inline namespace v1 {

void begin_transaction(handle::adapter::Dbc const& dbc);
void end_transaction(handle::adapter::Dbc const& dbc, SQLSMALLINT completionType = SQL_ROLLBACK);
bool autocommit_mode(handle::adapter::Dbc const& dbc);



class ScopedTransaction
{
	ScopedTransaction(ScopedTransaction const&) = delete;
	ScopedTransaction& operator=(ScopedTransaction const&) = delete;
public:
	ScopedTransaction(ScopedTransaction &&) = default;
	ScopedTransaction& operator=(ScopedTransaction &&) = default;

	ScopedTransaction(handle::adapter::Dbc dbc) : dbc_{ std::move(dbc) } { begin_transaction(dbc_); }
	~ScopedTransaction() 
	{
		if (handle::details::sqlhandle(dbc_) != SQL_NULL_HDBC)
		{
			try {
				end_transaction(dbc_, SQL_ROLLBACK);
			}
			catch (...)
			{
				assert(!"can't rollback transaction");
			}
		}
	}
	void commit() 
	{ 
		end_transaction(dbc_, SQL_COMMIT);
		dbc_ = handle::adapter::Dbc{ SQL_NULL_HDBC };
	}
private:
	handle::adapter::Dbc dbc_;
};

}/*inline namespace v1*/} //namespace odbcx