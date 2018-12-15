#include <boost/test/unit_test.hpp>
#include "tests.h"
#include "dd.h"
#include "odbcx/odbcx.h"
#include "odbcx/bindings/out.h"
#include "odbcx/details/statement.h"
#include "odbcx/query.h"
#include <boost/core/ignore_unused.hpp>

BOOST_FIXTURE_TEST_SUITE(BindingsTestSuite, Fixture)

BOOST_AUTO_TEST_CASE(StaticBindingsTest)
{
	static_assert(odbcx::details::out::IsSequenceStaticallyBindable<data::TestStatic>::value, "Oops! should be staticaly bindable");
	static_assert(odbcx::details::out::IsSequenceDynamicallyBindable<data::TestStatic>::value, "Oops! should be dynamically bindable");

	auto stmt = odbcx::handle::allocate<SQL_HANDLE_STMT>(dbc);
	//	odbcx::SQLSetStmtAttr(stmt, SQL_ATTR_CURSOR_TYPE, SQLPOINTER(SQL_CURSOR_KEYSET_DRIVEN), SQL_IS_INTEGER);
	odbcx::SQLSetStmtAttr(stmt, SQL_ATTR_CONCURRENCY, SQLPOINTER(SQL_CONCUR_READ_ONLY), SQL_IS_INTEGER);
	//odbcx::SQLSetStmtAttr(stmt, SQL_ATTR_CURSOR_SENSITIVITY, SQLPOINTER(SQL_SENSITIVE), SQL_IS_INTEGER);
	//odbcx::SQLSetStmtAttr(stmt, SQL_ATTR_CURSOR_SCROLLABLE, SQLPOINTER(SQL_SCROLLABLE), SQL_IS_INTEGER);
	auto statement = odbcx::StaticallyBindableStatement<data::TestStatic>{std::move(stmt), "select target, messagetype, pb from test"};
	auto recordset = statement.fetch();
	BOOST_CHECK(!recordset.empty());
	auto size = recordset.size();
	BOOST_CHECK_NE(size, 0);
	auto copy = std::vector<data::TestStatic> {cbegin(recordset), cend(recordset)};
	for (auto const& row : recordset)
	{
		BOOST_CHECK_NE(row.target[0], '0');
	}
	for (size_t i = 0; i != size; ++i)
	{
		auto const& row = recordset[i];
		BOOST_CHECK_NE(row.target[0], '0');
	}

	BOOST_CHECK(std::equal(cbegin(copy), cend(copy),
		cbegin(recordset), cend(recordset), [](auto const& l, auto const& r)
	{		
		return std::string{l.target} == std::string{ r.target }
					&& std::string{ l.messagetype } == std::string{ r.messagetype }
						&& std::equal(l.pb, l.pb + sizeof(l.pb)/ sizeof(l.pb[0]), r.pb, r.pb + sizeof(r.pb) / sizeof(r.pb[0]));
	}));

}

BOOST_AUTO_TEST_CASE(StaticBindingsQueryTest)
{
	auto statement = odbcx::query<data::TestStatic>(dbc, "select target, messagetype, pb from test");
	auto recordset = statement.fetch();
	BOOST_CHECK(!recordset.empty());
	auto size = recordset.size();
	BOOST_CHECK_NE(size, 0);
	auto copy = std::vector<data::TestStatic>{ cbegin(recordset), cend(recordset) };
	for (auto const& row : recordset)
	{
		BOOST_CHECK_NE(row.target[0], '0');
	}
	for (size_t i = 0; i != size; ++i)
	{
		auto const& row = recordset[i];
		BOOST_CHECK_NE(row.target[0], '0');
	}
	BOOST_CHECK(std::equal(cbegin(copy), cend(copy),
		cbegin(recordset), cend(recordset), [](auto const& l, auto const& r)
	{
		return std::string{ l.target } == std::string{ r.target }
			&& std::string{ l.messagetype } == std::string{ r.messagetype }
		&& std::equal(l.pb, l.pb + sizeof(l.pb) / sizeof(l.pb[0]), r.pb, r.pb + sizeof(r.pb) / sizeof(r.pb[0]));
	}));
}

BOOST_AUTO_TEST_CASE(StaticBindingsFetchOneTest)
{
	auto statement = odbcx::query<data::TestStatic>(dbc, "select target, messagetype, pb from test");
	auto val = statement.fetch_one();
	BOOST_CHECK_EQUAL(!val, false);
}

BOOST_AUTO_TEST_CASE(BindingsTest)
{
	static_assert(!odbcx::details::out::IsSequenceStaticallyBindable<data::Test>::value, "Oops! should be staticaly bindable");
	static_assert(odbcx::details::out::IsSequenceDynamicallyBindable<data::Test>::value, "Oops! should be dynamically bindable");

	auto stmt = odbcx::handle::allocate<SQL_HANDLE_STMT>(dbc);
	//	odbcx::SQLSetStmtAttr(stmt, SQL_ATTR_CURSOR_TYPE, SQLPOINTER(SQL_CURSOR_KEYSET_DRIVEN), SQL_IS_INTEGER);
	odbcx::SQLSetStmtAttr(stmt, SQL_ATTR_CONCURRENCY, SQLPOINTER(SQL_CONCUR_READ_ONLY), SQL_IS_INTEGER);
	//odbcx::SQLSetStmtAttr(stmt, SQL_ATTR_CURSOR_SENSITIVITY, SQLPOINTER(SQL_SENSITIVE), SQL_IS_INTEGER);
	//odbcx::SQLSetStmtAttr(stmt, SQL_ATTR_CURSOR_SCROLLABLE, SQLPOINTER(SQL_SCROLLABLE), SQL_IS_INTEGER);
	auto statement = odbcx::DynamicallyBindableStatement<data::Test>{std::move(stmt), "select id, ts, target, messagetype, pb from test"};
	BOOST_CHECK(!statement.bindings().bulk_fetch());
	auto recordset = statement.fetch();
	BOOST_CHECK(!recordset.empty());
	auto val = recordset[0];
	auto id = recordset.get<0>(0);
	boost::ignore_unused(id);
	auto ts = recordset.get<1>(0);
	auto target = recordset.get<2>(0);
	auto message_type = recordset.get<3>(0);
	BOOST_CHECK_THROW((recordset.get<4>(0)), std::runtime_error);
	//if (!val.pb.empty())
	//{
	//	BOOST_CHECK_THROW((recordset.get<3>(0)), std::runtime_error);
	//}
	//else
	//{
	//	BOOST_CHECK(recordset.get<3>(0).empty());
	//}

	BOOST_CHECK_EQUAL_COLLECTIONS(reinterpret_cast<char const*>(&ts), reinterpret_cast<char const*>(&ts) + sizeof(ts),
										reinterpret_cast<char const*>(&val.ts), reinterpret_cast<char const*>(&val.ts) + sizeof(val.ts));

	BOOST_CHECK_EQUAL_COLLECTIONS(cbegin(target), cend(target),
										cbegin(val.target), cend(val.target));

	BOOST_CHECK_EQUAL(message_type, val.messagetype);
	//BOOST_CHECK_EQUAL_COLLECTIONS(cbegin(val.pb), cend(val.pb),
	//								cbegin(pb), cend(pb));
	{
		auto recordset = statement.fetch();
		BOOST_CHECK(!recordset.empty());
		auto id = recordset.get<0>(0);
		auto ts = recordset.get<1>(0);
		auto target = recordset.get<2>(0);
		auto message_type = recordset.get<3>(0);
		auto pb = recordset.get<4>(0);
		boost::ignore_unused(id);
		boost::ignore_unused(ts);
		boost::ignore_unused(target);
		boost::ignore_unused(message_type);
		boost::ignore_unused(pb);
	}
}

BOOST_AUTO_TEST_CASE(BindingsQueryTest)
{
	auto statement = odbcx::query<data::Test>(dbc, "select id, ts, target, messagetype, pb from test");
	BOOST_CHECK(!statement.bindings().bulk_fetch());
	auto recordset = statement.fetch();
	BOOST_CHECK(!recordset.empty());
	auto val = recordset[0];
	auto id = recordset.get<0>(0);
	auto ts = recordset.get<1>(0);
	auto target = recordset.get<2>(0);
	auto message_type = recordset.get<3>(0);
	BOOST_CHECK_THROW((recordset.get<4>(0)), std::runtime_error);
	//if (!val.pb.empty())
	//{
	//	BOOST_CHECK_THROW((recordset.get<3>(0)), std::runtime_error);
	//}
	//else
	//{
	//	BOOST_CHECK(recordset.get<3>(0).empty());
	//}


	boost::ignore_unused(id);
	boost::ignore_unused(message_type);

	BOOST_CHECK_EQUAL_COLLECTIONS(reinterpret_cast<char const*>(&ts), reinterpret_cast<char const*>(&ts) + sizeof(ts),
										reinterpret_cast<char const*>(&val.ts), reinterpret_cast<char const*>(&val.ts) + sizeof(val.ts));

	BOOST_CHECK_EQUAL_COLLECTIONS(cbegin(target), cend(target),
										cbegin(val.target), cend(val.target));

	BOOST_CHECK_EQUAL(message_type, val.messagetype);
	//BOOST_CHECK_EQUAL_COLLECTIONS(cbegin(val.pb), cend(val.pb),
	//								cbegin(pb), cend(pb));
	{
		auto recordset = statement.fetch();
		BOOST_CHECK(!recordset.empty());
		auto id = recordset.get<0>(0);
		auto ts = recordset.get<1>(0);
		auto target = recordset.get<2>(0);
		auto message_type = recordset.get<3>(0);
		auto pb = recordset.get<4>(0);
		boost::ignore_unused(id);
		boost::ignore_unused(ts);
		boost::ignore_unused(target);
		boost::ignore_unused(message_type);
		boost::ignore_unused(pb);
	}
}

BOOST_AUTO_TEST_CASE(BindingsFetchOneQueryTest)
{
	auto statement = odbcx::query<data::Test>(dbc, "select id, ts, target, messagetype, pb from test");
	BOOST_CHECK(!statement.bindings().bulk_fetch());
	auto val = statement.fetch_one();
	BOOST_CHECK_EQUAL(!val, false);
}

BOOST_AUTO_TEST_CASE(TestNoPBBindingsTest)
{
	static_assert(!odbcx::details::out::IsSequenceStaticallyBindable<data::TestNoPB>::value, "Oops! shouldn't be staticaly bindable");
	static_assert(odbcx::details::out::IsSequenceDynamicallyBindable<data::TestNoPB>::value, "Oops! should be dynamically bindable");

	auto stmt = odbcx::handle::allocate<SQL_HANDLE_STMT>(dbc);
	//	odbcx::SQLSetStmtAttr(stmt, SQL_ATTR_CURSOR_TYPE, SQLPOINTER(SQL_CURSOR_KEYSET_DRIVEN), SQL_IS_INTEGER);
	odbcx::SQLSetStmtAttr(stmt, SQL_ATTR_CONCURRENCY, SQLPOINTER(SQL_CONCUR_READ_ONLY), SQL_IS_INTEGER);
	//odbcx::SQLSetStmtAttr(stmt, SQL_ATTR_CURSOR_SENSITIVITY, SQLPOINTER(SQL_SENSITIVE), SQL_IS_INTEGER);
	//odbcx::SQLSetStmtAttr(stmt, SQL_ATTR_CURSOR_SCROLLABLE, SQLPOINTER(SQL_SCROLLABLE), SQL_IS_INTEGER);
	auto statement = odbcx::DynamicallyBindableStatement<data::TestNoPB>{std::move(stmt), "select target, target as target1, messagetype, messagetype as messagetype1 from test"};
	BOOST_CHECK(statement.bindings().bulk_fetch());
	auto recordset = statement.fetch();
	BOOST_CHECK(!recordset.empty());
	auto target = recordset.get<0>(0);
	auto target1 = recordset.get<1>(0);
	auto message_type = recordset.get<2>(0);
	auto message_type1 = recordset.get<3>(0);

	boost::ignore_unused(target);
	boost::ignore_unused(target1);
	boost::ignore_unused(message_type);
	boost::ignore_unused(message_type1);
}


BOOST_AUTO_TEST_CASE(PBHybridBindingsTest)
{
	static_assert(!odbcx::details::out::IsSequenceStaticallyBindable<data::TestPBHybrid>::value, "Oops! should be staticaly bindable");
	static_assert(odbcx::details::out::IsSequenceDynamicallyBindable<data::TestPBHybrid>::value, "Oops! should be dynamically bindable");

	auto stmt = odbcx::handle::allocate<SQL_HANDLE_STMT>(dbc);
	//	odbcx::SQLSetStmtAttr(stmt, SQL_ATTR_CURSOR_TYPE, SQLPOINTER(SQL_CURSOR_KEYSET_DRIVEN), SQL_IS_INTEGER);
	odbcx::SQLSetStmtAttr(stmt, SQL_ATTR_CONCURRENCY, SQLPOINTER(SQL_CONCUR_READ_ONLY), SQL_IS_INTEGER);
	//odbcx::SQLSetStmtAttr(stmt, SQL_ATTR_CURSOR_SENSITIVITY, SQLPOINTER(SQL_SENSITIVE), SQL_IS_INTEGER);
	//odbcx::SQLSetStmtAttr(stmt, SQL_ATTR_CURSOR_SCROLLABLE, SQLPOINTER(SQL_SCROLLABLE), SQL_IS_INTEGER);
	auto statement = odbcx::DynamicallyBindableStatement<data::TestPBHybrid>{std::move(stmt), "select target, messagetype, pb as pb_s, pb  from test"};
	BOOST_CHECK(!statement.bindings().bulk_fetch());
	auto recordset = statement.fetch();
	BOOST_CHECK(!recordset.empty());
	auto target = recordset.get<0>(0);
	auto message_type = recordset.get<1>(0);
	auto pb_s = recordset.get<2>(0);
	auto pb = recordset.get<3>(0);
	BOOST_CHECK_EQUAL_COLLECTIONS(cbegin(pb_s), cend(pb_s),
									cbegin(pb), cend(pb));

	boost::ignore_unused(target);
	boost::ignore_unused(message_type);
}

BOOST_AUTO_TEST_CASE(PBHybridBindings1Test)
{
	static_assert(!odbcx::details::out::IsSequenceStaticallyBindable<data::TestPBHybrid>::value, "Oops! should be staticaly bindable");
	static_assert(odbcx::details::out::IsSequenceDynamicallyBindable<data::TestPBHybrid>::value, "Oops! should be dynamically bindable");

	auto stmt = odbcx::handle::allocate<SQL_HANDLE_STMT>(dbc);
	//	odbcx::SQLSetStmtAttr(stmt, SQL_ATTR_CURSOR_TYPE, SQLPOINTER(SQL_CURSOR_KEYSET_DRIVEN), SQL_IS_INTEGER);
	odbcx::SQLSetStmtAttr(stmt, SQL_ATTR_CONCURRENCY, SQLPOINTER(SQL_CONCUR_READ_ONLY), SQL_IS_INTEGER);
	//odbcx::SQLSetStmtAttr(stmt, SQL_ATTR_CURSOR_SENSITIVITY, SQLPOINTER(SQL_SENSITIVE), SQL_IS_INTEGER);
	auto scrollable = ::SQLSetStmtAttr(stmt.get(), SQL_ATTR_CURSOR_SCROLLABLE, SQLPOINTER(SQL_SCROLLABLE), SQL_IS_INTEGER) == SQL_SUCCESS;
	auto statement = odbcx::DynamicallyBindableStatement<data::TestPBHybrid>{ std::move(stmt), "select target, messagetype, pb as pb_s, pb  from test" };
	BOOST_CHECK(!statement.bindings().bulk_fetch());
	auto recordset = statement.fetch();
	auto copy = std::vector<data::TestPBHybrid>{ cbegin(recordset), cend(recordset) };
	if (scrollable)
	{
		auto recordset1 = statement.fetch(SQL_FETCH_FIRST);
		BOOST_CHECK(std::equal(cbegin(copy), cend(copy),
			cbegin(recordset1), cend(recordset1), [](auto const& l, auto const& r)
		{
			return std::string{ l.target } == std::string{ r.target }
				&& std::string{ l.messagetype } == std::string{ r.messagetype }
				&& std::equal(l.pb_s, l.pb_s + sizeof(l.pb_s) / sizeof(l.pb_s[0]), r.pb_s, r.pb_s + sizeof(r.pb_s) / sizeof(r.pb_s[0]))
				&& std::equal(cbegin(l.pb), cend(l.pb), cbegin(r.pb), cend(r.pb));
		}));
	}
}
BOOST_AUTO_TEST_SUITE_END()