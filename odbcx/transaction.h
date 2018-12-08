#pragma once
#include "utility.h"

namespace odbcx {

class ScopedTransaction
{
	ScopedTransaction(ScopedTransaction const&) = delete;
	ScopedTransaction& operator=(ScopedTransaction const&) = delete;
public:
	ScopedTransaction(ScopedTransaction &&) = default;
	ScopedTransaction& operator=(ScopedTransaction &&) = default;

	ScopedTransaction(SQLHANDLE dbc) : dbc_{ dbc } 
	{ 
		begin_transaction(dbc_);
	}
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

} //namespace odbcx