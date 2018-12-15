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
	BOOST_CHECK_EQUAL(std::get<0>(val.value()) + 100, std::get<1>(val.value()));
}

BOOST_AUTO_TEST_CASE(SimpleQueryOneStructWithParamTest)
{
	int xtra = 100;
	auto val = odbcx::query_one<std::tuple<long, long>>(dbc, "SELECT count(id), count(id) + ? FROM test", xtra);
	BOOST_CHECK_EQUAL(!val, false);
	BOOST_CHECK_EQUAL(std::get<0>(val.value()) + xtra, std::get<1>(val.value()));
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

BOOST_AUTO_TEST_CASE(InputParametersEmptyTest)
{
	auto empty = std::string{};
	auto data = std::vector<std::uint8_t>{};
	odbcx::query(dbc, "insert into test (target, messagetype, pb) values(?,?,?)", empty, empty, data);
}

BOOST_AUTO_TEST_CASE(InputParametersEmptyOptionalTest)
{
	auto ts = diversion::optional<SQL_TIMESTAMP_STRUCT>{};
	auto str = diversion::optional<std::string>{};
	auto n = diversion::optional<long>{};
	auto pb = diversion::optional< std::vector<std::uint8_t>>{};
	odbcx::query(dbc, "insert into test (ts, target, messagetype, n, pb) values(?,?,?,?,?)", ts, str, str, n, pb);
}

BOOST_AUTO_TEST_CASE(InputParametersOptionalTest)
{
	SQL_TIMESTAMP_STRUCT ts;
	ts.year = 2018;
	ts.month = 12;
	ts.day = 15;
	ts.hour = 13;
	ts.minute = 21;
	ts.second = 0;
	ts.fraction = 1000;

	auto ots = diversion::make_optional(ts);
	auto target = diversion::make_optional(std::string{"optional-target"});
	auto messagetype = diversion::make_optional(std::string{ "optional-messagetype" });
	auto n = diversion::make_optional(0xabcd);
	auto pb = diversion::make_optional(std::vector<std::uint8_t>(11 * 1024, 0xF));
	odbcx::query(dbc, "insert into test (ts, target, messagetype, n, pb) values(?,?,?,?,?)", ots, target, messagetype, n, pb);
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

BOOST_AUTO_TEST_CASE(InputParametersXTest)
{
	SQL_TIMESTAMP_STRUCT ts;
	ts.year = 2018;
	ts.month = 12;
	ts.day = 7;
	ts.hour = 05;
	ts.minute = 21;
	ts.second = 0;
	ts.fraction = 0;
	auto message = std::string{ "message" };
	auto data = std::vector<std::uint8_t>(100 * 1024, 0xF);
	odbcx::query(dbc, "insert into test (ts, target, messagetype, N, pb) values(?,?,?,?,?)", ts, "target", message, ts.day, data);
}


BOOST_AUTO_TEST_SUITE_END()