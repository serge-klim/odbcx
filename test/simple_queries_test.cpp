#include <boost/test/unit_test.hpp>
#include "tests.h"
#include "odbcx/odbcx.h"
#include <string>
#include <vector>
#include <cstdint>

BOOST_FIXTURE_TEST_SUITE(SimpleQueriesTestSuite, Fixture)

BOOST_AUTO_TEST_CASE(SimpleQueryOneTest)
{
	auto val = odbcx::query_one<long>(dbc, "SELECT count(id) FROM test");
	BOOST_CHECK_EQUAL(!val, false);
}

BOOST_AUTO_TEST_CASE(SimpleQueryOneStructTest)
{
	auto val = odbcx::query_one<std::tuple<long, long>>(dbc, "SELECT count(id), count(id) + 100 FROM test");
	BOOST_CHECK_EQUAL(!val, false);
	BOOST_CHECK_EQUAL(std::get<0>(val.get()) + 100, std::get<1>(val.get()));
}

BOOST_AUTO_TEST_CASE(SimpleQueryOneStructWithParamTest)
{
	int xtra = 100;
	auto val = odbcx::query_one<std::tuple<long, long>>(dbc, "SELECT count(id), count(id) + ? FROM test", xtra);
	BOOST_CHECK_EQUAL(!val, false);
	BOOST_CHECK_EQUAL(std::get<0>(val.get()) + xtra, std::get<1>(val.get()));
}

BOOST_AUTO_TEST_CASE(InputParametersSimpleTest)
{
	odbcx::query(dbc, "insert into test (target, messagetype) values(?,?)", "hi", "there");
}

BOOST_AUTO_TEST_CASE(InputParametersSimple1Test)
{
	auto target = std::string{ "test" };
	int n = 500;
	odbcx::query(dbc, "insert into test (target, messagetype, n) values(?,?,?)", target, "...", n);
}

BOOST_AUTO_TEST_CASE(InputParametersLongBinTest)
{
	auto data = std::vector<std::uint8_t>( 100 * 1024, 0xF );
	auto target = std::string{ "test" };
	odbcx::query(dbc, "insert into test (target, messagetype, pb) values(?,?,?)", target, "long  binary", data);
}

BOOST_AUTO_TEST_CASE(InputParametersBinTest)
{
	auto data = std::vector<std::uint8_t>(10, 0xA);
	odbcx::query(dbc, "insert into test (target, messagetype, pb) values(?,?,?)", "test","binary", data);
}

BOOST_AUTO_TEST_CASE(InputParametersTimestampTest)
{
	SQL_TIMESTAMP_STRUCT ts;
	ts.year = 2018;
	ts.month = 12;
	ts.day = 7;
	ts.hour = 05;
	ts.minute = 21;
	ts.second = 0;
	ts.fraction = 1000;
	odbcx::query(dbc, "insert into test (ts, target) values(?,?)", ts ,"timestamp");
}


BOOST_AUTO_TEST_SUITE_END()