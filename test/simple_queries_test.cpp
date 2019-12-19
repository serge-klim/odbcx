#include "test.hpp"
#include "dd.hpp"
#include "odbcx/query.hpp"
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(simple_queries_test_suite)

BOOST_DATA_TEST_CASE(SimpleQueryTest, boost::unit_test::data::make_delayed<test::AnyDBConnection>(), dbc, driver_type, driver_version)
{
    test::create_table(dbc, "simple_queries_test");
    auto stmt = odbcx::handle::allocate<SQL_HANDLE_STMT>(dbc);
    odbcx::query(stmt, "select * from simple_queries_test");
}

BOOST_DATA_TEST_CASE(AttrubutedSimpleQueryTwoStepsTest, boost::unit_test::data::make_delayed<test::AnyDBConnection>(), dbc, driver_type, driver_version)
{
    test::create_table(dbc, "simple_queries_test");
    auto stmt = odbcx::handle::allocate<SQL_HANDLE_STMT>(dbc, odbcx::attribute::concur_lock | odbcx::attribute::cursor_keyset_driven | odbcx::attribute::bookmarks);
    odbcx::query(stmt, "select * from simple_queries_test");
}

BOOST_DATA_TEST_CASE(AttrubutedSimpleQueryTest, boost::unit_test::data::make_delayed<test::AnyDBConnection>(), dbc, driver_type, driver_version)
{
    test::create_table(dbc, "simple_queries_test");
    odbcx::query(dbc, odbcx::attribute::concur_lock | odbcx::attribute::cursor_keyset_driven | odbcx::attribute::bookmarks, "select id, ts, type, target, value from simple_queries_test");
}

BOOST_DATA_TEST_CASE(CursorQueryTwoStepsTest, boost::unit_test::data::make_delayed<test::AnyDBConnection>(), dbc, driver_type, driver_version)
{
    test::create_table(dbc, "simple_queries_test");
    /*static constexpr*/ auto attributes = odbcx::attribute::concur_lock | odbcx::attribute::cursor_dynamic | odbcx::attribute::bookmarks;
    auto stmt = odbcx::handle::allocate<SQL_HANDLE_STMT>(dbc, attributes);
    auto cursor = odbcx::query<data::TestNoDataAdaptable>(std::move(stmt), "select id, ts, type, target, value from simple_queries_test");
    static_assert(std::is_same<odbcx::details::ModifiableCursor<data::TestNoDataAdaptable, decltype(attributes)>, decltype(cursor)>::value, "Oops! deduced  cursor type seems to be broken");
}

BOOST_DATA_TEST_CASE(SimpleQueryOneTest, boost::unit_test::data::make_delayed<test::AnyDBConnection>(), dbc, driver_type, driver_version)
{
    test::create_table(dbc, "simple_queries_test");
    auto val = odbcx::query_one<long>(dbc, "SELECT count(id) FROM simple_queries_test");
    BOOST_CHECK_EQUAL(!val, false);
}

BOOST_DATA_TEST_CASE(SimpleQueryOneStructTest, boost::unit_test::data::make_delayed<test::AnyDBConnection>(), dbc, driver_type, driver_version)
{
    test::create_table(dbc, "simple_queries_test");
    auto val = odbcx::query_one<std::tuple<long, long>>(dbc, "SELECT count(id), count(id) + 100 FROM simple_queries_test");
    BOOST_CHECK_EQUAL(!val, false);
    BOOST_CHECK_EQUAL(std::get<0>(val.value()) + 100, std::get<1>(val.value()));
}

BOOST_DATA_TEST_CASE(SimpleQueryOneStructWithParamTest, boost::unit_test::data::make_delayed<test::AnyDBConnection>(), dbc, driver_type, driver_version)
{
    test::create_table(dbc, "simple_queries_test");
    int xtra = 100;
    auto val = odbcx::query_one<std::tuple<long, long>>(dbc, "SELECT count(id), count(id) + ? FROM simple_queries_test", xtra);
    BOOST_CHECK_EQUAL(!val, false);
    BOOST_CHECK_EQUAL(std::get<0>(val.value()) + xtra, std::get<1>(val.value()));
}

BOOST_DATA_TEST_CASE(InputParametersSimpleTest, boost::unit_test::data::make_delayed<test::AnyDBConnection>(), dbc, driver_type, driver_version)
{
    test::create_table(dbc, "simple_queries_test");
    odbcx::query(dbc, "insert into simple_queries_test (target, type) values(?,?)", "hi", "there");
}

BOOST_DATA_TEST_CASE(InputParametersSimple1Test, boost::unit_test::data::make_delayed<test::AnyDBConnection>(), dbc, driver_type, driver_version)
{
    test::create_table(dbc, "simple_queries_test");
    auto target = std::string{ "test" };
    int value = 500;
    odbcx::query(dbc, "insert into simple_queries_test (target, type, value) values(?,?,?)", target, "...", value);
}

BOOST_DATA_TEST_CASE(InputParametersLongBinTest, boost::unit_test::data::make_delayed<test::AnyDBConnection>(), dbc, driver_type, driver_version)
{
    test::create_table(dbc, "simple_queries_test");
    odbcx::query(dbc, "insert into simple_queries_test (target, type, bin_data) values(?,?,?)", std::string{ "test" }, "long  binary", std::vector<std::uint8_t>(100 * 1024, 0xF));
}

BOOST_DATA_TEST_CASE(InputParametersBinTest, boost::unit_test::data::make_delayed<test::AnyDBConnection>(), dbc, driver_type, driver_version)
{
    test::create_table(dbc, "simple_queries_test");
    odbcx::query(dbc, "insert into simple_queries_test (target, type, bin_data) values(?,?,?)", "test", "binary", std::vector<std::uint8_t>(10, 0xA));
}

BOOST_DATA_TEST_CASE(InputParametersEmptyTest, boost::unit_test::data::make_delayed<test::AnyDBConnection>(), dbc, driver_type, driver_version)
{
    test::create_table(dbc, "simple_queries_test");
    auto empty = std::string{};
    switch (driver_type)
    {
        case test::OdbcDriverType::psql:
            #pragma message("TODO:revisit it!!!!")
            return;
        case test::OdbcDriverType::oracle: //oracle treats empty strings as NULL https://docs.oracle.com/database/121/SQLRF/sql_elements005.htm#SQLRF30037
            odbcx::query(dbc, "insert into simple_queries_test (target, type, bin_data) values(?,?,?)", empty, "not empty", std::vector<std::uint8_t>{});
            break;
        default:
            odbcx::query(dbc, "insert into simple_queries_test (target, type, bin_data) values(?,?,?)", empty, empty, std::vector<std::uint8_t>{});
    }
}

BOOST_DATA_TEST_CASE(InputParametersEmptyOptionalTest, boost::unit_test::data::make_delayed<test::AnyDBConnection>(), dbc, driver_type, driver_version)
{
    test::create_table(dbc, "simple_queries_test");
    auto ts = diversion::optional<SQL_TIMESTAMP_STRUCT>{};
    auto str = diversion::optional<std::string>{};
    auto value = diversion::optional<long>{};
    auto bin_data = diversion::optional< std::vector<std::uint8_t>>{};

    switch (driver_type)
    {
        case test::OdbcDriverType::oracle: //oracle treats empty strings as NULL https://docs.oracle.com/database/121/SQLRF/sql_elements005.htm#SQLRF30037
            odbcx::query(dbc, "insert into simple_queries_test (ts, target, type, value, bin_data) values(?,?,?,?,?)", ts, str, "not empty", value, bin_data);
            break;
        default:
            odbcx::query(dbc, "insert into simple_queries_test (ts, target, type, value, bin_data) values(?,?,?,?,?)", ts, str, "", value, bin_data);
    }
}

BOOST_DATA_TEST_CASE(InputParametersOptionalTest, boost::unit_test::data::make_delayed<test::AnyDBConnection>(), dbc, driver_type, driver_version)
{
    test::create_table(dbc, "simple_queries_test");
    SQL_TIMESTAMP_STRUCT ts;
    ts.year = 2018;
    ts.month = 12;
    ts.day = 15;
    ts.hour = 13;
    ts.minute = 21;
    ts.second = 0;
    ts.fraction = 1000;

    auto ots = diversion::make_optional(ts);
    auto target = diversion::make_optional(std::string{ "optional-target" });
    auto type = diversion::make_optional(std::string{ "optional-type" });
    auto value = diversion::make_optional(0xabcd);
    auto bin_data = diversion::make_optional(std::vector<std::uint8_t>(11 * 1024, 0xF));
    odbcx::query(dbc, "insert into simple_queries_test (ts, target, type, value, bin_data) values(?,?,?,?,?)", ots, target, type, value, bin_data);
}

BOOST_DATA_TEST_CASE(InputParametersTimestampTest, boost::unit_test::data::make_delayed<test::AnyDBConnection>(), dbc, driver_type, driver_version)
{
    test::create_table(dbc, "simple_queries_test");
    SQL_TIMESTAMP_STRUCT ts;
    ts.year = 2018;
    ts.month = 12;
    ts.day = 7;
    ts.hour = 05;
    ts.minute = 21;
    ts.second = 0;
    ts.fraction = 1000;
    odbcx::query(dbc, "insert into simple_queries_test (ts, type, target) values(?,?,?)", ts, "inserted-timestamp", "timestamp");
}

BOOST_DATA_TEST_CASE(InputParametersXTest, boost::unit_test::data::make_delayed<test::AnyDBConnection>(), dbc, driver_type, driver_version)
{
    test::create_table(dbc, "simple_queries_test");
    SQL_TIMESTAMP_STRUCT ts;
    ts.year = 2018;
    ts.month = 12;
    ts.day = 7;
    ts.hour = 05;
    ts.minute = 21;
    ts.second = 0;
    ts.fraction = 0;
    odbcx::query(dbc, "insert into simple_queries_test (ts, target, type, value, bin_data) values(?,?,?,?,?)", ts, "target", std::string{ "message" }, ts.day, std::vector<std::uint8_t>(100 * 1024, 0xF));
}

BOOST_AUTO_TEST_SUITE_END()


