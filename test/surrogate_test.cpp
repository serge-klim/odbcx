#include "test.hpp"
#include "odbcx/utility.hpp"
#include "odbcx/query.hpp"
#include "odbcx/handle.hpp"
#include "odbcx/bindings/surrogate.hpp"
#include "odbcx/attribute.hpp"
#include "odbcx/details/diversion.hpp"
#include <boost/test/unit_test.hpp>
#include <vector>
#include <string>
#include <cstdint>

struct SurrogateTestSuiteFixture
{
    void init_test(odbcx::handle::adapter::Dbc const& dbc)
    {
        test::create_table(dbc, "surrogate_test");
        auto stmt = odbcx::handle::allocate<SQL_HANDLE_STMT>(dbc, odbcx::attribute::concur_read_only);
        odbcx::query(stmt, "insert into surrogate_test (id, type, target) values (1, 'type1', '')");
        odbcx::query(stmt, "insert into surrogate_test (id, type, target, value) values (2, 'type2', 'target2', 222)");
    }
};

BOOST_FIXTURE_TEST_SUITE(surrogate_test_suit, SurrogateTestSuiteFixture)

BOOST_DATA_TEST_CASE(BasicSurrogateTest, boost::unit_test::data::make_delayed<test::AnyDBConnection>(), dbc, driver_type, driver_version)
{
    init_test(dbc);
    auto stmt = odbcx::handle::allocate<SQL_HANDLE_STMT>(dbc, odbcx::attribute::concur_read_only);
    odbcx::query(stmt, "select id, type, target, value, bin_data from surrogate_test where id = 1");
    long id = 0;
    SQLLEN id_ix;
    odbcx::call(&SQLBindCol, stmt, 1, SQL_C_LONG, &id, 0, &id_ix);

    odbcx::call(&SQLSetStmtAttr, stmt, SQL_ATTR_ROW_ARRAY_SIZE, reinterpret_cast<SQLPOINTER>(1), SQLINTEGER(0));
    SQLULEN fetched = 0;
    odbcx::call(&SQLSetStmtAttr, stmt, SQL_ATTR_ROWS_FETCHED_PTR, &fetched, SQLINTEGER(0));

    BOOST_CHECK_NE(odbcx::call(&SQLFetchScroll, stmt, SQL_FETCH_NEXT, 0), SQL_NO_DATA);
    BOOST_CHECK_EQUAL(fetched, 1);
    BOOST_CHECK_EQUAL(id, 1);

    auto type = odbcx::details::cast<std::string>(odbcx::SurrogateVector<char, SQL_C_CHAR>{stmt, 2});
    BOOST_CHECK_EQUAL(type, "type1");
    BOOST_CHECK_THROW((odbcx::SurrogateVector<char, SQL_C_CHAR>{stmt, 2}), std::runtime_error);

    auto target = odbcx::details::cast<std::string>(odbcx::SurrogateVector<char, SQL_C_CHAR>{ stmt, 3 });
    BOOST_CHECK(target.empty());
    BOOST_CHECK_THROW((odbcx::SurrogateVector<char, SQL_C_CHAR>{stmt, 3}), std::runtime_error);

    auto value = odbcx::details::cast<int>(odbcx::Surrogate<int>{ stmt, 4 });
    switch (driver_type)
    {
        case test::OdbcDriverType::psql:
        case test::OdbcDriverType::mysql:
            BOOST_CHECK_EQUAL(odbcx::details::cast<decltype(value)>(odbcx::Surrogate<int>{stmt, 4}), value); //pstgres allows to call it more than once
            break;
        default:
            BOOST_CHECK_THROW((odbcx::Surrogate<int>{stmt, 4}), std::runtime_error);
    }
    auto data = odbcx::details::cast<diversion::optional<std::vector<std::uint8_t>>>(odbcx::SurrogateVector<std::uint8_t, SQL_C_BINARY>{ stmt, 5 });
    BOOST_CHECK(!data);
    switch (driver_type)
    {
        case test::OdbcDriverType::psql:
        case test::OdbcDriverType::mysql:
            BOOST_CHECK((odbcx::SurrogateVector<std::uint8_t, SQL_C_BINARY>{stmt, 5}.is_null())); //pstgres allows to call it more than once
            break;
        default:
            BOOST_CHECK_THROW((odbcx::SurrogateVector<std::uint8_t, SQL_C_BINARY>{stmt, 5}), std::runtime_error);
    }
}

BOOST_DATA_TEST_CASE(OptionalSurrogateTest, boost::unit_test::data::make_delayed<test::AnyDBConnection>(), dbc, driver_type, driver_version)
{
    init_test(dbc);
    auto stmt = odbcx::handle::allocate<SQL_HANDLE_STMT>(dbc, odbcx::attribute::concur_read_only);
    odbcx::query(stmt, "select id, type, target, value, bin_data from surrogate_test where id = 1");
    long id = 0;
    SQLLEN id_ix;
    odbcx::call(&SQLBindCol, stmt, 1, SQL_C_LONG, &id, 0, &id_ix);

    odbcx::call(&SQLSetStmtAttr, stmt, SQL_ATTR_ROW_ARRAY_SIZE, reinterpret_cast<SQLPOINTER>(1), SQLINTEGER(0));
    SQLULEN fetched = 0;
    odbcx::call(&SQLSetStmtAttr, stmt, SQL_ATTR_ROWS_FETCHED_PTR, &fetched, SQLINTEGER(0));

    BOOST_CHECK_NE(odbcx::call(&SQLFetchScroll, stmt, SQL_FETCH_NEXT, 0), SQL_NO_DATA);
    BOOST_CHECK_EQUAL(fetched, 1);
    BOOST_CHECK_EQUAL(id, 1);

    auto type = odbcx::details::cast<diversion::optional<std::string>>(odbcx::SurrogateVector<char, SQL_C_CHAR>{ stmt, 2 });
    BOOST_CHECK_EQUAL(!type, false);
    BOOST_CHECK_EQUAL(*type, "type1");
    BOOST_CHECK_THROW((odbcx::SurrogateVector<char, SQL_C_CHAR>{stmt, 2}), std::runtime_error);

    auto target = odbcx::details::cast<diversion::optional<std::string>>(odbcx::SurrogateVector<char, SQL_C_CHAR>{ stmt, 3 });
    switch (driver_type)
    {
        case test::OdbcDriverType::oracle:
            BOOST_CHECK(!target); //https://docs.oracle.com/cd/B28359_01/server.111/b28286/sql_elements005.htm#SQLRF51081
            break;
        default:
            BOOST_CHECK_EQUAL(!target, false);
            BOOST_CHECK(target->empty());
    }
    BOOST_CHECK_THROW((odbcx::SurrogateVector<char, SQL_C_CHAR>{stmt, 3}), std::runtime_error);

    auto value = odbcx::details::cast<int>(odbcx::Surrogate<int>{ stmt, 4 });
    switch (driver_type)
    {
        case test::OdbcDriverType::psql:
        case test::OdbcDriverType::mysql:
            BOOST_CHECK_EQUAL(odbcx::details::cast<decltype(value)>(odbcx::Surrogate<int>{stmt, 4}), value); //pstgres allows to call it more than once
            break;
        default:
            BOOST_CHECK_THROW((odbcx::Surrogate<int>{stmt, 4}), std::runtime_error);
    }

    auto data = odbcx::details::cast<diversion::optional<std::vector<std::uint8_t>>>(odbcx::SurrogateVector<std::uint8_t, SQL_C_BINARY>{ stmt, 5 });
    BOOST_CHECK(!data);
    switch (driver_type)
    {
        case test::OdbcDriverType::psql:
        case test::OdbcDriverType::mysql:
            BOOST_CHECK((odbcx::SurrogateVector<std::uint8_t, SQL_C_BINARY>{stmt, 5}.is_null())); //pstgres allows to call it more than once
            break;
        default:
            BOOST_CHECK_THROW((odbcx::SurrogateVector<std::uint8_t, SQL_C_BINARY>{stmt, 5}), std::runtime_error);
    }
}

//BOOST_AUTO_TEST_CASE(SurrogateTest)
//{
//    auto stmt = odbcx::handle::allocate<SQL_HANDLE_STMT>(dbc, odbcx::attribute::concur_read_only);
//    odbcx::query(stmt, "select id, type, target, value, data from surrogate_test where id = 1");
//    long id = 0;
//    SQLLEN id_ix;
//    odbcx::call(&SQLBindCol, stmt, 1, SQL_C_LONG, &id, 0, &id_ix);
//
//    odbcx::attribute::set(stmt, SQL_ATTR_ROW_ARRAY_SIZE, 1);
//    SQLULEN fetched = 0;
//    odbcx::attribute::set(stmt, SQL_ATTR_ROWS_FETCHED_PTR, &fetched);
//
//    BOOST_CHECK_NE(odbcx::call(&SQLFetchScroll, stmt, SQL_FETCH_NEXT, 0), SQL_NO_DATA);
//    BOOST_CHECK_EQUAL(fetched, 1);
//    BOOST_CHECK_EQUAL(id, 1);
//
//    std::string type = odbcx::Surrogate<std::vector<char>, SQL_C_CHAR>{ stmt, 2 };
//    BOOST_CHECK_EQUAL(type, "type1");
//    BOOST_CHECK_THROW((odbcx::Surrogate<std::vector<char>, SQL_C_CHAR>{stmt, 2}), std::runtime_error);
//
//    std::vector<char> target = odbcx::Surrogate<std::vector<char>, SQL_C_CHAR>{ stmt, 3 };
//    BOOST_CHECK(target.empty());
//    BOOST_CHECK_THROW((odbcx::Surrogate<std::vector<char>, SQL_C_CHAR>{stmt, 3}), std::runtime_error);
//
//
//    diversion::optional<int> value = odbcx::Surrogate<int, SQL_C_CHAR>{ stmt, 4 };
//    BOOST_CHECK(!value);
//    BOOST_CHECK_THROW((odbcx::Surrogate<std::vector<char>, SQL_C_CHAR>{stmt, 4}), std::runtime_error);
//
//    diversion::optional<std::vector<std::uint8_t>> data = odbcx::Surrogate<std::vector<std::uint8_t>, SQL_C_BINARY>{ stmt, 5 };
//    BOOST_CHECK(!data);
//    BOOST_CHECK_THROW((odbcx::Surrogate<std::vector<char>, SQL_C_BINARY>{stmt, 5}), std::runtime_error);
//
//}


BOOST_AUTO_TEST_SUITE_END()
