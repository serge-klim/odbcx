#include <boost/test/unit_test.hpp>
#include "odbcx/bindings/ttraits.hpp"

BOOST_AUTO_TEST_SUITE(TypeTTraitsestSuite)


BOOST_AUTO_TEST_CASE(CompaundStatementsTestSuite)
{
	static_assert(odbcx::details::CType<short>::value == SQL_C_SHORT, "wrong SQL type detected");
	static_assert(odbcx::details::CType<unsigned short>::value == SQL_C_USHORT, "wrong SQL type detected");
	static_assert(odbcx::details::CType<std::int32_t>::value == SQL_C_LONG , "wrong SQL type detected");
	static_assert(odbcx::details::CType<long long>::value == SQL_C_SBIGINT, "wrong SQL type detected");
	static_assert(odbcx::details::CType<unsigned long long>::value == SQL_C_UBIGINT, "wrong SQL type detected");
	static_assert(odbcx::details::CType<float>::value == SQL_C_FLOAT, "wrong SQL type detected");
	static_assert(odbcx::details::CType<double>::value == SQL_C_DOUBLE, "wrong SQL type detected");
}


BOOST_AUTO_TEST_SUITE_END()