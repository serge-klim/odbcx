// Copyright (c) 2018-2019 Serge Klimov serge.klim@outlook.com

#include "transaction.hpp"


void odbcx::v1::begin_transaction(handle::adapter::Dbc const& dbc)
{
	assert(autocommit_mode(dbc));
	handle::set_attribute(dbc, SQL_ATTR_AUTOCOMMIT, reinterpret_cast<SQLPOINTER>(SQL_AUTOCOMMIT_OFF));
}

void odbcx::v1::end_transaction(handle::adapter::Dbc const& dbc, SQLSMALLINT completionType /*= SQL_ROLLBACK*/)
{
	assert(!autocommit_mode(dbc));
	if_failed_throw<SQL_HANDLE_DBC>(::SQLEndTran(SQL_HANDLE_DBC, handle::details::sqlhandle(dbc), completionType), handle::details::sqlhandle(dbc));
	handle::set_attribute(dbc, SQL_ATTR_AUTOCOMMIT, reinterpret_cast<SQLPOINTER>(SQL_AUTOCOMMIT_ON));
}

bool odbcx::v1::autocommit_mode(handle::adapter::Dbc const& dbc)
{
	return handle::get_attribute<SQLULEN>(dbc, SQL_ATTR_AUTOCOMMIT) == SQL_AUTOCOMMIT_ON;
}


