#include <boost/test/unit_test.hpp>
#include "tests.h"
#include "odbcx/odbcx.h"
#include "odbcx/transaction.h"
#include <string>
#include <vector>
#include <cstdint>

BOOST_FIXTURE_TEST_SUITE(TransactionTestSuite, Fixture)

BOOST_AUTO_TEST_CASE(SopedTransactionAutoRollbackTest)
{
	odbcx::query(dbc, "DELETE FROM test WHERE target = 'tran fail'");
	auto n = odbcx::query_one<long>(dbc, "SELECT count(id) FROM test");
	BOOST_CHECK_EQUAL(!n, false);
	{
		auto transaction = odbcx::ScopedTransaction{ dbc };
		odbcx::query(dbc, "insert into test (target, messagetype) values(?,?)", "tran fail", "part 1");
		odbcx::query(dbc, "insert into test (target, messagetype) values(?,?)", "tran fail", "part 2");
	}	
	BOOST_CHECK_EQUAL(odbcx::query_one<long>(dbc, "SELECT count(id) FROM test").value(), n.value());
	BOOST_CHECK_EQUAL(odbcx::query_one<long>(dbc, "SELECT count(id) FROM test WHERE target = 'tran fail'").value(), 0);

	BOOST_CHECK(odbcx::autocommit_mode(dbc));
}

BOOST_AUTO_TEST_CASE(SopedTransactionCommitTest)
{
	odbcx::query(dbc, "DELETE FROM test WHERE target = 'tran ok'");
	auto n = odbcx::query_one<long>(dbc, "SELECT count(id) FROM test");
	BOOST_CHECK_EQUAL(!n, false);
	{
		auto transaction = odbcx::ScopedTransaction{ dbc };
		odbcx::query(dbc, "insert into test (target, messagetype) values(?,?)", "tran ok", "part 1");
		odbcx::query(dbc, "insert into test (target, messagetype) values(?,?)", "tran ok", "part 2");
		transaction.commit();
	}
	BOOST_CHECK_EQUAL(odbcx::query_one<long>(dbc, "SELECT count(id) FROM test").value(), n.value() + 2);
	BOOST_CHECK_EQUAL(odbcx::query_one<long>(dbc, "SELECT count(id) FROM test WHERE target = 'tran ok'").value(), 2);
	odbcx::query(dbc, "DELETE FROM test WHERE target = 'tran ok'");

	BOOST_CHECK(odbcx::autocommit_mode(dbc));
}

BOOST_AUTO_TEST_SUITE_END()