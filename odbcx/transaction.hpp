// Copyright (c) 2018-2019 Serge Klimov serge.klim@outlook.com

#pragma once
#include "utility.hpp"
#include <cassert>

namespace odbcx { inline namespace v0 {

class ScopedTransaction
{
	ScopedTransaction(ScopedTransaction const&) = delete;
	ScopedTransaction& operator=(ScopedTransaction const&) = delete;
public:
	ScopedTransaction(ScopedTransaction &&) = default;
	ScopedTransaction& operator=(ScopedTransaction &&) = default;

	ScopedTransaction(SQLHANDLE dbc) : dbc_{ dbc } { begin_transaction(dbc_); }
	ScopedTransaction(handle::Dbc const& dbc) : ScopedTransaction{ dbc.get() } {}
	~ScopedTransaction() 
	{
		if (dbc_ != SQL_NULL_HDBC)
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
		dbc_ = SQL_NULL_HDBC;
	}
private:
	SQLHANDLE dbc_;
};

}/*inline namespace v0*/} //namespace odbcx