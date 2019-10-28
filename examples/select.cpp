#include "odbcx/iterator.hpp"
#include "odbcx/query.hpp"
#include "odbcx/utility.hpp"
#include <boost/fusion/include/adapt_struct.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <tuple>
#include <cstdint>
#include <cassert>

namespace data
{
    struct Test
    {
        int id;
        SQL_TIMESTAMP_STRUCT ts;
        std::vector<char> target;
        std::string messagetype;
        std::vector<std::uint8_t> pb;
    };
} // namespace data

BOOST_FUSION_ADAPT_STRUCT(
    data::Test,
    id,
    ts,
    target,
    messagetype,
    pb
)

int main(int argc, char* argv[])
{
    try
    {
        if (argc == 1)
        {
            std::cerr << "please specify connection string or DSN" << std::endl;
            return 1;
        }

        auto env = odbcx::handle::allocate<SQL_HANDLE_ENV>();
        odbcx::SQLSetEnvAttr(env, SQL_ATTR_ODBC_VERSION, SQL_OV_ODBC3_80);
        auto  dbc = odbcx::handle::allocate<SQL_HANDLE_DBC>(env);
        odbcx::connect(dbc, argv[1]);

        // clean up the test table 
        odbcx::query(dbc, "delete from test");

        // make sure that the test table is empty
        auto n = odbcx::query_one<long>(dbc, "SELECT count(id) FROM test");
        assert(n.get() == 0);

        // let's insert some data
        auto type1 = std::string{ "type 1" };
        odbcx::query(dbc, "insert into test (target, messagetype, n) values(?,?,?)", "first", type1, 500);
        odbcx::query(dbc, "insert into test (target, messagetype, pb) values(?,?,?)", std::string{ "second" }, type1, std::vector<std::uint8_t>(100 * 1024, 0xF));
        auto empty = std::string{};
        odbcx::query(dbc, "insert into test (target, messagetype, pb) values(?,?,?)", empty, type1, std::vector<std::uint8_t>{});

        //make sure that 3 recors has been inserted
        n = odbcx::query_one<long>(dbc, "SELECT count(id) FROM test");
        assert(n.get() == 3);

        //lets fetch the data
        auto range = fetch_range(odbcx::query<std::tuple<int, SQL_TIMESTAMP_STRUCT, std::string, std::vector<std::uint8_t>> >(dbc, "SELECT id, ts, target, pb FROM test where messagetype = ?", type1));
        for (auto const& rec : range)
        {
            auto const& id = std::get<int>(rec);
            auto const& ts = std::get<SQL_TIMESTAMP_STRUCT>(rec);
            auto const& target = std::get<std::string>(rec);
            auto const& blob = std::get<std::vector<std::uint8_t>>(rec);
        }

        auto range1 = fetch_range(odbcx::select<data::Test>{}.from("test").where("messagetype=?", type1).exec(dbc));
        assert(std::distance(range1.begin(), range1.end()) == 3);
        auto range2 = fetch_range(odbcx::select<data::Test>{}.from("test").where("messagetype=?", type1).order_by("id ASC").exec(dbc));
        assert(std::distance(range2.begin(), range2.end()) == 3);
        auto range3 = fetch_range(odbcx::select<data::Test>{}.from("test").as("alias").where("alias.messagetype='test'").order_by("id ASC").exec(dbc));
        for (auto const& rec : range3)
        {
        }

    }
    catch (std::exception & e)
    {
        std::cerr << "error : " << e.what() << std::endl;
    }
    catch (...)
    {
        std::cerr << "miserable failure" << std::endl;
    }
    return 0;
}
