#include "test.hpp"
#include "dd.hpp"
#include "odbcx/query.hpp"
#include <boost/format.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/mp11/list.hpp>
#include <iterator>
#include <string>
#include <vector>
#include <cstdint>


struct SelectTestSuiteFixture
{
    void init_test(odbcx::handle::adapter::Dbc dbc, test::OdbcDriverType driver_type)
    {
        test::create_table(dbc, "select_statement_test");

		auto stmt = odbcx::handle::allocate<SQL_HANDLE_STMT>(dbc);

		std::size_t inserted = 0;
		switch (driver_type)
		{
            case test::OdbcDriverType::oracle:
                for (; inserted != 300; ++inserted)
                    odbcx::query(stmt, str(boost::format("insert into select_statement_test (id, type, target, value) VALUES (%1%,'type %1%','target %1%', %2%)") % (inserted + 1) % (inserted % 5)));
                break;
            default:
            {
			    auto insert = std::string{ "insert into select_statement_test (id, type, target, value) VALUES " };
			    for (; inserted != 300; ++inserted)
				    insert += str(boost::format("(%1%,'type %1%','target %1%',%2%),") % (inserted + 1) % (inserted % 5));
			    insert.back() = ' ';
			    odbcx::query(stmt, insert);
                break;
		    }
		}
		item_number = inserted;
    }
    std::size_t item_number = 0;
};


BOOST_FIXTURE_TEST_SUITE(select_statement_test_suite, SelectTestSuiteFixture)

BOOST_DATA_TEST_CASE(SelectStaticBindTest, boost::unit_test::data::make_delayed<test::AnyDBConnection>(), dbc, driver_type, driver_version)
{
    init_test(dbc, driver_type);

	auto cursor = odbcx::select<data::TestStaticNoData>{}.from("select_statement_test").exec(dbc);
	auto range = cursor.fetch();
	BOOST_CHECK(!range.empty());
	auto begin = range.begin();
	auto end = range.end();
	for (decltype(item_number) i = 0; i != item_number ; ++i, ++begin)
	{

        BOOST_CHECK_EQUAL(&begin->id, &begin->id);
        BOOST_CHECK_EQUAL(&begin->type, &begin->type);
        BOOST_CHECK_EQUAL(&begin->target, &begin->target);
        BOOST_CHECK_EQUAL(&begin->ts, &begin->ts);

		BOOST_CHECK_EQUAL(begin->id, i + 1);
		BOOST_CHECK_EQUAL(begin->value, i % 5);
        {
            auto target = str(boost::format("target %1%") % (i + 1));
            auto ii = std::find(begin->target, begin->target + sizeof(begin->target) / sizeof(begin->target[0]), '\0');
            auto len = std::distance(begin->target, ii);
            BOOST_CHECK_EQUAL(len, target.length());
            BOOST_CHECK_EQUAL(begin->target, target);
        }
        {
            auto type = str(boost::format("type %1%") % (i + 1));
            auto ii = std::find(begin->type, begin->type + sizeof(begin->type) / sizeof(begin->type[0]), '\0');
            auto len = std::distance(begin->type, ii);
            BOOST_CHECK_EQUAL(len, type.length());
            BOOST_CHECK_EQUAL(begin->type, type);
        }

	}
	BOOST_CHECK(begin == end);
}

BOOST_DATA_TEST_CASE(SelectStaticEnumBindTest, boost::unit_test::data::make_delayed<test::AnyDBConnection>(), dbc, driver_type, driver_version)
{
    init_test(dbc, driver_type);

    static data::TestStaticEnumNoData::Value const values[5] =
    {
        data::TestStaticEnumNoData::Value::zero,
        data::TestStaticEnumNoData::Value::one,
        data::TestStaticEnumNoData::Value::two,
        data::TestStaticEnumNoData::Value::three,
        data::TestStaticEnumNoData::Value::four,
    };

	auto cursor = odbcx::select<data::TestStaticEnumNoData>{}.from("select_statement_test").exec(dbc);
	auto range = cursor.fetch();
	BOOST_CHECK(!range.empty());
	auto begin = range.begin();
	auto end = range.end();
	for (decltype(item_number) i = 0; i != item_number ; ++i, ++begin)
	{

        BOOST_CHECK_EQUAL(&begin->id, &begin->id);
        BOOST_CHECK_EQUAL(&begin->type, &begin->type);
        BOOST_CHECK_EQUAL(&begin->target, &begin->target);
        BOOST_CHECK_EQUAL(&begin->ts, &begin->ts);

		BOOST_CHECK_EQUAL(begin->id, i + 1);
        BOOST_CHECK(begin->value == values[i % 5]);
        {
            auto target = str(boost::format("target %1%") % (i + 1));
            auto ii = std::find(begin->target, begin->target + sizeof(begin->target) / sizeof(begin->target[0]), '\0');
            auto len = std::distance(begin->target, ii);
            BOOST_CHECK_EQUAL(len, target.length());
            BOOST_CHECK_EQUAL(begin->target, target);
        }
        {
            auto type = str(boost::format("type %1%") % (i + 1));
            auto ii = std::find(begin->type, begin->type + sizeof(begin->type) / sizeof(begin->type[0]), '\0');
            auto len = std::distance(begin->type, ii);
            BOOST_CHECK_EQUAL(len, type.length());
            BOOST_CHECK_EQUAL(begin->type, type);
        }

	}
	BOOST_CHECK(begin == end);
}

BOOST_DATA_TEST_CASE(SelectAsStaticBindTest, boost::unit_test::data::make_delayed<test::AnyDBConnection>(), dbc, driver_type, driver_version)
{
    init_test(dbc, driver_type);

	auto cursor = odbcx::select<data::TestStaticNoData>{}.from("select_statement_test").as("alias").exec(dbc);
	auto range = cursor.fetch();
	BOOST_CHECK(!range.empty());
	BOOST_CHECK_EQUAL(std::distance(range.begin(), range.end()), item_number);
}

BOOST_DATA_TEST_CASE(SelectWhereWithParamStaticBindTest, boost::unit_test::data::make_delayed<test::AnyDBConnection>(), dbc, driver_type, driver_version)
{
    init_test(dbc, driver_type);

	auto target = std::string{ "target 1" };
	auto cursor = odbcx::select<data::TestStaticNoData>{}.from("select_statement_test").where("target=?", target).exec(dbc);
	auto range = cursor.fetch();
	BOOST_CHECK(!range.empty());
	BOOST_CHECK_EQUAL(std::distance(range.begin(), range.end()), 1);
}

BOOST_DATA_TEST_CASE(SelectAsWhereStaticBindTest, boost::unit_test::data::make_delayed<test::AnyDBConnection>(), dbc, driver_type, driver_version)
{
    init_test(dbc, driver_type);

	auto cursor = odbcx::select<data::TestStaticNoData>{}.from("select_statement_test").as("alias").where("alias.target like 'target%'").exec(dbc);
	auto range = cursor.fetch();
	BOOST_CHECK(!range.empty());
	BOOST_CHECK_EQUAL(std::distance(range.begin(), range.end()), item_number);
}

BOOST_DATA_TEST_CASE(SelectAsWhereWithParamStaticBindTest, boost::unit_test::data::make_delayed<test::AnyDBConnection>(), dbc, driver_type, driver_version)
{
    init_test(dbc, driver_type);

    auto target = std::string{ "target 1" };
	auto cursor = odbcx::select<data::TestStaticNoData>{}.from("select_statement_test").as("alias").where("alias.target=?", target).exec(dbc);
    auto range = cursor.fetch();
    BOOST_CHECK(!range.empty());
    BOOST_CHECK_EQUAL(std::distance(range.begin(), range.end()), 1);
}

BOOST_DATA_TEST_CASE(SelectDynamicBindTest, boost::unit_test::data::make_delayed<test::AnyDBConnection>(), dbc, driver_type, driver_version)
{
    init_test(dbc, driver_type);

	auto cursor = odbcx::select<data::TestNoData>{}.from("select_statement_test").exec(dbc);
	auto range = cursor.fetch();
	BOOST_CHECK(!range.empty());
    auto begin = range.begin();
    auto end = range.end();
    for (decltype(item_number) i = 0; i != item_number; ++i, ++begin)
    {
        BOOST_CHECK_EQUAL(&begin->id, &begin->id);
        BOOST_CHECK_EQUAL(&begin->type, &begin->type);
        BOOST_CHECK_EQUAL(&begin->target, &begin->target);
        BOOST_CHECK_EQUAL(&begin->ts, &begin->ts);

        BOOST_CHECK_EQUAL(begin->id, i + 1);
        BOOST_CHECK_EQUAL(begin->value, i % 5);

        auto target = str(boost::format("target %1%") % (i + 1));
        BOOST_CHECK_EQUAL(begin->target.size(), target.size());
        BOOST_CHECK_EQUAL_COLLECTIONS(std::begin(begin->target), std::end(begin->target), std::begin(target), std::end(target));
        auto type = str(boost::format("type %1%") % (i + 1));
        BOOST_CHECK_EQUAL(begin->type, type);

    }
    BOOST_CHECK(begin == end);
}


BOOST_DATA_TEST_CASE(SelectDynamicMixedNodataBindTest, boost::unit_test::data::make_delayed<test::AnyDBConnection>(), dbc, driver_type, driver_version)
{
    init_test(dbc, driver_type);

    auto cursor = odbcx::select<data::TestMixedNoData>{}.from("select_statement_test").exec(dbc);
    auto range = cursor.fetch();
    BOOST_CHECK(!range.empty());
    auto begin = range.begin();
    auto end = range.end();
    for (decltype(item_number) i = 0; i != item_number; ++i, ++begin)
    {
        BOOST_CHECK_EQUAL(&begin->id, &begin->id);
        BOOST_CHECK_EQUAL(&begin->type, &begin->type);
        BOOST_CHECK_EQUAL(&begin->target, &begin->target);
        BOOST_CHECK_EQUAL(&begin->ts, &begin->ts);

        BOOST_CHECK_EQUAL(begin->id, i + 1);
        BOOST_CHECK_EQUAL(begin->value, i % 5);

        auto target = str(boost::format("target %1%") % (i + 1));
        auto len = std::char_traits<char>::length(begin->target);
        BOOST_CHECK_EQUAL(len, target.size());
        BOOST_CHECK_EQUAL_COLLECTIONS(begin->target, begin->target + len, std::begin(target), std::end(target));
        auto type = str(boost::format("type %1%") % (i + 1));
        BOOST_CHECK_EQUAL(begin->type, type);

    }
    BOOST_CHECK(begin == end);
}

BOOST_DATA_TEST_CASE(SelectDynamicAdaptedNodataBindTest, boost::unit_test::data::make_delayed<test::AnyDBConnection>(), dbc, driver_type, driver_version)
{
    init_test(dbc, driver_type);

    auto cursor = odbcx::select<data::TestAdaptedNoData>{}.from("select_statement_test").exec(dbc);
    auto range = cursor.fetch();
    BOOST_CHECK(!range.empty());
    auto begin = range.begin();
    auto end = range.end();
    for (decltype(item_number) i = 0; i != item_number; ++i, ++begin)
    {
        BOOST_CHECK_EQUAL(&begin->id, &begin->id);
        BOOST_CHECK_EQUAL(&begin->type, &begin->type);
        BOOST_CHECK_EQUAL(&begin->target, &begin->target);
        BOOST_CHECK_EQUAL(&begin->ts, &begin->ts);
        BOOST_CHECK_EQUAL(&begin->sum, &begin->sum);

        BOOST_CHECK_EQUAL(!begin->id, false);
        BOOST_CHECK_EQUAL(*begin->id, i + 1);
        BOOST_CHECK_EQUAL(begin->value.is_null, false);
        BOOST_CHECK_EQUAL(begin->sum.is_null, true);
        BOOST_CHECK_EQUAL(begin->value.value, i % 5);

        auto target = str(boost::format("target %1%") % (i + 1));
        BOOST_CHECK_EQUAL(begin->target.value.size(), target.size());
        BOOST_CHECK_EQUAL_COLLECTIONS(std::begin(begin->target.value), std::end(begin->target.value), std::begin(target), std::end(target));
        auto type = str(boost::format("type %1%") % (i + 1));
        BOOST_CHECK_EQUAL(begin->type.value, type);
    }
    BOOST_CHECK(begin == end);
}

#pragma message("TODO:make it work for oracle")
BOOST_DATA_TEST_CASE(SelectDynamicBindMixedNotOptimalDataLayoutTest,
    boost::unit_test::data::make_delayed<test::DBConnections<test::Except<test::OdbcDriverType::oracle>>>(),
    dbc, driver_type, driver_version)
{
    init_test(dbc, driver_type);

    auto cursor = odbcx::select<data::TestMixedNotOptimal>{}.from("select_statement_test").exec(dbc);
    auto range = cursor.fetch();
    BOOST_CHECK(!range.empty());
    auto begin = range.begin();
    auto end = range.end();
    for (decltype(item_number) i = 0; i != item_number; ++i, ++begin)
    {
        BOOST_CHECK_EQUAL(&begin->id, &begin->id);
        BOOST_CHECK_EQUAL(&begin->type, &begin->type);
        BOOST_CHECK_EQUAL(&begin->target, &begin->target);
        BOOST_CHECK_EQUAL(&begin->ts, &begin->ts);
        BOOST_CHECK_EQUAL(&begin->bin_data, &begin->bin_data);

        BOOST_CHECK_EQUAL(begin->id, i + 1);
        BOOST_CHECK_EQUAL(begin->value, i % 5);

        auto target = str(boost::format("target %1%") % (i + 1));
        auto len = std::char_traits<char>::length(begin->target);
        BOOST_CHECK_EQUAL(len, target.size());
        BOOST_CHECK_EQUAL_COLLECTIONS(begin->target, begin->target + len, std::begin(target), std::end(target));
        auto type = str(boost::format("type %1%") % (i + 1));
        BOOST_CHECK_EQUAL(begin->type, type);
        BOOST_CHECK(begin->bin_data.empty());

    }
    BOOST_CHECK(begin == end);
}


#pragma message("TODO:make it work for oracle")
BOOST_DATA_TEST_CASE(SelectDynamicEnumBindTest, 
    boost::unit_test::data::make_delayed<test::DBConnections<test::Except<test::OdbcDriverType::oracle>>>(),
    dbc, driver_type, driver_version)
{
    init_test(dbc, driver_type);

    auto cursor = odbcx::select<data::TestEnum>{}.from("select_statement_test").exec(dbc);
    auto range = cursor.fetch();
    BOOST_CHECK(!range.empty());
    auto begin = range.begin();
    auto end = range.end();
    for (decltype(item_number) i = 0; i != item_number; ++i, ++begin)
    {
        BOOST_CHECK_EQUAL(&begin->id, &begin->id);
        BOOST_CHECK_EQUAL(&begin->type, &begin->type);
        BOOST_CHECK_EQUAL(&begin->target, &begin->target);
        BOOST_CHECK_EQUAL(&begin->ts, &begin->ts);

        BOOST_CHECK_EQUAL(begin->id, i + 1);
        BOOST_CHECK(begin->value ==  static_cast<decltype(begin->value)>(i % 5));

        auto target = str(boost::format("target %1%") % (i + 1));
        BOOST_CHECK_EQUAL(begin->target.size(), target.size());
        BOOST_CHECK_EQUAL_COLLECTIONS(std::begin(begin->target), std::end(begin->target), std::begin(target), std::end(target));
        auto type = str(boost::format("type %1%") % (i + 1));
        BOOST_CHECK_EQUAL(begin->type, type);

    }
    BOOST_CHECK(begin == end);
}


BOOST_DATA_TEST_CASE(SelectAsDynamicBindTest, boost::unit_test::data::make_delayed<test::AnyDBConnection>(), dbc, driver_type, driver_version)
{
    init_test(dbc, driver_type);

	auto cursor = odbcx::select<data::TestNoData>{}.from("select_statement_test").as("alias").exec(dbc);
	auto range = cursor.fetch();
	BOOST_CHECK(!range.empty());
	BOOST_CHECK_EQUAL(std::distance(range.begin(), range.end()), item_number);
}

BOOST_DATA_TEST_CASE(SelectWhereWithParamDynamicBindTest, boost::unit_test::data::make_delayed<test::AnyDBConnection>(), dbc, driver_type, driver_version)
{
    init_test(dbc, driver_type);

    auto target = std::string{ "target 1" };
	auto cursor = odbcx::select<data::TestNoData>{}.from("select_statement_test").where("target=?", target).exec(dbc);
	auto range = cursor.fetch();
	BOOST_CHECK(!range.empty());
    BOOST_CHECK_EQUAL(std::distance(range.begin(), range.end()), 1);
}

BOOST_DATA_TEST_CASE(SelectAsWhereDynamicBindTest, boost::unit_test::data::make_delayed<test::AnyDBConnection>(), dbc, driver_type, driver_version)
{
    init_test(dbc, driver_type);

	auto cursor = odbcx::select<data::TestNoData>{}.from("select_statement_test").as("alias").where("alias.target like 'target%'").exec(dbc);
	auto range = cursor.fetch();
	BOOST_CHECK(!range.empty());
	BOOST_CHECK_EQUAL(std::distance(range.begin(), range.end()), item_number);
}


BOOST_DATA_TEST_CASE(SelectAsWhereWithParamDynamicBindTest, boost::unit_test::data::make_delayed<test::AnyDBConnection>(), dbc, driver_type, driver_version)
{
    init_test(dbc, driver_type);

    auto target = std::string{ "target 1" };
	auto cursor = odbcx::select<data::TestNoData>{}.from("select_statement_test").as("alias").where("alias.target=?", target).exec(dbc);
	auto range = cursor.fetch();
	BOOST_CHECK(!range.empty());
	BOOST_CHECK_EQUAL(std::distance(range.begin(), range.end()), 1);
}

BOOST_DATA_TEST_CASE(SelectOptionalBindTest, boost::unit_test::data::make_delayed<test::AnyDBConnection>(), dbc, driver_type, driver_version)
{
    init_test(dbc, driver_type);

	auto cursor = odbcx::select<data::TestNoDataOptional>{}.from("select_statement_test").as("alias").where("alias.target like 'target%'").exec(dbc);
	auto range = cursor.fetch();
    BOOST_CHECK(!range.empty());
    auto begin = range.begin();
    auto end = range.end();
    for (decltype(item_number) i = 0; i != item_number; ++i, ++begin)
    {
        BOOST_CHECK_EQUAL(&begin->id, &begin->id);
        BOOST_CHECK_EQUAL(&begin->type, &begin->type);
        BOOST_CHECK_EQUAL(&begin->target, &begin->target);
        BOOST_CHECK_EQUAL(&begin->ts, &begin->ts);

        BOOST_CHECK_EQUAL(!begin->id, false);
        BOOST_CHECK_EQUAL(*begin->id, i + 1);
        BOOST_CHECK_EQUAL(!begin->value, false);
        BOOST_CHECK_EQUAL(*begin->value, i % 5);

        BOOST_CHECK_EQUAL(!begin->target, false);
        auto target = str(boost::format("target %1%") % (i + 1));
        BOOST_CHECK_EQUAL((*begin->target).size(), target.size());
        BOOST_CHECK_EQUAL_COLLECTIONS(std::begin(*begin->target), std::end(*begin->target), std::begin(target), std::end(target));
        BOOST_CHECK_EQUAL(!begin->type, false);
        auto type = str(boost::format("type %1%") % (i + 1));
        BOOST_CHECK_EQUAL(*begin->type, type);
        BOOST_CHECK(!begin->ts);
        BOOST_CHECK(!begin->sum);
    }
    BOOST_CHECK(begin == end);
}


BOOST_DATA_TEST_CASE(SelectOptionalEnumBindTest, boost::unit_test::data::make_delayed<test::AnyDBConnection>(), dbc, driver_type, driver_version)
{
    init_test(dbc, driver_type);

    static data::TestEnumOptional::Value const values[5] = 
    { 
        data::TestEnumOptional::Value::zero, 
        data::TestEnumOptional::Value::one,
        data::TestEnumOptional::Value::two,
        data::TestEnumOptional::Value::three,
        data::TestEnumOptional::Value::four,
    };
	auto cursor = odbcx::select<data::TestEnumOptional>{}.from("select_statement_test").as("alias").where("alias.target like 'target%'").exec(dbc);
	auto range = cursor.fetch();
    BOOST_CHECK(!range.empty());
    auto begin = range.begin();
    auto end = range.end();
    for (decltype(item_number) i = 0; i != item_number; ++i, ++begin)
    {
        BOOST_CHECK_EQUAL(&begin->id, &begin->id);
        BOOST_CHECK_EQUAL(&begin->type, &begin->type);
        BOOST_CHECK_EQUAL(&begin->target, &begin->target);
        BOOST_CHECK_EQUAL(&begin->ts, &begin->ts);
        BOOST_CHECK_EQUAL(&begin->bin_data, &begin->bin_data);

        BOOST_CHECK_EQUAL(!begin->id, false);
        BOOST_CHECK_EQUAL(*begin->id, i + 1);
        BOOST_CHECK_EQUAL(!begin->value, false);
        BOOST_CHECK(*begin->value == values[i % 5]);

        BOOST_CHECK_EQUAL(!begin->target, false);
        auto target = str(boost::format("target %1%") % (i + 1));
        BOOST_CHECK_EQUAL((*begin->target).size(), target.size());
        BOOST_CHECK_EQUAL_COLLECTIONS(std::begin(*begin->target), std::end(*begin->target), std::begin(target), std::end(target));
        BOOST_CHECK_EQUAL(!begin->type, false);
        auto type = str(boost::format("type %1%") % (i + 1));
        BOOST_CHECK_EQUAL(*begin->type, type);
        BOOST_CHECK(!begin->ts);
        BOOST_CHECK(!begin->bin_data);
    }
    BOOST_CHECK(begin == end);
}


#pragma message("TODO:make it work for oracle")
BOOST_DATA_TEST_CASE(SelectDynamicBindNotOptimalDataLayoutTest,
    boost::unit_test::data::make_delayed<test::DBConnections<test::Except<test::OdbcDriverType::oracle>>>(),
    dbc, driver_type, driver_version)
{
    init_test(dbc, driver_type);

    auto cursor = odbcx::select<data::TestNotOptimal>{}.from("select_statement_test").exec(dbc);
    auto range = cursor.fetch();
    BOOST_CHECK(!range.empty());
    auto begin = range.begin();
    auto end = range.end();
    for (decltype(item_number) i = 0; i != item_number ; ++i, ++begin)
    {
        BOOST_CHECK_EQUAL(&begin->id, &begin->id);
        BOOST_CHECK_EQUAL(&begin->type, &begin->type);
        BOOST_CHECK_EQUAL(&begin->target, &begin->target);
        BOOST_CHECK_EQUAL(&begin->ts, &begin->ts);
        BOOST_CHECK_EQUAL(&begin->bin_data, &begin->bin_data);

        BOOST_CHECK_EQUAL(begin->id, i + 1);
        BOOST_CHECK_EQUAL(begin->value, i % 5);

        auto target = str(boost::format("target %1%") % (i + 1));
        BOOST_CHECK_EQUAL(begin->target.size(), target.size());
        BOOST_CHECK_EQUAL_COLLECTIONS(std::begin(begin->target), std::end(begin->target), std::begin(target), std::end(target));
        auto type = str(boost::format("type %1%") % (i + 1));
        BOOST_CHECK_EQUAL(begin->type, type);
        BOOST_CHECK(begin->bin_data.empty());

    }
    BOOST_CHECK(begin == end);
}

BOOST_DATA_TEST_CASE(SelectDynamicBindOptionalNotOptimalDataLayoutTest, boost::unit_test::data::make_delayed<test::AnyDBConnection>(), dbc, driver_type, driver_version)
{
    init_test(dbc, driver_type);

    auto cursor = odbcx::select<data::TestOptionalNotOptimal>{}.from("select_statement_test").exec(dbc);
    auto range = cursor.fetch();
    BOOST_CHECK(!range.empty());
    auto begin = range.begin();
    auto end = range.end();
    for (decltype(item_number) i = 0; i != item_number; ++i, ++begin)
    {
        BOOST_CHECK_EQUAL(&begin->id, &begin->id);
        BOOST_CHECK_EQUAL(&begin->type, &begin->type);
        BOOST_CHECK_EQUAL(&begin->target, &begin->target);
        BOOST_CHECK_EQUAL(&begin->ts, &begin->ts);
        BOOST_CHECK_EQUAL(&begin->bin_data, &begin->bin_data);

        BOOST_CHECK_EQUAL(!begin->id, false);
        BOOST_CHECK_EQUAL(*begin->id, i + 1);
        BOOST_CHECK_EQUAL(!begin->value, false);
        BOOST_CHECK_EQUAL(*begin->value, i % 5);

        BOOST_CHECK_EQUAL(!begin->target, false);
        auto target = str(boost::format("target %1%") % (i + 1));
        BOOST_CHECK_EQUAL((*begin->target).size(), target.size());
        BOOST_CHECK_EQUAL_COLLECTIONS(std::begin(*begin->target), std::end(*begin->target), std::begin(target), std::end(target));
        BOOST_CHECK_EQUAL(!begin->type, false);
        auto type = str(boost::format("type %1%") % (i + 1));
        BOOST_CHECK_EQUAL(*begin->type, type);
        BOOST_CHECK(!begin->ts);
        BOOST_CHECK(!begin->sum);
        BOOST_CHECK(!begin->bin_data);
    }
    BOOST_CHECK(begin == end);
}


BOOST_AUTO_TEST_SUITE_END()