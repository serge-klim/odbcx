#define BOOST_TEST_MODULE odbcx_handles_test_suite

#include "test.hpp"
#include "odbcx/handle.hpp"
#include "odbcx/attribute.hpp"
#include "odbcx/utility.hpp"
#include <boost/test/unit_test.hpp>
#include <type_traits>


BOOST_AUTO_TEST_SUITE(handle_test_suite)

BOOST_AUTO_TEST_CASE(StaticAttributesTest)
{
    static_assert(std::is_same< decltype(odbcx::attribute::none | odbcx::attribute::concur_rowver), typename std::remove_cv<decltype(odbcx::attribute::concur_rowver)>::type>::value,
        "oops! odbcx::attribute::none | odbcx::attribute::concur_rowver expected to yeld odbcx::attribute::concur_rowver");


    static_assert(std::is_same<decltype(odbcx::attribute::concur_read_only | odbcx::attribute::concur_rowver), typename std::remove_cv<decltype(odbcx::attribute::concur_rowver)>::type>::value,
        "oops! odbcx::attribute::concur_read_only | odbcx::attribute::concur_rowver should colapse to odbcx::attribute::concur_rowver");

    static_assert(!odbcx::attribute::Contains<odbcx::attribute::None, decltype(odbcx::attribute::concur_rowver)>::value, "Oops!");
    static_assert(odbcx::attribute::Contains<decltype(odbcx::attribute::concur_rowver), decltype(odbcx::attribute::concur_rowver)>::value, "Oops!");
    static_assert(odbcx::attribute::Contains<decltype(odbcx::attribute::concur_read_only | odbcx::attribute::concur_rowver), decltype(odbcx::attribute::concur_rowver)>::value, "Oops!");
    static_assert(!odbcx::attribute::Contains<decltype(odbcx::attribute::concur_read_only) , decltype(odbcx::attribute::concur_rowver)>::value, "Oops!");

    static_assert(odbcx::attribute::IsSubset<odbcx::attribute::None, odbcx::attribute::None>::value, "Oops!");
    static_assert(odbcx::attribute::IsSubset<odbcx::attribute::None, decltype(odbcx::attribute::concur_read_only)>::value, "Oops!");
    static_assert(!odbcx::attribute::IsSubset<decltype(odbcx::attribute::concur_read_only), odbcx::attribute::None>::value, "Oops!");
    static_assert(odbcx::attribute::IsSubset<decltype(odbcx::attribute::concur_read_only | odbcx::attribute::concur_rowver), decltype(odbcx::attribute::concur_read_only | odbcx::attribute::concur_rowver)>::value
        , "Oops!");

    static_assert(std::is_same<odbcx::attribute::ValueOf<odbcx::attribute::None, odbcx::attribute::synthetic::Id<odbcx::attribute::synthetic::id::BufferSizeHint>>,
                                    void>::value, "oops! bytes_per_buffer_hint seems not working");

    static_assert(std::is_same<odbcx::attribute::ValueOf<decltype(odbcx::attribute::bytes_per_buffer_hint<137>()), odbcx::attribute::synthetic::Id<odbcx::attribute::synthetic::id::BufferSizeHint>>,
                                    odbcx::attribute::synthetic::value::BufferSizeHintBytes<137>>::value, "oops! bytes_per_buffer_hint seems not working");

    static_assert(std::is_same < odbcx::attribute::Remove<odbcx::attribute::synthetic::SharedHandle, odbcx::attribute::synthetic::Id<odbcx::attribute::synthetic::id::SharedHandle>>
                                                          ,odbcx::attribute::None>::value, "Oops!");

    static_assert(std::is_same < odbcx::attribute::Remove<decltype(odbcx::attribute::nonscrollable | odbcx::attribute::concur_rowver | odbcx::attribute::synthetic::SharedHandle{})
                                                                   , odbcx::attribute::synthetic::Id<odbcx::attribute::synthetic::id::SharedHandle >>,
        typename std::remove_cv<decltype(odbcx::attribute::nonscrollable | odbcx::attribute::concur_rowver)>::type >::value, "Oops!");


    static_assert(std::is_same < odbcx::attribute::Remove<decltype(odbcx::attribute::nonscrollable | odbcx::attribute::concur_rowver), odbcx::handle::AttributeId<SQL_HANDLE_STMT, SQL_CONCUR_READ_ONLY>>,
        typename std::remove_cv<decltype(odbcx::attribute::nonscrollable | odbcx::attribute::concur_rowver)>::type >::value, "Oops!");

    static_assert(std::is_same < odbcx::attribute::Remove<decltype(odbcx::attribute::concur_read_only | odbcx::attribute::concur_rowver), odbcx::handle::AttributeId<SQL_HANDLE_STMT, SQL_CONCUR_READ_ONLY>>,
        typename std::remove_cv<decltype(odbcx::attribute::concur_rowver)>::type >::value , "Oops!");
}

BOOST_DATA_TEST_CASE(AddapterAttributeTest, boost::unit_test::data::make_delayed<test::AnyDBConnection>(), dbc, driver_type, driver_version)
{
    auto env1 = odbcx::handle::allocate<SQL_HANDLE_ENV>();
    auto handle = odbcx::handle::adapt<SQL_HANDLE_ENV>(odbcx::handle::details::sqlhandle(env1), odbcx::attribute::odbc2);
    static_assert(std::is_same<decltype(handle), odbcx::handle::details::adapter::Env<odbcx::attribute::OdbcVersion<SQL_OV_ODBC2>>>::value, "Oops! SQL_OV_ODBC2 is not attached");
    BOOST_CHECK_EQUAL(test::get_attribute(driver_type, env1, SQL_ATTR_ODBC_VERSION), SQL_OV_ODBC2);
    BOOST_CHECK_EQUAL(test::get_attribute(driver_type, handle, SQL_ATTR_ODBC_VERSION), SQL_OV_ODBC2);
}


BOOST_DATA_TEST_CASE(HandleAttributeTest, boost::unit_test::data::make_delayed<test::AnyDBConnection>(), dbc, driver_type, driver_version)
{
    auto env1 = odbcx::handle::allocate<SQL_HANDLE_ENV>(odbcx::attribute::odbc2);
    static_assert(std::is_same<decltype(env1), odbcx::handle::details::Env<odbcx::attribute::OdbcVersion<SQL_OV_ODBC2>>>::value, "Oops! SQL_OV_ODBC2 is not attached");
    BOOST_CHECK_EQUAL(test::get_attribute(driver_type, env1, SQL_ATTR_ODBC_VERSION), SQL_OV_ODBC2);

    using Attributes1 = typename odbcx::handle::AttachedAttributes<decltype(env1)>::type;
    //auto attributes1 = odbcx::handle::attributes(env1);
    //using Attributes1 = Attributes1;
    static_assert(std::is_same<Attributes1, typename std::remove_cv<decltype(odbcx::attribute::odbc2)>::type>::value, "oops! attributes are broken");
    static_assert(odbcx::attribute::Contains<Attributes1, decltype(odbcx::attribute::odbc2)>::value, "oops! attributes are broken");
    static_assert(!odbcx::attribute::Contains<Attributes1, decltype(odbcx::attribute::odbc3)>::value, "oops! attributes are broken");

    auto env2 = odbcx::attribute::set(std::move(env1), odbcx::attribute::odbc3 | odbcx::attribute::cp_one_per_driver);
    using Attributes2 = typename odbcx::handle::AttachedAttributes<decltype(env2)>::type;
//    auto attributes2 = odbcx::handle::attributes(env2);
    static_assert(std::is_same<Attributes2, typename std::remove_cv<decltype(odbcx::attribute::odbc3 | odbcx::attribute::cp_one_per_driver)>::type>::value, "oops! attributes are broken");
    static_assert(odbcx::attribute::Contains<Attributes2, decltype(odbcx::attribute::odbc3)>::value, "oops! attributes are broken");
    static_assert(!odbcx::attribute::Contains<Attributes2, decltype(odbcx::attribute::odbc2)>::value, "oops! attributes are broken");
    static_assert(odbcx::attribute::Contains<Attributes2, decltype(odbcx::attribute::cp_one_per_driver)>::value, "oops! attributes are broken");

    auto env3 = odbcx::attribute::set(std::move(env2), odbcx::attribute::cp_off);
    using Attributes3 = typename odbcx::handle::AttachedAttributes<decltype(env3)>::type;
//    auto attributes3 = odbcx::handle::attributes(env3);
    static_assert(std::is_same<Attributes3, typename std::remove_cv<decltype(odbcx::attribute::odbc3 | odbcx::attribute::cp_off)>::type>::value, "oops! attributes are broken");
    static_assert(!odbcx::attribute::Contains<Attributes3, decltype(odbcx::attribute::cp_one_per_driver)>::value, "oops! attributes are broken");
    static_assert(odbcx::attribute::Contains<Attributes3, decltype(odbcx::attribute::cp_off)>::value, "oops! attributes are broken");

    auto dbc1 = odbcx::handle::allocate<SQL_HANDLE_DBC>(env3, odbcx::attribute::mode_read_only);
    using Attributes4 = typename odbcx::handle::AttachedAttributes<decltype(dbc1)>::type;
    //auto attributes4 = odbcx::handle::attributes(dbc1);
    static_assert(std::is_same<Attributes4, typename std::remove_cv<decltype(odbcx::attribute::odbc3 | odbcx::attribute::cp_off| odbcx::attribute::mode_read_only)>::type>::value, "oops! attributes are broken");
    static_assert(!odbcx::attribute::Contains<Attributes4, decltype(odbcx::attribute::cp_one_per_driver)>::value, "oops! attributes are broken");
    static_assert(odbcx::attribute::Contains<Attributes4, decltype(odbcx::attribute::cp_off)>::value, "oops! attributes are broken");
    static_assert(odbcx::attribute::Contains<Attributes4, decltype(odbcx::attribute::mode_read_only)>::value, "oops! attributes are broken");


    odbcx::handle::Env e1 = std::move(env1);
    BOOST_CHECK(!e1);
    odbcx::handle::Env e2 = std::move(env2);
    BOOST_CHECK(!e2);
    odbcx::handle::Env e3 = std::move(env3);
    BOOST_CHECK(e3);

    odbcx::handle::Dbc d1 = std::move(dbc1);
    BOOST_CHECK(d1);
}

BOOST_AUTO_TEST_CASE(HandleMixedAttributeTest)
{
    auto env1 = odbcx::handle::allocate<SQL_HANDLE_ENV>(odbcx::attribute::odbc2 | odbcx::attribute::bytes_per_buffer_hint<137>());
    using Attributes1 = typename odbcx::handle::AttachedAttributes<decltype(env1)>::type;
    //auto attributes1 = odbcx::handle::attributes(env1);
    static_assert(std::is_same<Attributes1, typename std::remove_cv<decltype(odbcx::attribute::odbc2 | odbcx::attribute::bytes_per_buffer_hint<137>())>::type>::value, "oops! attributes are broken");
    static_assert(odbcx::attribute::Contains<Attributes1, decltype(odbcx::attribute::odbc2)>::value, "oops! attributes are broken");
    static_assert(!odbcx::attribute::Contains<Attributes1, decltype(odbcx::attribute::odbc3)>::value, "oops! attributes are broken");
    static_assert(std::is_same<odbcx::attribute::ValueOf<Attributes1, odbcx::attribute::synthetic::Id<odbcx::attribute::synthetic::id::BufferSizeHint>>,
                odbcx::attribute::synthetic::value::BufferSizeHintBytes<137>>::value, "oops! attributes are broken");

    auto env2 = odbcx::attribute::set(std::move(env1), odbcx::attribute::odbc3 | odbcx::attribute::cp_one_per_driver);
    using Attributes2 = typename odbcx::handle::AttachedAttributes<decltype(env2)>::type;
    //auto attributes2 = odbcx::handle::attributes(env2);
    static_assert(std::is_same<Attributes2, typename std::remove_cv<decltype(odbcx::attribute::odbc3 | odbcx::attribute::bytes_per_buffer_hint<137>() | odbcx::attribute::cp_one_per_driver)>::type>::value, "oops! attributes are broken");
    static_assert(odbcx::attribute::Contains<Attributes2, decltype(odbcx::attribute::odbc3)>::value, "oops! attributes are broken");
    static_assert(!odbcx::attribute::Contains<Attributes2, decltype(odbcx::attribute::odbc2)>::value, "oops! attributes are broken");
    static_assert(odbcx::attribute::Contains<Attributes2, decltype(odbcx::attribute::cp_one_per_driver)>::value, "oops! attributes are broken");

    auto env3 = odbcx::attribute::set(std::move(env2), odbcx::attribute::cp_off);
    using Attributes3 = typename odbcx::handle::AttachedAttributes<decltype(env3)>::type;
    //auto attributes3 = odbcx::handle::attributes(env3);
    static_assert(std::is_same<Attributes3, typename std::remove_cv<decltype(odbcx::attribute::odbc3 | odbcx::attribute::bytes_per_buffer_hint<137>() | odbcx::attribute::cp_off)>::type>::value, "oops! attributes are broken");
    static_assert(!odbcx::attribute::Contains<Attributes3, decltype(odbcx::attribute::cp_one_per_driver)>::value, "oops! attributes are broken");
    static_assert(odbcx::attribute::Contains<Attributes3, decltype(odbcx::attribute::cp_off)>::value, "oops! attributes are broken");


    auto dbc1 = odbcx::handle::allocate<SQL_HANDLE_DBC>(env3, odbcx::attribute::mode_read_only);
    using Attributes4 = typename odbcx::handle::AttachedAttributes<decltype(dbc1)>::type;
    //auto attributes4 = odbcx::handle::attributes(dbc1);
    static_assert(std::is_same<Attributes4, typename std::remove_cv<decltype(odbcx::attribute::odbc3 | odbcx::attribute::bytes_per_buffer_hint<137>() | odbcx::attribute::cp_off | odbcx::attribute::mode_read_only)>::type>::value, "oops! attributes are broken");
    static_assert(!odbcx::attribute::Contains<Attributes4, decltype(odbcx::attribute::cp_one_per_driver)>::value, "oops! attributes are broken");
    static_assert(odbcx::attribute::Contains<Attributes4, decltype(odbcx::attribute::cp_off)>::value, "oops! attributes are broken");
    static_assert(odbcx::attribute::Contains<Attributes4, decltype(odbcx::attribute::mode_read_only)>::value, "oops! attributes are broken");


    odbcx::handle::Env e1 = std::move(env1);
    BOOST_CHECK(!e1);
    odbcx::handle::Env e2 = std::move(env2);
    BOOST_CHECK(!e2);
    odbcx::handle::Env e3 = std::move(env3);
    BOOST_CHECK(e3);


    odbcx::handle::Dbc d1 = std::move(dbc1);
    BOOST_CHECK(d1);
}


BOOST_AUTO_TEST_CASE(InvlalidHandlesCastErrorTest)
{
    auto env = odbcx::handle::cast<SQL_HANDLE_ENV>(SQLHENV(0));
    BOOST_CHECK_THROW(odbcx::handle::allocate<SQL_HANDLE_DBC>(env), std::runtime_error);
    BOOST_CHECK_THROW(odbcx::attribute::set(std::move(env), odbcx::attribute::odbc3), std::runtime_error);

    auto stmt = odbcx::handle::cast<SQL_HANDLE_STMT>(SQLHSTMT(0));
    BOOST_CHECK_THROW(odbcx::attribute::set(std::move(stmt), odbcx::attribute::concur_read_only), std::runtime_error);
    BOOST_CHECK_THROW(odbcx::attribute::set(std::move(stmt), odbcx::attribute::concur_read_only | odbcx::attribute::cursor_forward_only), std::runtime_error);
    BOOST_CHECK_THROW(odbcx::attribute::set(std::move(stmt), odbcx::attribute::concur_read_only | odbcx::attribute::cursor_forward_only | odbcx::attribute::nonscrollable), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(InvlalidHandlesAdaptErrorTest)
{
    auto stmt = odbcx::handle::adapt<SQL_HANDLE_STMT>(SQLHSTMT(0));
    static_assert(odbcx::handle::AttachedType<decltype(stmt)>::value == SQL_HANDLE_STMT, "Oops! odbcx::handle::adapt seems broken!" );
    BOOST_CHECK_THROW(odbcx::handle::adapt<SQL_HANDLE_STMT>(SQLHSTMT(0), odbcx::attribute::concur_read_only), std::runtime_error);
    BOOST_CHECK_THROW(odbcx::handle::adapt<SQL_HANDLE_STMT>(SQLHSTMT(0), odbcx::attribute::concur_read_only | odbcx::attribute::cursor_forward_only), std::runtime_error);
    BOOST_CHECK_THROW(odbcx::handle::adapt<SQL_HANDLE_STMT>(SQLHSTMT(0), odbcx::attribute::concur_read_only | odbcx::attribute::cursor_forward_only | odbcx::attribute::nonscrollable), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(StatementEarlyAllocationTest)
{
    auto env = odbcx::handle::allocate<SQL_HANDLE_ENV>(odbcx::attribute::odbc3);
    using Attributes = typename odbcx::handle::AttachedAttributes<decltype(env)>::type;
    //auto attributes = odbcx::handle::attributes(env);
    static_assert(std::is_same<Attributes, typename std::remove_cv<decltype(odbcx::attribute::odbc3)>::type>::value, "oops! attributes are broken");
    static_assert(odbcx::attribute::Contains<Attributes, decltype(odbcx::attribute::odbc3)>::value, "oops! attributes are broken");
    static_assert(!odbcx::attribute::Contains<Attributes, decltype(odbcx::attribute::odbc2)>::value, "oops! attributes are broken");


    odbcx::call(&SQLSetEnvAttr, env, SQL_ATTR_ODBC_VERSION, SQLPOINTER(SQL_OV_ODBC3), 0);
    auto dbc = odbcx::handle::allocate<SQL_HANDLE_DBC>(env);
    BOOST_CHECK_THROW(odbcx::handle::allocate<SQL_HANDLE_STMT>(dbc, odbcx::attribute::concur_read_only), std::runtime_error/*{"Connection not open"}*/);
    BOOST_CHECK_THROW(odbcx::handle::allocate<SQL_HANDLE_STMT>(dbc, odbcx::attribute::concur_read_only | odbcx::attribute::nonscrollable), std::runtime_error/*{"Connection not open"}*/);
    BOOST_CHECK_THROW(odbcx::handle::allocate<SQL_HANDLE_STMT>(dbc, odbcx::attribute::concur_read_only | odbcx::attribute::cursor_forward_only | odbcx::attribute::nonscrollable), std::runtime_error/*{"Connection not open"}*/);
}

BOOST_AUTO_TEST_CASE(HandleSizeTest)
{
    auto env1 = odbcx::handle::allocate<SQL_HANDLE_ENV>(odbcx::attribute::odbc2 | odbcx::attribute::bytes_per_buffer_hint<137>());
    static_assert(sizeof(env1) == sizeof(std::unique_ptr<int>), "oops!");
    static_assert(sizeof(env1) < sizeof(std::unique_ptr<int, void(*)(int*)>), "oops!");
}

BOOST_AUTO_TEST_SUITE_END()
