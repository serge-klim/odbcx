#pragma once
#include <boost/test/unit_test.hpp>
#include <boost/program_options.hpp>
#include "odbcx/utility.h"
#include <cassert>


//-- - c "Driver={ODBC Driver 13 for SQL Server};server=localhost;database=test;trusted_connection=Yes;"

struct Fixture
{
	Fixture() : dbc{ odbcx::handle::allocate<SQL_HANDLE_DBC>(Environment()) }
	{
		std::string constring;
		auto description = boost::program_options::options_description{ "options" };
		description.add_options()
			("cs,c", boost::program_options::value<std::string>(&constring)
//				->default_value("Driver={ODBC Driver 13 for SQL Server};server=localhost;database=test;trusted_connection=Yes"), "connection string")
				->default_value("Driver={ODBC Driver 13 for SQL Server};server=(local);database=test;UID=sa;PWD=Password12!;Mars_connection=yes;MultipleActiveResultSets=true;"), "connection string")
			;

		boost::program_options::variables_map vm;
		boost::program_options::store(boost::program_options::parse_command_line(
											boost::unit_test::framework::master_test_suite().argc, 
											boost::unit_test::framework::master_test_suite().argv, description), vm);
		boost::program_options::notify(vm);

		auto res = odbcx::call(&SQLDriverConnect, dbc, nullptr, 
									reinterpret_cast<SQLCHAR*>(const_cast<char*>(constring.data())), SQLSMALLINT(constring.length()),
									nullptr, 0, nullptr, SQL_DRIVER_NOPROMPT);
		//BOOST_CHECK_NE(res, SQL_NO_DATA);
		assert(res != SQL_NO_DATA);
	}

	odbcx::handle::Dbc dbc;
private:
	static odbcx::handle::Env AllocEnvironment()
	{
		auto env = odbcx::handle::allocate<SQL_HANDLE_ENV>();
		odbcx::call(&SQLSetEnvAttr, env, SQL_ATTR_ODBC_VERSION, SQLPOINTER(SQL_OV_ODBC3_80), 0);
		odbcx::call(&SQLSetEnvAttr, env, SQL_ATTR_CONNECTION_POOLING, SQLPOINTER(SQL_CP_ONE_PER_DRIVER), SQL_IS_UINTEGER);
		return env;
	}

	static odbcx::handle::Env const& Environment()
	{
		static auto env = AllocEnvironment();
		return env;
	}
};
