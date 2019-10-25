#define BOOST_TEST_MODULE odbcx_test_suite
#include <boost/test/unit_test.hpp>
#include "odbcx/utility.hpp"



BOOST_AUTO_TEST_CASE(InvlalidHandlesErrorTest)
{
    auto env = odbcx::handle::Env{ SQLHENV(0) , odbcx::handle::deleter<SQL_HANDLE_ENV>{} };
    BOOST_CHECK_THROW(odbcx::handle::allocate<SQL_HANDLE_DBC>(env), std::runtime_error);
    BOOST_CHECK_THROW(odbcx::call(&SQLSetEnvAttr, env, SQL_ATTR_ODBC_VERSION, SQLPOINTER(SQL_OV_ODBC3), 0), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(ConnectionTest)
{
	auto env = odbcx::handle::allocate<SQL_HANDLE_ENV>();
	BOOST_CHECK_THROW(odbcx::call(&SQLSetEnvAttr, env, SQL_ATTR_ODBC_VERSION, SQLPOINTER(SQL_OV_ODBC3 + 100), 0), std::runtime_error);
	odbcx::call(&SQLSetEnvAttr, env, SQL_ATTR_ODBC_VERSION, SQLPOINTER(SQL_OV_ODBC3), 0);
}
