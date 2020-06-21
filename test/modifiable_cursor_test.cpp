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
    
    cursor.insert(data::TestStaticNoData{ 900, timestamp, "type - inserted 1", "ok1-static", 1.1f });
    cursor.insert(data::TestStaticNoData{ 902,timestamp, "type - inserted 2", "ok2-static", 2.2f });
    //BOOST_CHECK_EQUAL(item_number, std::distance(range.begin(), range.end()));
    cursor.apply_changes();    
    cursor.insert(data::TestStaticNoData{ 1003, timestamp, "type - inserted 3", "ok3-static", 3.3f });
    cursor.insert(data::TestStaticNoData{ 1004, timestamp, "type - inserted 4", "ok4-static", 4.4f });
    cursor.apply_changes();

    auto n = odbcx::query_one<long>(dbc /*std::move(cursor)*/, "select count(id) from modifiable_cursor_test");
    BOOST_REQUIRE_EQUAL(!n, false);
    BOOST_CHECK_EQUAL(*n, item_number + 4);

    auto inserted1 = odbcx::query_one<data::TestStaticNoData>(dbc /*std::move(cursor)*/, "select id, ts, type, target, sum, value from modifiable_cursor_test where id = 900");
    BOOST_REQUIRE_EQUAL(!inserted1, false);
    BOOST_CHECK_EQUAL(inserted1->ts.year, timestamp.year);
    BOOST_CHECK_EQUAL(inserted1->ts.month, timestamp.month);
    BOOST_CHECK_EQUAL(inserted1->ts.day, timestamp.day);
    BOOST_CHECK_EQUAL(inserted1->ts.hour, timestamp.hour);
    BOOST_CHECK_EQUAL(inserted1->ts.minute, timestamp.minute);
    BOOST_CHECK_EQUAL(inserted1->ts.second, timestamp.second);

    BOOST_CHECK_EQUAL(inserted1->type, "type - inserted 1");
    BOOST_CHECK_EQUAL(inserted1->target, "ok1-static");
    BOOST_CHECK_EQUAL(inserted1->sum, 1.1f);

    auto inserted2 = odbcx::query_one<data::TestStaticNoData>(dbc /*std::move(cursor)*/, "select id, ts, type, target, sum, value from modifiable_cursor_test where id = 902");
    BOOST_REQUIRE_EQUAL(!inserted2, false);
    BOOST_CHECK_EQUAL(inserted2->ts.year, timestamp.year);
    BOOST_CHECK_EQUAL(inserted2->ts.month, timestamp.month);
    BOOST_CHECK_EQUAL(inserted2->ts.day, timestamp.day);
    BOOST_CHECK_EQUAL(inserted2->ts.hour, timestamp.hour);
    BOOST_CHECK_EQUAL(inserted2->ts.minute, timestamp.minute);
    BOOST_CHECK_EQUAL(inserted2->ts.second, timestamp.second);

    BOOST_CHECK_EQUAL(inserted2->type, "type - inserted 2");
    BOOST_CHECK_EQUAL(inserted2->target, "ok2-static");
    BOOST_CHECK_EQUAL(inserted2->sum, 2.2f);

    auto inserted3 = odbcx::query_one<data::TestStaticNoData>(dbc /*std::move(cursor)*/, "select id, ts, type, target, sum, value from modifiable_cursor_test where id = 1003");
    BOOST_REQUIRE_EQUAL(!inserted3, false);
    BOOST_CHECK_EQUAL(inserted3->ts.year, timestamp.year);
    BOOST_CHECK_EQUAL(inserted3->ts.month, timestamp.month);
    BOOST_CHECK_EQUAL(inserted3->ts.day, timestamp.day);
    BOOST_CHECK_EQUAL(inserted3->ts.hour, timestamp.hour);
    BOOST_CHECK_EQUAL(inserted3->ts.minute, timestamp.minute);
    BOOST_CHECK_EQUAL(inserted3->ts.second, timestamp.second);

    BOOST_CHECK_EQUAL(inserted3->type, "type - inserted 3");
    BOOST_CHECK_EQUAL(inserted3->target, "ok3-static");
    BOOST_CHECK_EQUAL(inserted3->sum, 3.3f);

    auto inserted4 = odbcx::query_one<data::TestStaticNoData>(dbc /*std::move(cursor)*/, "select id, ts, type, target, sum, value from modifiable_cursor_test where id = 1004");
    BOOST_REQUIRE_EQUAL(!inserted4, false);
    BOOST_CHECK_EQUAL(inserted4->ts.year, timestamp.year);
    BOOST_CHECK_EQUAL(inserted4->ts.month, timestamp.month);
    BOOST_CHECK_EQUAL(inserted4->ts.day, timestamp.day);
    BOOST_CHECK_EQUAL(inserted4->ts.hour, timestamp.hour);
    BOOST_CHECK_EQUAL(inserted4->ts.minute, timestamp.minute);
    BOOST_CHECK_EQUAL(inserted4->ts.second, timestamp.second);

    BOOST_CHECK_EQUAL(inserted4->type, "type - inserted 4");
    BOOST_CHECK_EQUAL(inserted4->target, "ok4-static");
    BOOST_CHECK_EQUAL(inserted4->sum, 4.4f);
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
    cursor.apply_changes();

    auto n = odbcx::query_one<long>(dbc /*std::move(cursor)*/, "select count(id) from modifiable_cursor_test");
    BOOST_REQUIRE_EQUAL(!n, false);
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
    cursor.emplace(1003, timestamp, "type - inserted 3", std::vector<char>{'o', 'k', '3'}, 1013);
    cursor.emplace(1004, timestamp, "type - inserted 4", std::vector<char>{'o', 'k', '4'}, 1014);
    cursor.apply_changes();

    auto n = odbcx::query_one<long>(dbc /*std::move(cursor)*/, "select count(id) from modifiable_cursor_test");
    BOOST_REQUIRE_EQUAL(!n, false);
    BOOST_CHECK_EQUAL(*n, item_number + 4);

    auto inserted1 = odbcx::query_one<data::TestNoData>(dbc /*std::move(cursor)*/, "select id, ts, type, target, value from modifiable_cursor_test where id = 1001");
    BOOST_REQUIRE_EQUAL(!inserted1, false);
    BOOST_CHECK_EQUAL(inserted1->ts.year, timestamp.year);
    BOOST_CHECK_EQUAL(inserted1->ts.month, timestamp.month);
    BOOST_CHECK_EQUAL(inserted1->ts.day, timestamp.day);
    BOOST_CHECK_EQUAL(inserted1->ts.hour, timestamp.hour);
    BOOST_CHECK_EQUAL(inserted1->ts.minute, timestamp.minute);
    BOOST_CHECK_EQUAL(inserted1->ts.second, timestamp.second);

    BOOST_CHECK_EQUAL(inserted1->type, "type - inserted 1");
    BOOST_CHECK_EQUAL(diversion::string_view(inserted1->target.data(), inserted1->target.size()), "ok1");
    BOOST_CHECK_EQUAL(inserted1->value, 1011);

    auto inserted2 = odbcx::query_one<data::TestNoData>(dbc /*std::move(cursor)*/, "select id, ts, type, target, value from modifiable_cursor_test where id = 1002");
    BOOST_REQUIRE_EQUAL(!inserted2, false);
    BOOST_CHECK_EQUAL(inserted2->ts.year, timestamp.year);
    BOOST_CHECK_EQUAL(inserted2->ts.month, timestamp.month);
    BOOST_CHECK_EQUAL(inserted2->ts.day, timestamp.day);
    BOOST_CHECK_EQUAL(inserted2->ts.hour, timestamp.hour);
    BOOST_CHECK_EQUAL(inserted2->ts.minute, timestamp.minute);
    BOOST_CHECK_EQUAL(inserted2->ts.second, timestamp.second);

    BOOST_CHECK_EQUAL(inserted2->type, "type - inserted 2");
    BOOST_CHECK_EQUAL(diversion::string_view(inserted2->target.data(), inserted2->target.size()), "ok2");
    BOOST_CHECK_EQUAL(inserted2->value, 1012);


    auto inserted3 = odbcx::query_one<data::TestNoData>(dbc /*std::move(cursor)*/, "select id, ts, type, target, value from modifiable_cursor_test where id = 1003");
    BOOST_REQUIRE_EQUAL(!inserted3, false);
    BOOST_CHECK_EQUAL(inserted3->ts.year, timestamp.year);
    BOOST_CHECK_EQUAL(inserted3->ts.month, timestamp.month);
    BOOST_CHECK_EQUAL(inserted3->ts.day, timestamp.day);
    BOOST_CHECK_EQUAL(inserted3->ts.hour, timestamp.hour);
    BOOST_CHECK_EQUAL(inserted3->ts.minute, timestamp.minute);
    BOOST_CHECK_EQUAL(inserted3->ts.second, timestamp.second);

    BOOST_CHECK_EQUAL(inserted3->type, "type - inserted 3");
    BOOST_CHECK_EQUAL(diversion::string_view(inserted3->target.data(), inserted3->target.size()), "ok3");
    BOOST_CHECK_EQUAL(inserted3->value, 1013);


    auto inserted4 = odbcx::query_one<data::TestNoData>(dbc /*std::move(cursor)*/, "select id, ts, type, target, value from modifiable_cursor_test where id = 1004");
    BOOST_REQUIRE_EQUAL(!inserted4, false);
    BOOST_CHECK_EQUAL(inserted4->ts.year, timestamp.year);
    BOOST_CHECK_EQUAL(inserted4->ts.month, timestamp.month);
    BOOST_CHECK_EQUAL(inserted4->ts.day, timestamp.day);
    BOOST_CHECK_EQUAL(inserted4->ts.hour, timestamp.hour);
    BOOST_CHECK_EQUAL(inserted4->ts.minute, timestamp.minute);
    BOOST_CHECK_EQUAL(inserted4->ts.second, timestamp.second);

    BOOST_CHECK_EQUAL(inserted4->type, "type - inserted 4");
    BOOST_CHECK_EQUAL(diversion::string_view(inserted4->target.data(), inserted4->target.size()), "ok4");
    BOOST_CHECK_EQUAL(inserted4->value, 1014);
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
    cursor.emplace(1003, now, "type - inserted 3", std::vector<char>{'o', 'k', '3'}, 1013);
    cursor.emplace(1004, now, "type - inserted 4", std::vector<char>{'o', 'k', '4'}, 'a');
    cursor.apply_changes();

    auto n = odbcx::query_one<long>(dbc /*std::move(cursor)*/, "select count(id) from modifiable_cursor_test");
    BOOST_REQUIRE_EQUAL(!n, false);
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
    cursor.emplace(1003, timestamp, std::string{ "type - inserted 3" }, std::vector<char>{'o', 'k', '3'}, diversion::nullopt, 1013);
    cursor.emplace(1004, timestamp, std::string{ "type - inserted 4" }, std::vector<char>{'o', 'k', '4'}, diversion::nullopt, 'a');
    cursor.emplace(1005, timestamp, std::string{ "type - inserted 5" }, std::vector<char>{'o', 'k', '5'}, 555., diversion::nullopt);
    cursor.emplace(1006, timestamp, std::string{ "type - inserted 6" }, diversion::nullopt, diversion::nullopt, diversion::nullopt);
    cursor.emplace(1007, diversion::make_optional(timestamp), std::string{ "type - inserted 7" }, diversion::make_optional(std::vector<char>{'o', 'k', '6'}), 345, diversion::nullopt);
    cursor.apply_changes();

    auto n = odbcx::query_one<long>(dbc /*std::move(cursor)*/, "select count(id) from modifiable_cursor_test");
    BOOST_REQUIRE_EQUAL(!n, false);
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
    cursor.emplace(1003, data::BizarreTimepoint{ now }, data::BizarreString{ "type - inserted 3" }, data::BizarreVector<char>{std::vector<char>{'o', 'k', '3'}}, data::BizarreDouble{}, 1013);
    cursor.emplace(1004, data::BizarreTimepoint{ now }, data::BizarreString{ "type - inserted 4" }, data::BizarreVector<char>{std::vector<char>{'o', 'k', '4'}}, data::BizarreDouble{}, 'a');
    cursor.emplace(diversion::make_optional(1005), data::BizarreTimepoint{ now }, data::BizarreString{ "type - inserted 5" }, data::BizarreVector<char>{std::vector<char>{'o', 'k', '5'}}, 555., data::BizarreInt{});
    cursor.apply_changes();

    auto n = odbcx::query_one<long>(dbc /*std::move(cursor)*/, "select count(id) from modifiable_cursor_test");
    BOOST_REQUIRE_EQUAL(!n, false);
    BOOST_CHECK_EQUAL(*n, item_number + 5);
}

BOOST_DATA_TEST_CASE(DynamicallyBoundModifiableCursorWithCArrayTestNoDataAdaptableInsertTest, boost::unit_test::data::make_delayed<test::AnyDBConnection>(), dbc, driver_type, driver_version)
{
    init_test(dbc, driver_type);

    /*static constexpr*/ auto attributes = odbcx::attribute::concur_lock | odbcx::attribute::cursor_keyset_driven | odbcx::attribute::bookmarks;
    ///*static constexpr*/ auto attributes = odbcx::attribute::concur_rowver | odbcx::attribute::cursor_static | odbcx::attribute::bookmarks;
    auto cursor = odbcx::select<data::TestMixedNoData>{}.from("modifiable_cursor_test").exec(dbc, attributes);

    auto now = std::chrono::system_clock::now();
    cursor.insert(data::TestMixedNoData{ 1001, now, "type - inserted 1", {'o', 'k', '1'}, 1.1f, 1011 });
    cursor.insert(data::TestMixedNoData{ 1002, now, "type - inserted 2", {'o', 'k', '2'}, 2.2f, 1012 });
    cursor.apply_changes();
#pragma message("make it work")
    //cursor.emplace(1003, now, "type - inserted 3", "ok3", .1, 1013);
    //cursor.emplace(1004, now, "type - inserted 4", "ok4", .1, 'a');
    cursor.apply_changes();

    auto n = odbcx::query_one<long>(dbc /*std::move(cursor)*/, "select count(id) from modifiable_cursor_test");
    BOOST_REQUIRE_EQUAL(!n, false);
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
    cursor.apply_changes();

    auto n = odbcx::query_one<long>(dbc /*std::move(cursor)*/, "select count(id) from modifiable_cursor_test");
    BOOST_REQUIRE_EQUAL(!n, false);
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
    cursor.apply_changes();

    auto n = odbcx::query_one<long>(dbc /*std::move(cursor)*/, "select count(id) from modifiable_cursor_test");
    BOOST_REQUIRE_EQUAL(!n, false);
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
    cursor.apply_changes();

    auto n = odbcx::query_one<long>(dbc /*std::move(cursor)*/, "select count(id) from modifiable_cursor_test");
    BOOST_REQUIRE_EQUAL(!n, false);
    BOOST_CHECK_EQUAL(*n, item_number - items2delete);
}


BOOST_DATA_TEST_CASE(StaticallyBoundModifiableCursorDeleteInsertTest,
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

    cursor.apply_changes();

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

    cursor.insert(data::TestStaticNoData{ 900, timestamp, "type - inserted 1", "ok1-static", 1.1f });
    cursor.insert(data::TestStaticNoData{ 9001,timestamp, "type - inserted 2", "ok2-static", 2.2f });
    cursor.apply_changes();

    auto n = odbcx::query_one<long>(dbc /*std::move(cursor)*/, "select count(id) from modifiable_cursor_test");
    BOOST_REQUIRE_EQUAL(!n, false);
    BOOST_CHECK_EQUAL(*n, item_number - items2delete + 2/*inserted*/);

    auto inserted1 = odbcx::query_one<data::TestStaticNoData>(dbc /*std::move(cursor)*/, "select id, ts, type, target, sum, value from modifiable_cursor_test where id = 900");
    BOOST_REQUIRE_EQUAL(!inserted1, false);
    BOOST_CHECK_EQUAL(inserted1->ts.year, timestamp.year);
    BOOST_CHECK_EQUAL(inserted1->ts.month, timestamp.month);
    BOOST_CHECK_EQUAL(inserted1->ts.day, timestamp.day);
    BOOST_CHECK_EQUAL(inserted1->ts.hour, timestamp.hour);
    BOOST_CHECK_EQUAL(inserted1->ts.minute, timestamp.minute);
    BOOST_CHECK_EQUAL(inserted1->ts.second, timestamp.second);

    BOOST_CHECK_EQUAL(inserted1->type, "type - inserted 1");
    BOOST_CHECK_EQUAL(inserted1->target, "ok1-static");
    BOOST_CHECK_EQUAL(inserted1->sum, 1.1f);

    auto inserted2 = odbcx::query_one<data::TestStaticNoData>(dbc /*std::move(cursor)*/, "select id, ts, type, target, sum, value from modifiable_cursor_test where id = 9001");
    BOOST_REQUIRE_EQUAL(!inserted2, false);
    BOOST_CHECK_EQUAL(inserted2->ts.year, timestamp.year);
    BOOST_CHECK_EQUAL(inserted2->ts.month, timestamp.month);
    BOOST_CHECK_EQUAL(inserted2->ts.day, timestamp.day);
    BOOST_CHECK_EQUAL(inserted2->ts.hour, timestamp.hour);
    BOOST_CHECK_EQUAL(inserted2->ts.minute, timestamp.minute);
    BOOST_CHECK_EQUAL(inserted2->ts.second, timestamp.second);

    BOOST_CHECK_EQUAL(inserted2->type, "type - inserted 2");
    BOOST_CHECK_EQUAL(inserted2->target, "ok2-static");
    BOOST_CHECK_EQUAL(inserted2->sum, 2.2f);
}

BOOST_DATA_TEST_CASE(DynamicallyBoundModifiableCursorTestNoDataDeleteInsertTest,
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

    cursor.apply_changes();

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
    cursor.emplace(1003, timestamp, "type - inserted 3", std::vector<char>{'o', 'k', '3'}, 1013);
    cursor.apply_changes();

    auto n = odbcx::query_one<long>(dbc /*std::move(cursor)*/, "select count(id) from modifiable_cursor_test");
    BOOST_REQUIRE_EQUAL(!n, false);
    BOOST_CHECK_EQUAL(*n, item_number - items2delete + 2/*inserted*/);

    auto inserted1 = odbcx::query_one<data::TestNoData>(dbc /*std::move(cursor)*/, "select id, ts, type, target, value from modifiable_cursor_test where id = 1001");
    BOOST_REQUIRE_EQUAL(!inserted1, false);
    BOOST_CHECK_EQUAL(inserted1->ts.year, timestamp.year);
    BOOST_CHECK_EQUAL(inserted1->ts.month, timestamp.month);
    BOOST_CHECK_EQUAL(inserted1->ts.day, timestamp.day );
    BOOST_CHECK_EQUAL(inserted1->ts.hour, timestamp.hour);
    BOOST_CHECK_EQUAL(inserted1->ts.minute, timestamp.minute);
    BOOST_CHECK_EQUAL(inserted1->ts.second, timestamp.second);

    BOOST_CHECK_EQUAL(inserted1->type, "type - inserted 1");
    BOOST_CHECK_EQUAL(diversion::string_view( inserted1->target.data(), inserted1->target.size() ), "ok1");
    BOOST_CHECK_EQUAL(inserted1->value, 1011);

    auto inserted2 = odbcx::query_one<data::TestNoData>(dbc /*std::move(cursor)*/, "select id, ts, type, target, value from modifiable_cursor_test where id = 1003");
    BOOST_REQUIRE_EQUAL(!inserted2, false);
    BOOST_CHECK_EQUAL(inserted2->ts.year, timestamp.year);
    BOOST_CHECK_EQUAL(inserted2->ts.month, timestamp.month);
    BOOST_CHECK_EQUAL(inserted2->ts.day, timestamp.day);
    BOOST_CHECK_EQUAL(inserted2->ts.hour, timestamp.hour);
    BOOST_CHECK_EQUAL(inserted2->ts.minute, timestamp.minute);
    BOOST_CHECK_EQUAL(inserted2->ts.second, timestamp.second);

    BOOST_CHECK_EQUAL(inserted2->type, "type - inserted 3");
    BOOST_CHECK_EQUAL(diversion::string_view( inserted2->target.data(), inserted2->target.size() ), "ok3");
    BOOST_CHECK_EQUAL(inserted2->value, 1013);
}

BOOST_DATA_TEST_CASE(DynamicallyBoundModifiableCursorTestNoDataAdaptableDeleteInsertTest,
                            (boost::unit_test::data::make_delayed<test::DBConnections<
                                                test::Except<test::OdbcDriverType::mysql>,
                                                test::Except<test::OdbcDriverType::psql, 10>
                                                >>()),
                            dbc, driver_type, driver_version)

{
    init_test(dbc, driver_type);

    /*static constexpr*/ auto attributes = odbcx::attribute::concur_lock | odbcx::attribute::cursor_keyset_driven | odbcx::attribute::bookmarks;
    ///*static constexpr*/ auto attributes = odbcx::attribute::concur_rowver | odbcx::attribute::cursor_static | odbcx::attribute::bookmarks;
    auto cursor = odbcx::select<data::TestNoDataAdaptable>{}.from("modifiable_cursor_test").exec(dbc, attributes);

    auto range = cursor.fetch();
    std::size_t items2delete = 100;
    BOOST_CHECK(item_number > items2delete);
    auto begin = range.begin();
    for (int i = 0; i != items2delete; ++i)
        cursor.remove(begin++);

    cursor.apply_changes();

    auto now = std::chrono::system_clock::now();
    cursor.insert(data::TestNoDataAdaptable{ 1001, now, "type - inserted 1", std::vector<char>{'o', 'k', '1'}, 1011 });
    cursor.emplace(1002, now, "type - inserted 2", std::vector<char>{'o', 'k', '2'}, 1012);
    cursor.emplace(1003, now, "type - inserted 3", std::vector<char>{'o', 'k', '3'}, 1013);
    cursor.apply_changes();

    auto n = odbcx::query_one<long>(dbc /*std::move(cursor)*/, "select count(id) from modifiable_cursor_test");
    BOOST_REQUIRE_EQUAL(!n, false);
    BOOST_CHECK_EQUAL(*n, item_number - items2delete + 3/*inserted*/);

    auto inserted1 = odbcx::query_one<data::TestNoDataAdaptable>(dbc /*std::move(cursor)*/, "select id, ts, type, target, value from modifiable_cursor_test where id = 1001");
    BOOST_REQUIRE_EQUAL(!inserted1, false);
    BOOST_CHECK(std::chrono::duration_cast<std::chrono::seconds>(inserted1->ts.time_since_epoch()) == std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()));
    BOOST_CHECK_EQUAL(inserted1->type, "type - inserted 1");
    BOOST_CHECK_EQUAL(diversion::string_view( inserted1->target.data(), inserted1->target.size() ), "ok1");
    BOOST_CHECK_EQUAL(inserted1->value, 1011);

    auto inserted2 = odbcx::query_one<data::TestNoDataAdaptable>(dbc /*std::move(cursor)*/, "select id, ts, type, target, value from modifiable_cursor_test where id = 1002");
    BOOST_REQUIRE_EQUAL(!inserted2, false);
    BOOST_CHECK(std::chrono::duration_cast<std::chrono::seconds>(inserted2->ts.time_since_epoch()) == std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()));

    BOOST_CHECK_EQUAL(inserted2->type, "type - inserted 2");
    BOOST_CHECK_EQUAL(diversion::string_view(inserted2->target.data(), inserted2->target.size()), "ok2");
    BOOST_CHECK_EQUAL(inserted2->value, 1012);

    auto inserted3 = odbcx::query_one<data::TestNoDataAdaptable>(dbc /*std::move(cursor)*/, "select id, ts, type, target, value from modifiable_cursor_test where id = 1003");
    BOOST_REQUIRE_EQUAL(!inserted3, false);
    BOOST_CHECK(std::chrono::duration_cast<std::chrono::seconds>(inserted3->ts.time_since_epoch()) == std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()));

    BOOST_CHECK_EQUAL(inserted3->type, "type - inserted 3");
    BOOST_CHECK_EQUAL(diversion::string_view( inserted3->target.data(), inserted3->target.size() ), "ok3");
    BOOST_CHECK_EQUAL(inserted3->value, 1013);
}

BOOST_AUTO_TEST_SUITE_END()


