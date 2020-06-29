#include "connections.hpp"
//#include <boost/spirit/home/x3.hpp>
#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <boost/fusion/adapted/std_tuple.hpp>
#include <boost/format.hpp>
#include <boost/test/unit_test.hpp>
#include <iostream>
#include <algorithm>
#include <stdexcept>

//namespace x3 = boost::spirit::x3;
namespace qi = boost::spirit::qi;

namespace {

std::vector<std::string> connection_strings()
{
	std::string connection_string;
	auto description = boost::program_options::options_description{ "options" };
	description.add_options()
		("connection-string,c", boost::program_options::value<std::string>(&connection_string)
			//				->default_value("Driver={ODBC Driver 13 for SQL Server};server=localhost;database=test;trusted_connection=Yes"), "connection string")
			->default_value("\"Driver={ODBC Driver 13 for SQL Server};server=(local);database=test;UID=sa;PWD=Password12!;Mars_connection=yes;MultipleActiveResultSets=true;\""), "connection string")
		;

	boost::program_options::positional_options_description positional;
	positional.add("connection-string", -1);

	boost::program_options::variables_map vm;
	boost::program_options::store(boost::program_options::command_line_parser(
		boost::unit_test::framework::master_test_suite().argc,
		boost::unit_test::framework::master_test_suite().argv).options(description).positional(positional).run(), vm);

	boost::program_options::notify(vm);

	//static auto part = x3::rule<struct part, std::string>{ "part" } = (x3::lit('"') >> (*(x3::char_ - '"')) >> '"')
	//	| (*(x3::char_ - ';'));

    static qi::rule<std::string::const_iterator, std::string()> part  = (qi::lit('"') >> (*(qi::char_ - '"')) >> '"')	
        | (*(qi::char_ - ';'));

	std::vector<std::string> res;
	auto begin = connection_string.cbegin();
	auto end = connection_string.cend();
	if (!qi/*x3*/::parse(begin, end, part % ';', res) || begin != end)
		throw std::runtime_error{ str(boost::format("invalid connection string %1%") % connection_string) };

	return res;
}

odbcx::handle::Env alloc_environment()
{
	auto env = odbcx::handle::allocate<SQL_HANDLE_ENV>();
#ifdef SQL_OV_ODBC3_80
	if (SQLSetEnvAttr(env.get(), SQL_ATTR_ODBC_VERSION, SQLPOINTER(SQL_OV_ODBC3_80), 0) == SQL_SUCCESS)
		return env;
#endif // SQL_OV_ODBC3_80
	odbcx::call(&SQLSetEnvAttr, env, SQL_ATTR_ODBC_VERSION, SQLPOINTER(SQL_OV_ODBC3), 0);
	return env;
}

std::string driver_info(odbcx::handle::Dbc const& dbc, SQLUSMALLINT type)
{
    auto size = SQLSMALLINT{};
    odbcx::call(&SQLGetInfo, dbc, type, nullptr, 0, &size);
    if (size == 0)
        return {};
    auto buffer = std::vector<char>(std::size_t(size) + 1);
    odbcx::call(&SQLGetInfo, dbc, type, reinterpret_cast<SQLPOINTER>(buffer.data()), SQLSMALLINT(buffer.size()), &size);
    return { buffer.data(), std::string::size_type(size) };
}

std::vector<test::Connection> init_connections()
{
	static auto environment = alloc_environment();

	auto res = std::vector<test::Connection>{};
	auto cs = connection_strings();
	for (auto connection_string : cs)
	{
		try
		{
			std::clog << "connecting " << connection_string << "..." << std::endl;
			//BOOST_TEST_MESSAGE("connecting " << connection_string << "...");
			auto dbc = odbcx::handle::allocate<SQL_HANDLE_DBC>(environment);
			auto connected = false;
			try { odbcx::connect(dbc, connection_string); connected = true; } catch (std::exception&){}
			if (!connected)
			{
				auto path = boost::filesystem::path{ connection_string };
				if (!boost::filesystem::exists(path))
				{
					auto filename = path;
					if (path.extension().empty())
						filename += ".dsn";

					path = "test";
					path /= "dsn";
					path /=  filename;
					if (!boost::filesystem::exists(path))
					{
						path = "dsn";
						path /= filename;
						if (!boost::filesystem::exists(path))
							continue;
					}
				}
				auto dsn = std::string{ "FILEDSN=" };
				dsn += boost::filesystem::system_complete(path).string();
				dsn += ';';
				connection_string = std::move(dsn);
				odbcx::connect(dbc, connection_string);
			}

			//static auto ver_part = boost::spirit::x3::rule<struct ver_part, unsigned int>{ "ver_part" } = x3::uint_ | x3::attr(0);
            static qi::rule<std::string::const_iterator, unsigned int()> ver_part =  qi::uint_ | qi::attr(0);

			auto version = ::driver_info(dbc, SQL_DRIVER_VER);
			auto version_begin = version.cbegin();
			auto driver_version = test::OdbcDriverVersion{0,0,0,0};
			qi/*x3*/::parse(version_begin, version.cend(), ver_part >> '.' >> ver_part >> '.' >> ver_part >> '.' >> ver_part >> '.', driver_version);

			struct KnownDriverParsers : /*x3::symbols<test::OdbcDriverType>*/
                                        qi::symbols<char, test::OdbcDriverType>
			{
				KnownDriverParsers()
				{
					add
						("msodbc", test::OdbcDriverType::mssql)
						("libmsodbc", test::OdbcDriverType::mssql)
						("sqlncli", test::OdbcDriverType::mssql)
						("SQORA", test::OdbcDriverType::oracle)
						("PSQL", test::OdbcDriverType::psql)
						("psql", test::OdbcDriverType::psql)
						("MYSQL", test::OdbcDriverType::mysql)
						("myodbc", test::OdbcDriverType::mysql)
						("libmyodbc", test::OdbcDriverType::mysql)
						;
				}
			} static known_drivers_parser;

			auto name = ::driver_info(dbc, SQL_DRIVER_NAME);
            std::clog << "connected[" << name << ':' << ::driver_info(dbc, SQL_DRIVER_VER) << "] ..." << std::endl;
			//BOOST_TEST_MESSAGE("connected [" << name << ":" << ::driver_info(dbc, SQL_DRIVER_VER) << "] ...");
			auto driver_type = test::OdbcDriverType::unknown;
			auto name_begin = name.cbegin();
			if (!qi/*x3*/::parse(name_begin, name.cend(), known_drivers_parser, driver_type))
				driver_type = test::OdbcDriverType::unknown;


			res.emplace_back(std::move(dbc), driver_type, std::move(driver_version));
		}
		catch (std::exception & e)
		{
			std::clog << connection_string << " connection failure : " << e.what() << std::endl;
			//BOOST_TEST_MESSAGE(connection_string << " connection failure : " << e.what());
		}
	}
	return res;
}

}// namespace {

std::vector<test::Connection> const& test::active_connection()
{
	static auto connections = init_connections();
	return connections;
}

std::ostream& std::operator << (std::ostream& out, test::OdbcDriverType const& type)
{
	switch (type)
	{
		case test::OdbcDriverType::unknown:
			out << "unknown";
			break;
		case test::OdbcDriverType::mssql:
			out << "mssql";
			break;
		case test::OdbcDriverType::oracle:
			out << "oracle";
			break;
		case test::OdbcDriverType::psql:
			out << "postgres";
			break;
		case test::OdbcDriverType::mysql:
			out << "mysql";
			break;
	}
	return out;
}

std::ostream& std::operator << (std::ostream& out, test::OdbcDriverVersion const& version)
{
	out << std::get<0>(version) << '.' << std::get<1>(version) << '.' << std::get<2>(version) << '.' << std::get<3>(version) << '.';
	return out;
}
