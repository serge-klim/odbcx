#include <boost/test/unit_test.hpp>
#include "tests.h"
#include "dd.h"
#include "odbcx/iterator.h"
#include "odbcx/query.h"
#include <iterator>
#include <string>
#include <vector>
#include <cstdint>


struct OpFixture : Fixture
{
	OpFixture()
	{
		odbcx::query(dbc, "DELETE FROM test_optional");
		auto ts = diversion::optional<SQL_TIMESTAMP_STRUCT>{};
		auto str = diversion::optional<std::string>{};
		auto n = diversion::optional<long>{};
		auto pb = diversion::optional< std::vector<std::uint8_t>>{};
		odbcx::query(dbc, "insert into test_optional (ts, target, messagetype, n, pb) values(?,?,?,?,?)", ts, str, str, n, pb);

		{
			SQL_TIMESTAMP_STRUCT ts;
			ts.year = 2018;
			ts.month = 12;
			ts.day = 15;
			ts.hour = 13;
			ts.minute = 21;
			ts.second = 0;
			ts.fraction = 1000;

			odbcx::query(dbc, "insert into test_optional (ts) values(?)", ts);
			odbcx::query(dbc, "insert into test_optional (target) values('target')");
			odbcx::query(dbc, "insert into test_optional (messagetype) values('messagetype')");
			odbcx::query(dbc, "insert into test_optional (n) values(101)");

			auto data = std::vector<std::uint8_t>(100 * 1024, 0xF);
			odbcx::query(dbc, "insert into test_optional (pb) values(?)", data);

		}
	}
};

BOOST_FIXTURE_TEST_SUITE(SelectOptionalTestSuite, OpFixture)

BOOST_AUTO_TEST_CASE(SelectOptionalTest)
{

	auto val = odbcx::query_one<long>(dbc, "SELECT count(id) FROM test_optional");
	BOOST_CHECK_EQUAL(!val, false);
	BOOST_CHECK_EQUAL(val.value(), 6);

	auto range = fetch_range(odbcx::select<data::TestOptional>{}.from("test_optional").order_by("id ASC").exec(dbc));
	auto data = std::vector<data::TestOptional>{ range.begin(), range.end() };
	BOOST_CHECK_EQUAL(data.size(), 6);

	BOOST_CHECK(!data[0].ts);
	BOOST_CHECK(!data[0].target);
	BOOST_CHECK(!data[0].messagetype);
	BOOST_CHECK(!data[0].n);
	BOOST_CHECK(data[0].pb.empty());

	BOOST_CHECK_EQUAL(!data[1].ts, false);
	BOOST_CHECK(!data[1].target);
	BOOST_CHECK(!data[1].messagetype);
	BOOST_CHECK(!data[1].n);
	BOOST_CHECK(data[1].pb.empty());


	BOOST_CHECK(!data[2].ts);
	BOOST_CHECK_EQUAL(!data[2].target, false);
	BOOST_CHECK(!data[2].messagetype);
	BOOST_CHECK(!data[2].n);
	BOOST_CHECK(data[2].pb.empty());
	auto const& target = data[2].target.value();
	BOOST_CHECK_EQUAL((std::string{cbegin(target), cend(target) }), "target");

	BOOST_CHECK(!data[3].ts);
	BOOST_CHECK(!data[3].target);
	BOOST_CHECK_EQUAL(!data[3].messagetype, false);
	BOOST_CHECK(!data[3].n);
	BOOST_CHECK(data[3].pb.empty());
	BOOST_CHECK_EQUAL(data[3].messagetype.value(), "messagetype");

	BOOST_CHECK(!data[4].ts);
	BOOST_CHECK(!data[4].target);
	BOOST_CHECK(!data[4].messagetype);
	BOOST_CHECK_EQUAL(!data[4].n, false);
	BOOST_CHECK(data[4].pb.empty());
	BOOST_CHECK_EQUAL(data[4].n.value(), 101);

	BOOST_CHECK(!data[5].ts);
	BOOST_CHECK(!data[5].target);
	BOOST_CHECK(!data[5].messagetype);
	BOOST_CHECK(!data[5].n);
	BOOST_CHECK_EQUAL(data[5].pb.empty(), false);
}



BOOST_AUTO_TEST_SUITE_END()