#include "test.hpp"
#include <odbcx/attribute.hpp>
#include <odbcx/query.hpp>
#include <odbcx/handle.hpp>
#include <boost/format.hpp>
#include <boost/test/unit_test.hpp>

void test::create_table(odbcx::handle::adapter::Dbc const& dbc, std::string const& table_name)
{
    auto stmt = odbcx::handle::allocate<SQL_HANDLE_STMT>(dbc, odbcx::attribute::concur_read_only);
    odbcx::call(&SQLGetTypeInfo, stmt, SQL_LONGVARBINARY);

    char bin_data_name[0x100];
    SQLLEN bin_data_name_ix;
    odbcx::call(&SQLBindCol, stmt, 1, SQL_C_CHAR, reinterpret_cast<SQLPOINTER>(bin_data_name), SQLLEN(sizeof(bin_data_name)), &bin_data_name_ix);
    BOOST_CHECK_NE(odbcx::call(&SQLFetchScroll, stmt, SQL_FETCH_NEXT, 0), SQL_NO_DATA);

    odbcx::call(&SQLFreeStmt, stmt, SQL_CLOSE);
    odbcx::call(&SQLFreeStmt, stmt, SQL_UNBIND);
    odbcx::call(&SQLFreeStmt, stmt, SQL_RESET_PARAMS);

    auto timestamp = std::string{ "datetime"};
    odbcx::call(&SQLGetTypeInfo, stmt, SQL_TIMESTAMP);
    char ts_name[0x100];
    SQLLEN ts_name_ix;
    odbcx::call(&SQLBindCol, stmt, 1, SQL_C_CHAR, reinterpret_cast<SQLPOINTER>(ts_name), SQLLEN(sizeof(ts_name)), &ts_name_ix);
    if (odbcx::call(&SQLFetchScroll, stmt, SQL_FETCH_NEXT, 0) != SQL_NO_DATA)
        timestamp = ts_name;
    odbcx::call(&SQLFreeStmt, stmt, SQL_CLOSE);
    odbcx::call(&SQLFreeStmt, stmt, SQL_UNBIND);
    odbcx::call(&SQLFreeStmt, stmt, SQL_RESET_PARAMS);

    try { odbcx::query(stmt, str(boost::format("DROP TABLE %1%;") % table_name)); } catch (...) {}
    odbcx::query(stmt, str(boost::format("CREATE TABLE %1%"
                                    "("
                                    "id int,"
                                    "ts %2%,"
                                    "type varchar(50) NOT NULL,"
                                    "target varchar(255),"
                                    "sum float,"
                                    "value int,"
                                    "bin_data %3%"
                                    ")"
    ) % table_name % timestamp % std::string{ bin_data_name, std::size_t(bin_data_name_ix) }));
}

#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable: 4996)
#endif // _WIN32

SQLLEN test::get_attribute(OdbcDriverType driver, odbcx::handle::adapter::Env const& env, SQLINTEGER id)
{
    return SQLLEN(odbcx::handle::get_attribute<SQLUINTEGER>(env, id));
}

SQLLEN test::get_attribute(OdbcDriverType driver, odbcx::handle::adapter::Stmt const& stmt, SQLINTEGER id)
{
    switch (driver)
    {
        case OdbcDriverType::oracle:
            SQLUINTEGER res;
            odbcx::call(&SQLGetStmtOption, stmt, SQLUSMALLINT(id), reinterpret_cast<SQLPOINTER>(&res));
            return res;
        default:
            break;
    }
    return SQLLEN(odbcx::handle::get_attribute<SQLUINTEGER>(stmt, id));
}

#ifdef _WIN32
#pragma warning(pop)
#endif // _WIN32