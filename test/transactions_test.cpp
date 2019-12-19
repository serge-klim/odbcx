#include "test.hpp"
#include "odbcx/query.hpp"
#include "odbcx/transaction.hpp"
#include <boost/format.hpp>
#include <boost/test/unit_test.hpp>
#include <string>
#include <cstdint>

BOOST_AUTO_TEST_SUITE(transaction_test_suite)

BOOST_DATA_TEST_CASE(SopedTransactionAutoRollbackTest, boost::unit_test::data::make_delayed<test::AnyDBConnection>(), dbc, driver_type, driver_version)
{
    test::create_table(dbc, "transaction_test");
	auto n = odbcx::query_one<long>(dbc, "SELECT count(type) FROM transaction_test");
	BOOST_CHECK_EQUAL(!n, false);
	{
		auto transaction = odbcx::ScopedTransaction{ dbc };
		odbcx::query(dbc, "insert into transaction_test (target, type) values(?,?)", "tran fail", "part 1");
		odbcx::query(dbc, "insert into transaction_test (target, type) values(?,?)", "tran fail", "part 2");
	}	
	BOOST_CHECK_EQUAL(odbcx::query_one<long>(dbc, "SELECT count(type) FROM transaction_test").value(), n.value());
	BOOST_CHECK_EQUAL(odbcx::query_one<long>(dbc, "SELECT count(type) FROM transaction_test WHERE target = 'tran fail'").value(), 0);

	BOOST_CHECK(odbcx::autocommit_mode(dbc));
}

BOOST_DATA_TEST_CASE(SopedTransactionCommitTest, boost::unit_test::data::make_delayed<test::AnyDBConnection>(), dbc, driver_type, driver_version)
{
	test::create_table(dbc, "transaction_test");
	auto n = odbcx::query_one<long>(dbc, "SELECT count(type) FROM transaction_test");
	BOOST_CHECK_EQUAL(!n, false);
	{
		auto transaction = odbcx::ScopedTransaction{ dbc };
		odbcx::query(dbc, "insert into transaction_test (target, type) values(?,?)", "tran ok", "part 1");
		odbcx::query(dbc, "insert into transaction_test (target, type) values(?,?)", "tran ok", "part 2");
		transaction.commit();
	}
	BOOST_CHECK_EQUAL(odbcx::query_one<long>(dbc, "SELECT count(type) FROM transaction_test").value(), n.value() + 2);
	BOOST_CHECK_EQUAL(odbcx::query_one<long>(dbc, "SELECT count(type) FROM transaction_test WHERE target = 'tran ok'").value(), 2);

	BOOST_CHECK(odbcx::autocommit_mode(dbc));
}

BOOST_AUTO_TEST_SUITE_END()