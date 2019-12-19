#include "test.hpp"
#include "dd.hpp"
#include "odbcx/query.hpp"
#include <boost/test/unit_test.hpp>
#include <boost/fusion/adapted/std_tuple.hpp>
#include <algorithm>
#include <cstdint>

BOOST_AUTO_TEST_SUITE(blob_test_suite)

BOOST_DATA_TEST_CASE(InputOutputLongBinTest, boost::unit_test::data::make_delayed<test::AnyDBConnection>(), dbc, driver_type, driver_version)
{
    test::create_table(dbc, "blob_test");
    constexpr std::size_t bin_data_size = 100 * 1024;
    odbcx::query(dbc, "insert into blob_test (target, type, bin_data) values(?,?,?)", std::string{ "test" }, "long  binary", std::vector<std::uint8_t>(bin_data_size, 0xF));
    auto cursor = odbcx::select<data::Test>{}.from("blob_test").exec(dbc);
    auto range = cursor.fetch();
    BOOST_CHECK(!range.empty());
    auto i = range.begin();
    auto test = std::string{ "test" };
    //BOOST_CHECK_EQUAL(i->target, "test");
    //BOOST_CHECK(i->ts == std::chrono::system_clock::time_point{});
    //BOOST_CHECK_EQUAL(i->type, "long  binary");
    //BOOST_CHECK_EQUAL(i->bin_data.size(), bin_data_size);
    //BOOST_CHECK_EQUAL(std::count(begin(i->bin_data), end(i->bin_data), 0xF), bin_data_size);
    auto val = *i;
    BOOST_CHECK_EQUAL_COLLECTIONS(begin(test), end(test),
                                    begin(val.target), end(val.target));
    BOOST_CHECK(val.ts == std::chrono::system_clock::time_point{});
    BOOST_CHECK_EQUAL(val.type, "long  binary");
    BOOST_CHECK_EQUAL(val.bin_data.size(), bin_data_size);
    BOOST_CHECK_EQUAL(std::count(begin(val.bin_data), end(val.bin_data), 0xF), bin_data_size);
    BOOST_CHECK(++i == range.end());
}

BOOST_DATA_TEST_CASE(QuearyOneLongBinTest, boost::unit_test::data::make_delayed<test::AnyDBConnection>(), dbc, driver_type, driver_version)
{
    test::create_table(dbc, "blob_test");
    constexpr std::size_t bin_data_size = 100 * 1024;
    odbcx::query(dbc, "insert into blob_test (target, type, bin_data) values(?,?,?)", std::string{ "test1" }, "long  binary", std::vector<std::uint8_t>(bin_data_size, 0xF));
    //auto optional = odbcx::query_one<data::Test>(dbc, "select id, ts, type, target, sum, value, bin_data FROM blob_test where target = ?", "test1");
    auto optional = odbcx::query_one<data::Test>(dbc, "select id, ts, type, target, sum, value, bin_data FROM blob_test");
    BOOST_CHECK_EQUAL(!optional, false);
    auto val = *optional;
    auto test = std::string{ "test1" };
    BOOST_CHECK_EQUAL_COLLECTIONS(begin(test), end(test),
                                    begin(val.target), end(val.target));
    BOOST_CHECK(val.ts == std::chrono::system_clock::time_point{});
    BOOST_CHECK_EQUAL(val.type, "long  binary");
    BOOST_CHECK_EQUAL(val.bin_data.size(), bin_data_size);
    BOOST_CHECK_EQUAL(std::count(begin(val.bin_data), end(val.bin_data), 0xF), bin_data_size);
}

BOOST_DATA_TEST_CASE(QuearyOneJustLongBinTest, boost::unit_test::data::make_delayed<test::AnyDBConnection>(), dbc, driver_type, driver_version)
{
    test::create_table(dbc, "blob_test");
    constexpr std::size_t bin_data_size = 100 * 1024;
    odbcx::query(dbc, "insert into blob_test (target, type, bin_data) values(?,?,?)", std::string{ "test2" }, "long  binary", std::vector<std::uint8_t>(bin_data_size, 0xF));
    //auto optional = odbcx::query_one<std::vector<std::uint8_t>>(dbc, "select bin_data FROM blob_test where target=?", "test2");
    auto optional = odbcx::query_one<std::vector<std::uint8_t>>(dbc, "select bin_data FROM blob_test");
    BOOST_CHECK_EQUAL(!optional, false);
    BOOST_CHECK_EQUAL(optional->size(), bin_data_size);
    BOOST_CHECK_EQUAL(std::count(begin(*optional), end(*optional), 0xF), bin_data_size);
}

BOOST_DATA_TEST_CASE(QuearyOne2LongBinTest, boost::unit_test::data::make_delayed<test::AnyDBConnection>(), dbc, driver_type, driver_version)
{
    test::create_table(dbc, "blob_test");
    constexpr std::size_t bin_data_size = 100 * 1024;
    odbcx::query(dbc, "insert into blob_test (target, type, bin_data) values(?,?,?)", std::string{ "test3" }, "long  binary", std::vector<std::uint8_t>(bin_data_size, 0xF));
    //auto optional = odbcx::query_one<std::vector<std::uint8_t>, std::vector<std::uint8_t>>(dbc, "select bin_data, bin_data FROM blob_test where target=?", "test3");
    auto optional = odbcx::query_one<std::vector<std::uint8_t>, std::vector<std::uint8_t>>(dbc, "select bin_data, bin_data FROM blob_test");
    BOOST_CHECK_EQUAL(!optional, false);
    BOOST_CHECK_EQUAL(std::get<0>(*optional).size(), bin_data_size);
    BOOST_CHECK_EQUAL(std::count(begin(std::get<0>(*optional)), end(std::get<0>(*optional)), 0xF), bin_data_size);
    BOOST_CHECK_EQUAL(std::get<1>(*optional).size(), bin_data_size);
    BOOST_CHECK_EQUAL(std::count(begin(std::get<1>(*optional)), end(std::get<1>(*optional)), 0xF), bin_data_size);
}

BOOST_AUTO_TEST_SUITE_END()


