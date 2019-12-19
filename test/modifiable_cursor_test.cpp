#include "test.hpp"
#include "dd.hpp"
#include "odbcx/query.hpp"
#include "odbcx/cursor.hpp"
#include <boost/format.hpp>
#include <boost/fusion/adapted/std_tuple.hpp>
#include <boost/core/ignore_unused.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/mp11/list.hpp>
#include <chrono>
#include <algorithm>
#include <vector>
#include <tuple>
#include <string>

struct ModifiableCursorTestSuiteFixture 
{
    void init_test(odbcx::handle::adapter::Dbc dbc, test::OdbcDriverType driver_type)
    {
        test::create_table(dbc, "modifiable_cursor_test");
        auto stmt = odbcx::handle::allocate<SQL_HANDLE_STMT>(dbc);

        std::size_t inserted = 0;
        switch (driver_type)
        {
            case test::OdbcDriverType::oracle:
                for (; inserted != 300; ++inserted)
                    odbcx::query(stmt, str(boost::format("insert into modifiable_cursor_test (id, type, target, value) VALUES (%1%,'type %1%','target %1%',%1%%1%%1%)") % (inserted + 1)));
                break;
            default:
            {
                auto insert = std::string{ "insert into modifiable_cursor_test (id, type, target, value) VALUES " };
                for (; inserted != 300; ++inserted)
                    insert += str(boost::format("(%1%,'type %1%','target %1%',%1%%1%%1%),") % (inserted + 1));
                insert.back() = ' ';
                odbcx::query(stmt, insert);
            }
            }
        item_number = inserted;
    }
    std::size_t item_number = 0;
};

BOOST_FIXTURE_TEST_SUITE(modifiable_cursor_test_suite, ModifiableCursorTestSuiteFixture)

BOOST_DATA_TEST_CASE(StaticallyBoundModifiableCursorInsertTest, boost::unit_test::data::make_delayed<test::AnyDBConnection>(), dbc, driver_type, driver_version)
{
    init_test(dbc, driver_type);
    /*static constexpr*/ auto attributes = odbcx::attribute::concur_lock | odbcx::attribute::cursor_keyset_driven | odbcx::attribute::bookmarks;
    ///*static constexpr*/ auto attributes = odbcx::attribute::concur_rowver | odbcx::attribute::cursor_static | odbcx::attribute::bookmarks;
    auto cursor = odbcx::select<data::TestStaticNoData>{}.from("modifiable_cursor_test").exec(dbc, attributes);

    auto now = std::chrono::system_clock::now();
    time_t tt = std::chrono::system_clock::to_time_t(now);
    auto ts = gmtime(&tt);
    
    auto timestamp = SQL_TIMESTAMP_STRUCT{ 0 };
    timestamp.year = ts->tm_year + 1900;
    timestamp.month = ts->tm_mon + 1;
    timestamp.day = ts->tm_mday;
    timestamp.hour = ts->tm_hour;
    timestamp.minute = ts->tm_min;
    timestamp.second = ts->tm_sec;
    
    cursor.insert(data::TestStaticNoData{ 900, timestamp, "type - inserted 1", "ok1-static", 999999 });
    cursor.insert(data::TestStaticNoData{ 9001,timestamp, "type - inserted 2", "ok2-static", 111999 });
    //BOOST_CHECK_EQUAL(item_number, std::distance(range.begin(), range.end()));
    cursor.apply_changes();    
    cursor.insert(data::TestStaticNoData{ 1003, timestamp, "type - inserted 3", "ok3-static", 1013 });
    cursor.insert(data::TestStaticNoData{ 1004, timestamp, "type - inserted 4", "ok4-static", 1014 });
    cursor.apply_changes();

    auto n = odbcx::query_one<long>(dbc /*std::move(cursor)*/, "select count(id) from modifiable_cursor_test");
    BOOST_CHECK_EQUAL(!n, false);
    BOOST_CHECK_EQUAL(*n, item_number + 4);
}

BOOST_DATA_TEST_CASE(StaticallyBoundModifiableCursorDeleteTest,
    (boost::unit_test::data::make_delayed<test::DBConnections<
                        test::Except<test::OdbcDriverType::mysql>,
                        test::Except<test::OdbcDriverType::psql, 10>
                      >>()),
    dbc, driver_type, driver_version)
{
    init_test(dbc, driver_type);

    /*static constexpr*/ auto attributes = odbcx::attribute::concur_lock | odbcx::attribute::cursor_keyset_driven | odbcx::attribute::bookmarks;
    ///*static constexpr*/ auto attributes = odbcx::attribute::concur_rowver | odbcx::attribute::cursor_static | odbcx::attribute::bookmarks;
    auto cursor = odbcx::select<data::TestStaticNoData>{}.from("modifiable_cursor_test").exec(dbc, attributes);

    auto range = cursor.fetch();
    std::size_t items2delete = 100;
    BOOST_CHECK(item_number > items2delete);
    auto begin = range.begin();
    for (int i = 0; i != items2delete; ++i)
        cursor.remove(begin++);
//    while(++begin != range.end()){}
    cursor.apply_changes();

    auto n = odbcx::query_one<long>(dbc /*std::move(cursor)*/, "select count(id) from modifiable_cursor_test");
    BOOST_CHECK_EQUAL(!n, false);
    BOOST_CHECK_EQUAL(*n, item_number - items2delete);
}

BOOST_DATA_TEST_CASE(DynamicallyBoundModifiableCursorInsertTest, boost::unit_test::data::make_delayed<test::AnyDBConnection>(), dbc, driver_type, driver_version)
{
    init_test(dbc, driver_type);

    /*static constexpr*/ auto attributes = odbcx::attribute::concur_lock | odbcx::attribute::cursor_keyset_driven | odbcx::attribute::bookmarks;
    ///*static constexpr*/ auto attributes = odbcx::attribute::concur_rowver | odbcx::attribute::cursor_static | odbcx::attribute::bookmarks;
    auto cursor = odbcx::select<data::TestNoData>{}.from("modifiable_cursor_test").exec(dbc, attributes);

    auto now = std::chrono::system_clock::now();
    time_t tt = std::chrono::system_clock::to_time_t(now);
    auto ts = gmtime(&tt);

    auto timestamp = SQL_TIMESTAMP_STRUCT{ 0 };
    timestamp.year = ts->tm_year + 1900;
    timestamp.month = ts->tm_mon + 1;
    timestamp.day = ts->tm_mday;
    timestamp.hour = ts->tm_hour;
    timestamp.minute = ts->tm_min;
    timestamp.second = ts->tm_sec;

    cursor.insert(data::TestNoData{ 1001, timestamp, "type - inserted 1", std::vector<char>{'o', 'k', '1'}, 1011 });
    cursor.insert(data::TestNoData{ 1002, timestamp, "type - inserted 2", std::vector<char>{'o', 'k', '2'}, 1012 });
    //cursor.insert(9001, SQL_TIMESTAMP_STRUCT{}, std::string{ "type - inserted 3" }, std::vector<char>{'o', 'k', '3'}, 111999);
    //BOOST_CHECK_EQUAL(item_number, std::distance(range.begin(), range.end()));
    cursor.apply_changes();
    cursor.insert(1003, timestamp, "type - inserted 3", std::vector<char>{'o', 'k', '3'}, 1013);
    cursor.insert(1004, timestamp, "type - inserted 4", std::vector<char>{'o', 'k', '4'}, 1014);
    cursor.apply_changes();

    auto n = odbcx::query_one<long>(dbc /*std::move(cursor)*/, "select count(id) from modifiable_cursor_test");
    BOOST_CHECK_EQUAL(!n, false);
    BOOST_CHECK_EQUAL(*n, item_number + 4);
}

BOOST_DATA_TEST_CASE(DynamicallyBoundModifiableCursorTestNoDataAdaptableInsertTest, boost::unit_test::data::make_delayed<test::AnyDBConnection>(), dbc, driver_type, driver_version)
{
    init_test(dbc, driver_type);

    /*static constexpr*/ auto attributes = odbcx::attribute::concur_lock | odbcx::attribute::cursor_keyset_driven | odbcx::attribute::bookmarks;
    ///*static constexpr*/ auto attributes = odbcx::attribute::concur_rowver | odbcx::attribute::cursor_static | odbcx::attribute::bookmarks;
    auto cursor = odbcx::select<data::TestNoDataAdaptable>{}.from("modifiable_cursor_test").exec(dbc, attributes);

    auto now = std::chrono::system_clock::now();
    cursor.insert(data::TestNoDataAdaptable{ 1001, now, "type - inserted 1", std::vector<char>{'o', 'k', '1'}, 1011 });
    cursor.insert(data::TestNoDataAdaptable{ 1002, now, "type - inserted 2", std::vector<char>{'o', 'k', '2'}, 1012 });
    //cursor.insert(9001, SQL_TIMESTAMP_STRUCT{}, std::string{ "type - inserted 3" }, std::vector<char>{'o', 'k', '3'}, 111999);
    //BOOST_CHECK_EQUAL(item_number, std::distance(range.begin(), range.end()));
    cursor.apply_changes();
    cursor.insert(1003, now, "type - inserted 3", std::vector<char>{'o', 'k', '3'}, 1013);
    cursor.insert(1004, now, "type - inserted 4", std::vector<char>{'o', 'k', '4'}, 'a');
    cursor.apply_changes();

    auto n = odbcx::query_one<long>(dbc /*std::move(cursor)*/, "select count(id) from modifiable_cursor_test");
    BOOST_CHECK_EQUAL(!n, false);
    BOOST_CHECK_EQUAL(*n, item_number + 4);
}

BOOST_DATA_TEST_CASE(DynamicallyBoundModifiableCursorTestNoDataOptionalInsertTest, boost::unit_test::data::make_delayed<test::AnyDBConnection>(), dbc, driver_type, driver_version)
{
    init_test(dbc, driver_type);

    /*static constexpr*/ auto attributes = odbcx::attribute::concur_lock | odbcx::attribute::cursor_keyset_driven | odbcx::attribute::bookmarks;
    ///*static constexpr*/ auto attributes = odbcx::attribute::concur_rowver | odbcx::attribute::cursor_static | odbcx::attribute::bookmarks;
    auto cursor = odbcx::select<data::TestNoDataOptional>{}.from("modifiable_cursor_test").exec(dbc, attributes);

    auto now = std::chrono::system_clock::now();
    time_t tt = std::chrono::system_clock::to_time_t(now);
    auto ts = gmtime(&tt);

    auto timestamp = SQL_TIMESTAMP_STRUCT{ 0 };
    timestamp.year = ts->tm_year + 1900;
    timestamp.month = ts->tm_mon + 1;
    timestamp.day = ts->tm_mday;
    timestamp.hour = ts->tm_hour;
    timestamp.minute = ts->tm_min;
    timestamp.second = ts->tm_sec;

    cursor.insert(data::TestNoDataOptional{ 1001, timestamp, std::string{"type - inserted 1"}, std::vector<char>{'o', 'k', '1'}, 1011 });
    cursor.insert(data::TestNoDataOptional{ 1002, timestamp, std::string{"type - inserted 2"}, std::vector<char>{'o', 'k', '2'}, 1012 });
    cursor.apply_changes();
    cursor.insert(1003, timestamp, std::string{ "type - inserted 3" }, std::vector<char>{'o', 'k', '3'}, diversion::nullopt, 1013);
    cursor.insert(1004, timestamp, std::string{ "type - inserted 4" }, std::vector<char>{'o', 'k', '4'}, diversion::nullopt, 'a');
    cursor.insert(1005, timestamp, std::string{ "type - inserted 5" }, std::vector<char>{'o', 'k', '5'}, 555., diversion::nullopt);
    cursor.insert(1006, timestamp, std::string{ "type - inserted 6" }, diversion::nullopt, diversion::nullopt, diversion::nullopt);
    cursor.insert(1007, diversion::make_optional(timestamp), std::string{ "type - inserted 7" }, diversion::make_optional(std::vector<char>{'o', 'k', '6'}), 345, diversion::nullopt);
    cursor.apply_changes();

    auto n = odbcx::query_one<long>(dbc /*std::move(cursor)*/, "select count(id) from modifiable_cursor_test");
    BOOST_CHECK_EQUAL(!n, false);
    BOOST_CHECK_EQUAL(*n, item_number + 7);
}

BOOST_DATA_TEST_CASE(DynamicallyBoundModifiableCursorTestAdaptedNoDataInsertTest, boost::unit_test::data::make_delayed<test::AnyDBConnection>(), dbc, driver_type, driver_version)
{
    init_test(dbc, driver_type);

    /*static constexpr*/ auto attributes = odbcx::attribute::concur_lock | odbcx::attribute::cursor_keyset_driven | odbcx::attribute::bookmarks;
    ///*static constexpr*/ auto attributes = odbcx::attribute::concur_rowver | odbcx::attribute::cursor_static | odbcx::attribute::bookmarks;
    auto cursor = odbcx::select<data::TestAdaptedNoData>{}.from("modifiable_cursor_test").exec(dbc, attributes);

    auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());


    cursor.insert(data::TestAdaptedNoData{ 1001, data::BizarreTimepoint{now}, data::BizarreString{"type - inserted 1"}, std::vector<char>{'o', 'k', '1'}, 1011 });
    cursor.insert(data::TestAdaptedNoData{ 1002, data::BizarreTimepoint{now}, data::BizarreString{"type - inserted 2"}, std::vector<char>{'o', 'k', '2'}, 1012 });
    cursor.apply_changes();
    cursor.insert(1003, data::BizarreTimepoint{ now }, data::BizarreString{ "type - inserted 3" }, data::BizarreVector<char>{std::vector<char>{'o', 'k', '3'}}, data::BizarreDouble{}, 1013);
    cursor.insert(1004, data::BizarreTimepoint{ now }, data::BizarreString{ "type - inserted 4" }, data::BizarreVector<char>{std::vector<char>{'o', 'k', '4'}}, data::BizarreDouble{}, 'a');
    cursor.insert(diversion::make_optional(1005), data::BizarreTimepoint{ now }, data::BizarreString{ "type - inserted 5" }, data::BizarreVector<char>{std::vector<char>{'o', 'k', '5'}}, 555., data::BizarreInt{});
    cursor.apply_changes();

    auto n = odbcx::query_one<long>(dbc /*std::move(cursor)*/, "select count(id) from modifiable_cursor_test");
    BOOST_CHECK_EQUAL(!n, false);
    BOOST_CHECK_EQUAL(*n, item_number + 5);
}

BOOST_DATA_TEST_CASE(DynamicallyBoundModifiableCursorWithCArrayTestNoDataAdaptableInsertTest, boost::unit_test::data::make_delayed<test::AnyDBConnection>(), dbc, driver_type, driver_version)
{
    init_test(dbc, driver_type);

    /*static constexpr*/ auto attributes = odbcx::attribute::concur_lock | odbcx::attribute::cursor_keyset_driven | odbcx::attribute::bookmarks;
    ///*static constexpr*/ auto attributes = odbcx::attribute::concur_rowver | odbcx::attribute::cursor_static | odbcx::attribute::bookmarks;
    auto cursor = odbcx::select<data::TestMixedNoData>{}.from("modifiable_cursor_test").exec(dbc, attributes);

    auto now = std::chrono::system_clock::now();
    cursor.insert(data::TestMixedNoData{ 1001, now, "type - inserted 1", {'o', 'k', '1'}, .1f, 1011 });
    cursor.insert(data::TestMixedNoData{ 1002, now, "type - inserted 2", {'o', 'k', '2'}, .1f, 1012 });
    cursor.apply_changes();
#pragma message("make it work")
    //cursor.insert(1003, now, "type - inserted 3", "ok3", .1, 1013);
    //cursor.insert(1004, now, "type - inserted 4", "ok4", .1, 'a');
    //cursor.apply_changes();

    auto n = odbcx::query_one<long>(dbc /*std::move(cursor)*/, "select count(id) from modifiable_cursor_test");
    BOOST_CHECK_EQUAL(!n, false);
    BOOST_CHECK_EQUAL(*n, item_number + 2);
}

BOOST_DATA_TEST_CASE(DynamicallyBoundModifiableCursorDeleteTest,
    (boost::unit_test::data::make_delayed<test::DBConnections<
                        test::Except<test::OdbcDriverType::mysql>,
                        test::Except<test::OdbcDriverType::psql, 10>
                      >>()),
    dbc, driver_type, driver_version)
{
    init_test(dbc, driver_type);

    /*static constexpr*/ auto attributes = odbcx::attribute::concur_lock | odbcx::attribute::cursor_keyset_driven | odbcx::attribute::bookmarks;
    ///*static constexpr*/ auto attributes = odbcx::attribute::concur_rowver | odbcx::attribute::cursor_static | odbcx::attribute::bookmarks;
    auto cursor = odbcx::select<data::TestNoData>{}.from("modifiable_cursor_test").exec(dbc, attributes);

    auto range = cursor.fetch();
    std::size_t items2delete = 100;
    BOOST_CHECK(item_number > items2delete);
    auto begin = range.begin();
    for (int i = 0; i != items2delete; ++i)
        cursor.remove(begin++);
    //    while(++begin != range.end()){}
    cursor.apply_changes();

    auto n = odbcx::query_one<long>(dbc /*std::move(cursor)*/, "select count(id) from modifiable_cursor_test");
    BOOST_CHECK_EQUAL(!n, false);
    BOOST_CHECK_EQUAL(*n, item_number - items2delete);
}


BOOST_DATA_TEST_CASE(DynamicallyBoundModifiableCursorWithBlobDeleteTest,
    (boost::unit_test::data::make_delayed<test::DBConnections<
                        test::Except<test::OdbcDriverType::mysql>,
                        test::Except<test::OdbcDriverType::psql, 10>
                      >>()),
    dbc, driver_type, driver_version)
{
    init_test(dbc, driver_type);

    /*static constexpr*/ auto attributes = odbcx::attribute::concur_lock | odbcx::attribute::cursor_keyset_driven | odbcx::attribute::bookmarks;
    ///*static constexpr*/ auto attributes = odbcx::attribute::concur_rowver | odbcx::attribute::cursor_static | odbcx::attribute::bookmarks;
    auto cursor = odbcx::select<data::Test>{}.from("modifiable_cursor_test").exec(dbc, attributes);

    auto range = cursor.fetch();
    std::size_t items2delete = 100;
    BOOST_CHECK(item_number > items2delete);
    auto begin = range.begin();
    for (int i = 0; i != items2delete; ++i)
        cursor.remove(begin++);
    //    while(++begin != range.end()){}
    cursor.apply_changes();

    auto n = odbcx::query_one<long>(dbc /*std::move(cursor)*/, "select count(id) from modifiable_cursor_test");
    BOOST_CHECK_EQUAL(!n, false);
    BOOST_CHECK_EQUAL(*n, item_number - items2delete);
}

BOOST_DATA_TEST_CASE(DynamicallyBoundModifiableCursorNotOptimalDeleteTest,
    (boost::unit_test::data::make_delayed<test::DBConnections<
                        test::Except<test::OdbcDriverType::mysql>,
                        test::Except<test::OdbcDriverType::mssql>, // MSSQL seems not support bulk delete on cursors containing unbound fields
                        test::Except<test::OdbcDriverType::psql, 10>
                      >>()),
    dbc, driver_type, driver_version)
{
    init_test(dbc, driver_type);

    /*static constexpr*/ auto attributes = odbcx::attribute::concur_lock | odbcx::attribute::cursor_keyset_driven | odbcx::attribute::bookmarks;
    ///*static constexpr*/ auto attributes = odbcx::attribute::concur_rowver | odbcx::attribute::cursor_static | odbcx::attribute::bookmarks;
    auto cursor = odbcx::select<data::TestNotOptimal>{}.from("modifiable_cursor_test").exec(dbc, attributes);

    auto range = cursor.fetch();
    std::size_t items2delete = 100;
    BOOST_CHECK(item_number > items2delete);
    auto begin = range.begin();
    for (int i = 0; i != items2delete; ++i)
        cursor.remove(begin++);
    //    while(++begin != range.end()){}
    cursor.apply_changes();

    auto n = odbcx::query_one<long>(dbc /*std::move(cursor)*/, "select count(id) from modifiable_cursor_test");
    BOOST_CHECK_EQUAL(!n, false);
    BOOST_CHECK_EQUAL(*n, item_number - items2delete);
}


//BOOST_AUTO_TEST_CASE(StaticallyBoundModifiableCursorTest)
//{
//    /*static constexpr*/ auto attributes = odbcx::attribute::concur_lock | odbcx::attribute::cursor_keyset_driven | odbcx::attribute::bookmarks;
//    ///*static constexpr*/ auto attributes = odbcx::attribute::concur_rowver | odbcx::attribute::cursor_static | odbcx::attribute::bookmarks;
//    auto cursor = odbcx::query<data::TestStaticNoData>(dbc, attributes, "select id, ts, type, target, sum, value from modifiable_cursor_test");
//    static_assert(std::is_same<odbcx::details::ModifiableCursor<data::TestStaticNoData, decltype(attributes)>, decltype(cursor)>::value, "Oops! deduced  cursor type seems to be broken");
//
//    //auto buffer = cursor.fetch_buffer();
//    //BOOST_CHECK(!buffer);
//    auto range = cursor.fetch();
//    BOOST_CHECK_EQUAL(item_number, std::distance(range.begin(), range.end()));
//    BOOST_CHECK_EQUAL44(item_number, std::distance(range.begin(), range.end()));
//    BOOST_CHECK_EQUAL(item_number, std::distance(range.begin(), range.end()));
//    auto range2 = cursor.fetch();
//    BOOST_CHECK_EQUAL(item_number, std::distance(range2.begin(), range2.end()));
//    auto range3 = cursor.fetch();
//    BOOST_CHECK_EQUAL(item_number, std::distance(range3.begin(), range3.end()));
//
//    int items2delete = 100;
//    BOOST_CHECK(item_number > items2delete);
//    auto begin = range.begin();
//    for (int i = 0; i != items2delete; ++i)
//        cursor.remove(begin++);
//
//    auto now = std::chrono::system_clock::now();
//    time_t tt = std::chrono::system_clock::to_time_t(now);
//    auto ts = gmtime(&tt);
//
//    auto timestamp = SQL_TIMESTAMP_STRUCT{ 0 };
//    timestamp.year = ts->tm_year + 1900;
//    timestamp.month = ts->tm_mon + 1;
//    timestamp.day = ts->tm_mday;
//    timestamp.hour = ts->tm_hour;
//    timestamp.minute = ts->tm_min;
//    timestamp.second = ts->tm_sec;
//
//    cursor.insert(data::TestStaticNoData{ 900, timestamp, "type - inserted 1", "ok1-static", 999999 });
//    cursor.insert(data::TestStaticNoData{ 9001,timestamp, "type - inserted 2", "ok2-static", 111999 });
//    //BOOST_CHECK_EQUAL(item_number, std::distance(range.begin(), range.end()));
//    cursor.apply_changes();    
//    cursor.insert(data::TestStaticNoData{ 1003, timestamp, "type - inserted 3", "ok3-static", 1013 });
//    cursor.insert(data::TestStaticNoData{ 1004, timestamp, "type - inserted 4", "ok4-static", 1014 });
//    cursor.apply_changes();
//
//    {
//        auto handle = std::move(cursor).handle();
//        auto n = odbcx::query_one<long>(odbcx::handle::adapt<SQL_HANDLE_STMT>(handle.get()), "select count(id) from modifiable_cursor_test");
//        BOOST_CHECK_EQUAL(!n, false);
//        BOOST_CHECK_EQUAL(*n, item_number - items2delete + 4);
//    }
//}
//
//BOOST_AUTO_TEST_CASE(DynamicallyBoundModifiableCursorTest)
//{
//    /*static constexpr*/ auto attributes = odbcx::attribute::concur_lock | odbcx::attribute::cursor_keyset_driven | odbcx::attribute::bookmarks;
//    auto cursor = odbcx::query<data::TestNoData>(dbc, attributes, "select id, ts, type, target, value from modifiable_cursor_test");
//    static_assert(std::is_same<odbcx::details::ModifiableCursor<data::TestNoData, decltype(attributes)>, decltype(cursor)>::value, "Oops! deduced  cursor type seems to be broken");
//
//    //auto buffer = cursor.fetch_buffer();
//    //BOOST_CHECK(!buffer);
//    auto range = cursor.fetch();
//    BOOST_CHECK_EQUAL(item_number, std::distance(range.begin(), range.end()));
//    BOOST_CHECK_EQUAL(item_number, std::distance(range.begin(), range.end()));
//    BOOST_CHECK_EQUAL(item_number, std::distance(range.begin(), range.end()));
//    auto range2 = cursor.fetch();
//    BOOST_CHECK_EQUAL(item_number, std::distance(range2.begin(), range2.end()));
//    auto range3 = cursor.fetch();
//    BOOST_CHECK_EQUAL(item_number, std::distance(range3.begin(), range3.end()));
//
//    int items2delete = 100;
//    BOOST_CHECK(item_number > items2delete);
//    auto begin = range.begin();
//    for (int i = 0; i != items2delete; ++i)
//        cursor.remove(begin++);
//
//    auto now = std::chrono::system_clock::now();
//    time_t tt = std::chrono::system_clock::to_time_t(now);
//    auto ts = gmtime(&tt);
//
//    auto timestamp = SQL_TIMESTAMP_STRUCT{ 0 };
//    timestamp.year = ts->tm_year + 1900;
//    timestamp.month = ts->tm_mon + 1;
//    timestamp.day = ts->tm_mday;
//    timestamp.hour = ts->tm_hour;
//    timestamp.minute = ts->tm_min;
//    timestamp.second = ts->tm_sec;
//
//    cursor.insert(data::TestNoData{ 1001, timestamp, "type - inserted 1", std::vector<char>{'o', 'k', '1'}, 1011 });
//    cursor.insert(data::TestNoData{ 1002, timestamp, "type - inserted 2", std::vector<char>{'o', 'k', '2'}, 1012 });
//    //cursor.insert(9001, SQL_TIMESTAMP_STRUCT{}, std::string{ "type - inserted 3" }, std::vector<char>{'o', 'k', '3'}, 111999);
//    //BOOST_CHECK_EQUAL(item_number, std::distance(range.begin(), range.end()));
//    cursor.apply_changes();
//    cursor.insert(1003, timestamp, "type - inserted 3", std::vector<char>{'o', 'k', '3'}, 1013);
//    cursor.insert(1004, timestamp, "type - inserted 4", std::vector<char>{'o', 'k', '4'}, 1014);
//    cursor.apply_changes();
//
//    {
//        auto n = odbcx::query_one<long>(std::move(cursor), "select count(id) from modifiable_cursor_test");
//        BOOST_CHECK_EQUAL(!n, false);
//        BOOST_CHECK_EQUAL(*n, item_number - items2delete + 4);
//    }
//}
//
//BOOST_AUTO_TEST_CASE(DynamicallyBoundAdaptedModifiableCursorTest)
//{
//    static_assert(!odbcx::details::columns::IsSequenceStaticallyBindable<data::TestNoDataAdaptable>::value, "Oops! should NOT be staticaly bindable");
//    static_assert(odbcx::details::columns::IsSequenceDynamicallyBindable<data::TestNoDataAdaptable>::value, "Oops! should be dynamically bindable");
//
//    /*static constexpr*/ auto attributes = odbcx::attribute::concur_lock | odbcx::attribute::cursor_dynamic | odbcx::attribute::bookmarks;
//    auto cursor = odbcx::query<data::TestNoDataAdaptable>(dbc, attributes, "select id, ts, type, target, value from modifiable_cursor_test");
//    static_assert(std::is_same<odbcx::details::ModifiableCursor<data::TestNoDataAdaptable, decltype(attributes)>, decltype(cursor)>::value, "Oops! deduced  cursor type seems to be broken");
//
//    //auto buffer = cursor.fetch_buffer();
//    //BOOST_CHECK(!buffer);
//    auto range = cursor.fetch();
//    BOOST_CHECK_EQUAL(item_number, std::distance(range.begin(), range.end()));
//    BOOST_CHECK_EQUAL(item_number, std::distance(range.begin(), range.end()));
//    BOOST_CHECK_EQUAL(item_number, std::distance(range.begin(), range.end()));
//    auto range2 = cursor.fetch();
//    BOOST_CHECK_EQUAL(item_number, std::distance(range2.begin(), range2.end()));
//    auto range3 = cursor.fetch();
//    BOOST_CHECK_EQUAL(item_number, std::distance(range3.begin(), range3.end()));
//
//    int items2delete = 100;
//    BOOST_CHECK(item_number > items2delete);
//    auto begin = range.begin();
//    for (int i = 0; i != items2delete; ++i)
//        cursor.remove(begin++);
//
//    auto now = std::chrono::system_clock::now();
//
//    cursor.insert(data::TestNoDataAdaptable{ 1001, now, "type - inserted 1", std::vector<char>{'o', 'k', '1'}, 1011 });
//    cursor.insert(data::TestNoDataAdaptable{ 1002, now, "type - inserted 2", std::vector<char>{'o', 'k', '2'}, 1012 });
//    //cursor.insert(9001, SQL_TIMESTAMP_STRUCT{}, std::string{ "type - inserted 3" }, std::vector<char>{'o', 'k', '3'}, 111999);
//    //BOOST_CHECK_EQUAL(item_number, std::distance(range.begin(), range.end()));
//    cursor.apply_changes();
//    cursor.insert(1003, now, "type - inserted 3", std::vector<char>{'o', 'k', '3'}, 1013);
//    cursor.insert(1004, now, "type - inserted 4", std::vector<char>{'o', 'k', '4'}, 1014);
//    cursor.apply_changes();
//    {
//        //auto cursor = odbcx::query<long>(dbc, "select count(id) from modifiable_cursor_test");
//        auto cursor1 = odbcx::query<long>(std::move(cursor), "select count(id) from modifiable_cursor_test");
//        auto range = cursor1.fetch();
//        BOOST_CHECK_EQUAL(range.empty(), false);
//        auto i = range.begin();
//        BOOST_CHECK_EQUAL(std::distance(i, range.end()), 1);
//        BOOST_CHECK_EQUAL(std::get<0>(*i), item_number - items2delete + 4);
//    }
//}

BOOST_AUTO_TEST_SUITE_END()


