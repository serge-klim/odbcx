#pragma once
#include <boost/test/unit_test.hpp>
#include <boost/program_options.hpp>
#include "odbcx/utility.hpp"
#include <cassert>

class GlobalFixture
{
public:
	GlobalFixture()
	{
		auto description = boost::program_options::options_description{ "options" };
		description.add_options()
			("connection-string,c", boost::program_options::value<std::string>(&connection_string_)
				//				->default_value("Driver={ODBC Driver 13 for SQL Server};server=localhost;database=test;trusted_connection=Yes"), "connection string")
				->default_value("Driver={ODBC Driver 13 for SQL Server};server=(local);database=test;UID=sa;PWD=Password12!;Mars_connection=yes;MultipleActiveResultSets=true;"), "connection string")
				("dsn-file", boost::program_options::value<std::string>(), "dsn file")
			;

		boost::program_options::positional_options_description positional;
		positional.add("dsn-file", -1);

		boost::program_options::variables_map vm;
		boost::program_options::store(boost::program_options::command_line_parser(
			boost::unit_test::framework::master_test_suite().argc,
			boost::unit_test::framework::master_test_suite().argv).options(description).positional(positional).run(), vm);

		boost::program_options::notify(vm);
		if (vm.count("dsn-file"))
		{
			connection_string_ = "FILEDSN=";
			connection_string_ += vm["dsn-file"].as<std::string>();
			connection_string_ += ';';
		}
	}
	std::string const& connection_string() const { return connection_string_; }
private:
	std::string connection_string_;
};

struct Fixture
{
	Fixture() : dbc{ odbcx::handle::allocate<SQL_HANDLE_DBC>(Environment()) }
	{
		static auto gfixture = GlobalFixture{};
		//BOOST_TEST_MESSAGE("connecting \"" << connection_string_.data() << "\" ...");
        auto res = odbcx::connect(dbc, gfixture.connection_string());
        //auto const& connection_string = gfixture.connection_string();
		//auto res = odbcx::call(&SQLDriverConnect, dbc, nullptr, 
		//							reinterpret_cast<SQLCHAR*>(const_cast<char*>(connection_string.data())), SQLSMALLINT(connection_string.length()),
		//							nullptr, 0, nullptr, SQL_DRIVER_NOPROMPT);
		//BOOST_CHECK_NE(res, SQL_NO_DATA);
		assert(res != SQL_NO_DATA);
	}

	odbcx::handle::Dbc dbc;
private:
	static odbcx::handle::Env AllocEnvironment()
	{
		auto env = odbcx::handle::allocate<SQL_HANDLE_ENV>();
#ifdef SQL_OV_ODBC3_80
		if (SQLSetEnvAttr(env.get(), SQL_ATTR_ODBC_VERSION, SQLPOINTER(SQL_OV_ODBC3_80), 0) == SQL_SUCCESS)
			return env;
#endif // SQL_OV_ODBC3_80
		odbcx::call(&SQLSetEnvAttr, env, SQL_ATTR_ODBC_VERSION, SQLPOINTER(SQL_OV_ODBC3), 0);
		return env;
	}

	static odbcx::handle::Env const& Environment()
	{
		static auto env = AllocEnvironment();
		return env;
	}
};
