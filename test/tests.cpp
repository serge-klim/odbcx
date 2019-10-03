#define BOOST_TEST_MODULE DBStuffTests
#include <boost/test/unit_test.hpp>
#include "odbcx/utility.hpp"


BOOST_AUTO_TEST_CASE(ConnectionTestTest)
{
	auto env = odbcx::handle::allocate<SQL_HANDLE_ENV>();
	BOOST_CHECK_THROW(odbcx::call(&SQLSetEnvAttr, env, SQL_ATTR_ODBC_VERSION, SQLPOINTER(SQL_OV_ODBC3 + 100), 0), std::runtime_error);
	odbcx::call(&SQLSetEnvAttr, env, SQL_ATTR_ODBC_VERSION, SQLPOINTER(SQL_OV_ODBC3), 0);
}
