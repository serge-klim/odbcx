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
	BOOST_CHECK_EQUAL(odbcx::query_one<long>(dbc, "SELECT count(id) FROM test").get(), n.get());
	BOOST_CHECK_EQUAL(odbcx::query_one<long>(dbc, "SELECT count(id) FROM test WHERE target = 'tran fail'").get(), 0);
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
	BOOST_CHECK_EQUAL(odbcx::query_one<long>(dbc, "SELECT count(id) FROM test").get(), n.get() + 2);
	BOOST_CHECK_EQUAL(odbcx::query_one<long>(dbc, "SELECT count(id) FROM test WHERE target = 'tran ok'").get(), 2);
	odbcx::query(dbc, "DELETE FROM test WHERE target = 'tran ok'");
}

BOOST_AUTO_TEST_SUITE_END()