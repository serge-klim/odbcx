#include "test.hpp"
#include "dd.hpp"
#include "odbcx/query.hpp"
#include <boost/format.hpp>
#include <boost/fusion/adapted/std_tuple.hpp>
#include <boost/test/unit_test.hpp>
#include <algorithm>
#include <cstdint>



struct BlobTestSuiteFixture
{
    void init_test(odbcx::handle::adapter::Dbc dbc, test::OdbcDriverType /*driver_type*/)
    {
        test::create_table(dbc, "blob_test"); 

        for (records_added = 0; records_added < 20; ++records_added)
            odbcx::query(dbc, "insert into blob_test (id, target, type, bin_data) values(?,?,?,?)",
                9000 + records_added, str(boost::format("test %1%") % records_added), str(boost::format("long binary %1%") % records_added), 
                std::vector<std::uint8_t>(bin_data_size, 0xA + records_added));
    }
    const std::size_t bin_data_size = 100 * 1024;
    int records_added = 0;
};

BOOST_FIXTURE_TEST_SUITE(blob_test_suite, BlobTestSuiteFixture)

BOOST_DATA_TEST_CASE(InputOutputJustLongBinTest, boost::unit_test::data::make_delayed<test::AnyDBConnection>(), dbc, driver_type, driver_version)
{
    init_test(dbc, driver_type);
    auto cursor = odbcx::query<std::vector<std::uint8_t>>(dbc, "select bin_data FROM blob_test order by id");
    auto range = cursor.fetch();
    BOOST_CHECK(!range.empty());
    auto i = range.begin();
    auto next = range.begin();
    for (auto i = 0; i < records_added; ++i, ++next)
    {
        BOOST_REQUIRE(next != range.end());
        auto val1 = *next;
        BOOST_CHECK_EQUAL(std::count(begin(std::get<0>(val1)), end(std::get<0>(val1)), 0xA + i), bin_data_size);
    }
    BOOST_CHECK(next == range.end());
}

BOOST_DATA_TEST_CASE(InputOutputIdAndLongBinTest, boost::unit_test::data::make_delayed<test::AnyDBConnection>(), dbc, driver_type, driver_version)
{
    init_test(dbc, driver_type);
    auto cursor = odbcx::query<int, std::vector<std::uint8_t>>(dbc, "select id, bin_data FROM blob_test order by id");
    auto range = cursor.fetch();
    BOOST_CHECK(!range.empty());
    auto next = range.begin();
    for (auto i = 0; i < records_added; ++i, ++next)
    {
        BOOST_REQUIRE(next != range.end());
        auto val1 = *next;
        BOOST_CHECK_EQUAL(std::get<0>(val1), 9000 + i);
        BOOST_CHECK_EQUAL(std::count(begin(std::get<1>(val1)), end(std::get<1>(val1)), 0xA + i), bin_data_size);
    }
    BOOST_CHECK(next == range.end());
}

BOOST_DATA_TEST_CASE(InputOutputIdTypeAndLongBinTest, boost::unit_test::data::make_delayed<test::AnyDBConnection>(), dbc, driver_type, driver_version)
{
    init_test(dbc, driver_type);
    auto cursor = odbcx::query<int, std::string, std::vector<std::uint8_t>>(dbc, "select id, type, bin_data FROM blob_test order by id");
    auto range = cursor.fetch();
    BOOST_CHECK(!range.empty());
    auto next = range.begin();
    for (auto i = 0; i < records_added; ++i, ++next)
    {
        BOOST_REQUIRE(next != range.end());
        auto val1 = *next;
        BOOST_CHECK_EQUAL(std::get<0>(val1), 9000 + i);
        BOOST_CHECK_EQUAL(std::get<1>(val1), str(boost::format("long binary %1%") % i));
        BOOST_CHECK_EQUAL(std::count(begin(std::get<2>(val1)), end(std::get<2>(val1)), 0xA + i), bin_data_size);
    }
    BOOST_CHECK(next == range.end());
}

BOOST_DATA_TEST_CASE(InputOutputNotOptimalIdAndLongBinTest, boost::unit_test::data::make_delayed<test::AnyDBConnection>(), dbc, driver_type, driver_version)
{
    init_test(dbc, driver_type);
    auto cursor = odbcx::query<std::vector<std::uint8_t>, int>(dbc, "select bin_data, id FROM blob_test order by id");
    auto range = cursor.fetch();
    BOOST_CHECK(!range.empty());
    auto next = range.begin();
    for (auto i = 0; i < records_added; ++i, ++next)
    {
        BOOST_REQUIRE(next != range.end());
        auto val1 = *next;
        BOOST_CHECK_EQUAL(std::count(begin(std::get<0>(val1)), end(std::get<0>(val1)), 0xA + i), bin_data_size);
        BOOST_CHECK_EQUAL(std::get<1>(val1), 9000 + i);
    }
    BOOST_CHECK(next == range.end());
}

BOOST_DATA_TEST_CASE(InputOutputNotOptimalIdTypeAndLongBinTest, boost::unit_test::data::make_delayed<test::AnyDBConnection>(), dbc, driver_type, driver_version)
{
    init_test(dbc, driver_type);
    auto cursor = odbcx::query<std::vector<std::uint8_t>, std::string, int>(dbc, "select bin_data, type, id FROM blob_test order by id");
    auto range = cursor.fetch();
    BOOST_CHECK(!range.empty());
    auto next = range.begin();
    for (auto i = 0; i < records_added; ++i, ++next)
    {
        BOOST_REQUIRE(next != range.end());
        auto val1 = *next;
        BOOST_CHECK_EQUAL(std::count(begin(std::get<0>(val1)), end(std::get<0>(val1)), 0xA + i), bin_data_size);
        BOOST_CHECK_EQUAL(std::get<1>(val1), str(boost::format("long binary %1%") % i));
        BOOST_CHECK_EQUAL(std::get<2>(val1), 9000 + i);
    }
    BOOST_CHECK(next == range.end());
}


BOOST_DATA_TEST_CASE(InputOutputLongBinTest, boost::unit_test::data::make_delayed<test::AnyDBConnection>(), dbc, driver_type, driver_version)
{
    init_test(dbc, driver_type);
    auto cursor = odbcx::select<data::Test>{}.from("blob_test").order_by("id").exec(dbc);
    auto range = cursor.fetch();
    auto next = range.begin();
    for (auto i = 0; i < records_added; ++i, ++next)
    {
        BOOST_REQUIRE(next != range.end());
        auto val1 = *next;
        auto test = str(boost::format("test %1%") % i);
        BOOST_CHECK_EQUAL_COLLECTIONS(begin(test), end(test),
                                    begin(val1.target), end(val1.target));
        BOOST_CHECK(val1.ts == std::chrono::system_clock::time_point{});
        BOOST_CHECK_EQUAL(val1.type, str(boost::format("long binary %1%") % i));
        BOOST_CHECK_EQUAL(val1.bin_data.size(), bin_data_size);
        BOOST_CHECK_EQUAL(std::count(begin(val1.bin_data), end(val1.bin_data), 0xA + i), bin_data_size);
    }
    BOOST_CHECK(next == range.end());
}


BOOST_DATA_TEST_CASE(QueryOneLongBinTest, boost::unit_test::data::make_delayed<test::AnyDBConnection>(), dbc, driver_type, driver_version)
{
    test::create_table(dbc, "blob_test");
    constexpr std::size_t bin_data_size = 100 * 1024;
    odbcx::query(dbc, "insert into blob_test (target, type, bin_data) values(?,?,?)", std::string{ "test1" }, "long binary", std::vector<std::uint8_t>(bin_data_size, 0xF));
    //auto optional = odbcx::query_one<data::Test>(dbc, "select id, ts, type, target, sum, value, bin_data FROM blob_test where target = ?", "test1");
    auto optional = odbcx::query_one<data::Test>(dbc, "select id, ts, type, target, sum, value, bin_data FROM blob_test");
    BOOST_CHECK_EQUAL(!optional, false);
    auto val = *optional;
    auto test = std::string{ "test1" };
    BOOST_CHECK_EQUAL_COLLECTIONS(begin(test), end(test),
                                    begin(val.target), end(val.target));
    BOOST_CHECK(val.ts == std::chrono::system_clock::time_point{});
    BOOST_CHECK_EQUAL(val.type, "long binary");
    BOOST_CHECK_EQUAL(val.bin_data.size(), bin_data_size);
    BOOST_CHECK_EQUAL(std::count(begin(val.bin_data), end(val.bin_data), 0xF), bin_data_size);
}

BOOST_DATA_TEST_CASE(QueryOneJustLongBinTest, boost::unit_test::data::make_delayed<test::AnyDBConnection>(), dbc, driver_type, driver_version)
{
    test::create_table(dbc, "blob_test");
    constexpr std::size_t bin_data_size = 100 * 1024;
    odbcx::query(dbc, "insert into blob_test (target, type, bin_data) values(?,?,?)", std::string{ "test2" }, "long binary", std::vector<std::uint8_t>(bin_data_size, 0xF));
    //auto optional = odbcx::query_one<std::vector<std::uint8_t>>(dbc, "select bin_data FROM blob_test where target=?", "test2");
    auto optional = odbcx::query_one<std::vector<std::uint8_t>>(dbc, "select bin_data FROM blob_test");
    BOOST_CHECK_EQUAL(!optional, false);
    BOOST_CHECK_EQUAL(optional->size(), bin_data_size);
    BOOST_CHECK_EQUAL(std::count(begin(*optional), end(*optional), 0xF), bin_data_size);
}

BOOST_DATA_TEST_CASE(QueryOne2LongBinTest, boost::unit_test::data::make_delayed<test::AnyDBConnection>(), dbc, driver_type, driver_version)
{
    test::create_table(dbc, "blob_test");
    constexpr std::size_t bin_data_size = 100 * 1024;
    odbcx::query(dbc, "insert into blob_test (target, type, bin_data) values(?,?,?)", std::string{ "test3" }, "long binary", std::vector<std::uint8_t>(bin_data_size, 0xF));
    //auto optional = odbcx::query_one<std::vector<std::uint8_t>, std::vector<std::uint8_t>>(dbc, "select bin_data, bin_data FROM blob_test where target=?", "test3");
    auto optional = odbcx::query_one<std::vector<std::uint8_t>, std::vector<std::uint8_t>>(dbc, "select bin_data, bin_data FROM blob_test");
    BOOST_CHECK_EQUAL(!optional, false);
    BOOST_CHECK_EQUAL(std::get<0>(*optional).size(), bin_data_size);
    BOOST_CHECK_EQUAL(std::count(begin(std::get<0>(*optional)), end(std::get<0>(*optional)), 0xF), bin_data_size);
    BOOST_CHECK_EQUAL(std::get<1>(*optional).size(), bin_data_size);
    BOOST_CHECK_EQUAL(std::count(begin(std::get<1>(*optional)), end(std::get<1>(*optional)), 0xF), bin_data_size);
}

BOOST_DATA_TEST_CASE(QueryOneNotOptimalIdTypeAndLongBinTest, boost::unit_test::data::make_delayed<test::AnyDBConnection>(), dbc, driver_type, driver_version)
{
    init_test(dbc, driver_type);
    odbcx::query(dbc, "insert into blob_test (id, type, bin_data) values(?,?,?)", 333, "long binary", std::vector<std::uint8_t>(bin_data_size, 0xF));
    auto optional = odbcx::query_one<std::vector<std::uint8_t>, std::string, int>(dbc, "select bin_data, type, id FROM blob_test order by id");
    BOOST_CHECK_EQUAL(!optional, false);
    BOOST_CHECK_EQUAL(std::count(begin(std::get<0>(*optional)), end(std::get<0>(*optional)), 0xF), bin_data_size);
    BOOST_CHECK_EQUAL(std::get<1>(*optional), "long binary");
    BOOST_CHECK_EQUAL(std::get<2>(*optional), 333);
}

BOOST_AUTO_TEST_SUITE_END()


