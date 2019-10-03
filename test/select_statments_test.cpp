#include <boost/test/unit_test.hpp>
#include "tests.hpp"
#include "dd.hpp"
#include "odbcx/query.hpp"
#include <iterator>
#include <string>
#include <vector>
#include <cstdint>


BOOST_FIXTURE_TEST_SUITE(SelectStatementsTestSuite, Fixture)

BOOST_AUTO_TEST_CASE(SelectStatementStaticBindTest)
{
	auto statement = odbcx::select<data::TestStatic>{}.from("test").exec(dbc);
	auto recordset = statement.fetch();
	BOOST_CHECK(!recordset.empty());
	BOOST_CHECK_EQUAL(std::distance(recordset.cbegin(), recordset.cend()), recordset.size());
	BOOST_CHECK_EQUAL(std::distance(begin(recordset), cend(recordset)), recordset.size());
	BOOST_CHECK_EQUAL(std::distance(cbegin(recordset), cend(recordset)), recordset.size());
}

BOOST_AUTO_TEST_CASE(SelectStatementAsStaticBindTest)
{
	auto statement = odbcx::select<data::TestStatic>{}.from("test").as("alias").exec(dbc);
	auto recordset = statement.fetch();
	BOOST_CHECK(!recordset.empty());
	BOOST_CHECK_EQUAL(std::distance(recordset.cbegin(), recordset.cend()), recordset.size());
	BOOST_CHECK_EQUAL(std::distance(begin(recordset), cend(recordset)), recordset.size());
	BOOST_CHECK_EQUAL(std::distance(cbegin(recordset), cend(recordset)), recordset.size());
}

BOOST_AUTO_TEST_CASE(SelectStatementAsWhereStaticBindTest)
{
	auto statement = odbcx::select<data::TestStatic>{}.from("test").as("alias").where("alias.target='test'").exec(dbc);
	auto recordset = statement.fetch();
	BOOST_CHECK(!recordset.empty());
	BOOST_CHECK_EQUAL(std::distance(recordset.cbegin(), recordset.cend()), recordset.size());
	BOOST_CHECK_EQUAL(std::distance(begin(recordset), cend(recordset)), recordset.size());
	BOOST_CHECK_EQUAL(std::distance(cbegin(recordset), cend(recordset)), recordset.size());
}

BOOST_AUTO_TEST_CASE(SelectStatementWhereWithParamStaticBindTest)
{
	auto test = std::string{ "test" };
	auto statement = odbcx::select<data::TestStatic>{}.from("test").where("target=?", test).exec(dbc);
	auto recordset = statement.fetch();
	BOOST_CHECK(!recordset.empty());
	BOOST_CHECK_EQUAL(std::distance(recordset.cbegin(), recordset.cend()), recordset.size());
	BOOST_CHECK_EQUAL(std::distance(begin(recordset), cend(recordset)), recordset.size());
	BOOST_CHECK_EQUAL(std::distance(cbegin(recordset), cend(recordset)), recordset.size());
}

BOOST_AUTO_TEST_CASE(SelectStatementAsWhereWithParamStaticBindTest)
{
	auto test = std::string{ "test" };
	auto statement = odbcx::select<data::TestStatic>{}.from("test").as("alias").where("alias.target=?", test).exec(dbc);
	auto recordset = statement.fetch();
	BOOST_CHECK(!recordset.empty());
	BOOST_CHECK_EQUAL(std::distance(recordset.cbegin(), recordset.cend()), recordset.size());
	BOOST_CHECK_EQUAL(std::distance(begin(recordset), cend(recordset)), recordset.size());
	BOOST_CHECK_EQUAL(std::distance(cbegin(recordset), cend(recordset)), recordset.size());
}

BOOST_AUTO_TEST_CASE(SelectStatementDynamicBindTest)
{
	auto statement = odbcx::select<data::Test>{}.from("test").exec(dbc);
	auto recordset = statement.fetch();
	BOOST_CHECK(!recordset.empty());
	BOOST_CHECK_EQUAL(std::distance(recordset.cbegin(), recordset.cend()), recordset.size());
	BOOST_CHECK_EQUAL(std::distance(begin(recordset), cend(recordset)), recordset.size());
	BOOST_CHECK_EQUAL(std::distance(cbegin(recordset), cend(recordset)), recordset.size());
}

BOOST_AUTO_TEST_CASE(SelectStatementAsDynamicBindTest)
{
	auto statement = odbcx::select<data::Test>{}.from("test").as("alias").exec(dbc);
	auto recordset = statement.fetch();
	BOOST_CHECK(!recordset.empty());
	BOOST_CHECK_EQUAL(std::distance(recordset.cbegin(), recordset.cend()), recordset.size());
	BOOST_CHECK_EQUAL(std::distance(begin(recordset), cend(recordset)), recordset.size());
	BOOST_CHECK_EQUAL(std::distance(cbegin(recordset), cend(recordset)), recordset.size());
}

BOOST_AUTO_TEST_CASE(SelectStatementAsWhereDynamicBindTest)
{
	auto statement = odbcx::select<data::Test>{}.from("test").as("alias").where("alias.target='test'").exec(dbc);
	auto recordset = statement.fetch();
	BOOST_CHECK(!recordset.empty());
	BOOST_CHECK_EQUAL(std::distance(recordset.cbegin(), recordset.cend()), recordset.size());
	BOOST_CHECK_EQUAL(std::distance(begin(recordset), cend(recordset)), recordset.size());
	BOOST_CHECK_EQUAL(std::distance(cbegin(recordset), cend(recordset)), recordset.size());
}

BOOST_AUTO_TEST_CASE(SelectStatementWhereWithParamDynamicBindTest)
{
	auto test = std::string{ "test" };
	auto statement = odbcx::select<data::Test>{}.from("test").where("target=?", test).exec(dbc);
	auto recordset = statement.fetch();
	BOOST_CHECK(!recordset.empty());
	BOOST_CHECK_EQUAL(std::distance(recordset.cbegin(), recordset.cend()), recordset.size());
	BOOST_CHECK_EQUAL(std::distance(begin(recordset), cend(recordset)), recordset.size());
	BOOST_CHECK_EQUAL(std::distance(cbegin(recordset), cend(recordset)), recordset.size());
}

BOOST_AUTO_TEST_CASE(SelectStatementAsWhereWithParamDynamicBindTest)
{
	auto test = std::string{ "test" };
	auto statement = odbcx::select<data::Test>{}.from("test").as("alias").where("alias.target=?", test).exec(dbc);
	auto recordset = statement.fetch();
	BOOST_CHECK(!recordset.empty());
	BOOST_CHECK_EQUAL(std::distance(recordset.cbegin(), recordset.cend()), recordset.size());
	BOOST_CHECK_EQUAL(std::distance(begin(recordset), cend(recordset)), recordset.size());
	BOOST_CHECK_EQUAL(std::distance(cbegin(recordset), cend(recordset)), recordset.size());
}


BOOST_AUTO_TEST_SUITE_END()