#include "odbcx/details/odbc.hpp"
#include "odbcx/bindings/flyweight.hpp"
#include "odbcx/details/diversion.hpp"
#include <boost/range/iterator_range.hpp>
//#include <boost/core/ignore_unused.hpp>
#include <boost/test/unit_test.hpp>
#include <functional>
#include <algorithm>
#include <vector>
#include <string>
#include <cstdint>

template<typename T>
struct TestLayout
{
    SQLLEN indicator;
    T value;
};

BOOST_AUTO_TEST_SUITE(flyweight_test_suite)

BOOST_AUTO_TEST_CASE(ConstNullIntFlyweightTest)
{
    auto layout = TestLayout<int>{};
    layout.indicator = SQL_NULL_DATA;
    auto& x = layout.value;
    auto flyweight = odbcx::details::make_flyweight<int>(static_cast<SQLLEN const*>(&layout.indicator), 
                                                            static_cast<SQLLEN const*>(&layout.indicator + (sizeof(layout) + sizeof(SQLLEN) - 1) / sizeof(SQLLEN) ));
    BOOST_CHECK(!flyweight);
    BOOST_CHECK(flyweight.is_null());
    diversion::optional<int> optional = flyweight;
    BOOST_CHECK(!optional);

    int y = flyweight;
    BOOST_CHECK_EQUAL(y, 0);
}

BOOST_AUTO_TEST_CASE(NullIntFlyweightTest)
{
    auto layout = TestLayout<int>{};
    layout.indicator = SQL_NULL_DATA;
    auto& x = layout.value;
    auto flyweight = odbcx::details::make_flyweight<int>(&layout.indicator,&layout.indicator + sizeof(layout) / sizeof(SQLLEN) );
    BOOST_CHECK(!flyweight);
    BOOST_CHECK(flyweight.is_null());
    diversion::optional<int> optional = flyweight;
    BOOST_CHECK(!optional);

    int y = flyweight;
    BOOST_CHECK_EQUAL(y, 0);
    flyweight = 303;
    BOOST_CHECK_EQUAL(!flyweight, false);
    BOOST_CHECK(!flyweight.is_null());
    BOOST_CHECK_EQUAL(x, 303);

    /*diversion::optional<int>*/ optional = flyweight;
    BOOST_CHECK_EQUAL(!optional, false);
    BOOST_CHECK_EQUAL(optional.get(), 303);

    flyweight = nullptr;
    BOOST_CHECK(!flyweight);
    BOOST_CHECK(flyweight.is_null());

    flyweight = diversion::make_optional(505);
    BOOST_CHECK_EQUAL(!flyweight, false);
    BOOST_CHECK(!flyweight.is_null());
    BOOST_CHECK_EQUAL(x, 505);

    flyweight = diversion::nullopt;
    BOOST_CHECK(!flyweight);
    BOOST_CHECK(flyweight.is_null());
}

BOOST_AUTO_TEST_CASE(IntFlyweightTest)
{
    auto layout = TestLayout<int>{};
    layout.indicator = sizeof(int);
    auto& x = layout.value;
    x = 101;
    auto flyweight = odbcx::details::make_flyweight<int>(&layout.indicator, &layout.indicator + (sizeof(layout) + sizeof(SQLLEN) - 1) / sizeof(SQLLEN));
    BOOST_CHECK_EQUAL(!flyweight,false);
    BOOST_CHECK(!flyweight.is_null());
    BOOST_CHECK_EQUAL(static_cast<int>(flyweight), x);
    diversion::optional<int> optional = flyweight;
    BOOST_CHECK_EQUAL(!optional, false);
    BOOST_CHECK_EQUAL(optional.get(), x);

    int y = flyweight;
    BOOST_CHECK_EQUAL(y, x);
    flyweight = 303;
    BOOST_CHECK_EQUAL(!flyweight, false);
    BOOST_CHECK(!flyweight.is_null());
    BOOST_CHECK_EQUAL(x, 303);
    flyweight = nullptr;
    BOOST_CHECK(!flyweight);
    BOOST_CHECK(flyweight.is_null());
    flyweight = optional;
    BOOST_CHECK_EQUAL(!optional, false);
    BOOST_CHECK_EQUAL(optional.get(), x);

    flyweight = 505;
    BOOST_CHECK_EQUAL(!flyweight, false);
    BOOST_CHECK(!flyweight.is_null());
    BOOST_CHECK_EQUAL(x, 505);
    flyweight = diversion::nullopt;
    BOOST_CHECK(!flyweight);
    BOOST_CHECK(flyweight.is_null());
}

BOOST_AUTO_TEST_CASE(EmptyConstCharArrayFlyweightTest)
{
    auto layout = TestLayout<char[100]>{};
    auto garbage = std::string{ "garbage" };
    auto size = garbage.size();
    std::char_traits<char>::copy(layout.value, garbage.data(), size);
    //    layout.value[size] = '\0';
    layout.indicator = SQL_NULL_DATA;
    auto flyweight = odbcx::details::make_flyweight<char[100]>(static_cast<SQLLEN const*>(&layout.indicator),
        static_cast<SQLLEN const*>(&layout.indicator + (sizeof(layout) + sizeof(SQLLEN) - 1) / sizeof(SQLLEN)));

    BOOST_CHECK(!flyweight);
    std::string str = flyweight;
    BOOST_CHECK(str.empty());
    std::vector<char> vector = flyweight;
    BOOST_CHECK(vector.empty());
    diversion::optional<std::string> optional_str = flyweight;
    BOOST_CHECK(!optional_str);
    diversion::optional<std::vector<char>> optional_vector = flyweight;
    BOOST_CHECK(!optional_vector);

    char out[100];
    odbcx::details::copy2array(out, flyweight);
    BOOST_CHECK(std::all_of(out, out + sizeof(out) / sizeof(out[0]), std::bind(std::equal_to<char>{}, '\0', std::placeholders::_1)));
}

BOOST_AUTO_TEST_CASE(ConstCharArrayFlyweightTest)
{
    auto layout = TestLayout<char[100]>{};
    auto x = std::string{ "test-string" };
    auto size = x.size();
    std::char_traits<char>::copy(layout.value, x.data(), size);
    //    layout.value[size] = '\0';
    layout.indicator = size * sizeof(char);
    auto flyweight = odbcx::details::make_flyweight<char[100]>(static_cast<SQLLEN const*>(&layout.indicator),
            static_cast<SQLLEN const*>(&layout.indicator + (sizeof(layout) + sizeof(SQLLEN) - 1) / sizeof(SQLLEN)));

    BOOST_CHECK_EQUAL(!flyweight, false);
    BOOST_CHECK(!flyweight.is_null());
    BOOST_CHECK_EQUAL(flyweight.size(), size);
    std::string str = flyweight;
    BOOST_CHECK_EQUAL(str, x);
    //diversion::string_view view = flyweight;
    //BOOST_CHECK_EQUAL(view, x);
    std::vector<char> vector = flyweight;
    BOOST_CHECK_EQUAL_COLLECTIONS(begin(vector), end(vector), begin(x), end(x));
    //boost::iterator_range<char const*> range = flyweight;
    //BOOST_CHECK_EQUAL_COLLECTIONS(range.begin(), range.end(), begin(x), end(x));


    diversion::optional<std::string> optional_str = flyweight;
    BOOST_CHECK_EQUAL(!optional_str, false);
    BOOST_CHECK_EQUAL(optional_str.get(), x);
    diversion::optional<std::vector<char>> optional_vector = flyweight;
    BOOST_CHECK_EQUAL(!optional_vector, false);
    BOOST_CHECK_EQUAL_COLLECTIONS(begin(optional_vector.get()), end(optional_vector.get()), begin(x), end(x));

    char out[100];
    odbcx::details::copy2array(out, flyweight);
    BOOST_CHECK_EQUAL_COLLECTIONS(out, out + x.size(), begin(x), end(x));
    BOOST_CHECK(std::all_of(out + x.size(), out + sizeof(out) / sizeof(out[0]) - x.size(), std::bind(std::equal_to<char>{}, '\0', std::placeholders::_1)));
}

BOOST_AUTO_TEST_CASE(CharArrayFlyweightTest)
{
    auto layout = TestLayout<char[100]>{};
    auto x = std::string{ "test-string" };
    auto size = x.size();
    std::char_traits<char>::copy(layout.value, x.data(), size);
    layout.indicator = size * sizeof(char);
    auto flyweight = odbcx::details::make_flyweight<char[100]>(&layout.indicator,&layout.indicator + (sizeof(layout) + sizeof(SQLLEN) - 1 ) / sizeof(SQLLEN));

    BOOST_CHECK_EQUAL(!flyweight, false);
    BOOST_CHECK(!flyweight.is_null());
    BOOST_CHECK_EQUAL(flyweight.size(), size);

    std::string str = flyweight;
    BOOST_CHECK_EQUAL(str, x);
    //diversion::string_view view = flyweight;
    //BOOST_CHECK_EQUAL(view, x);
    std::vector<char> vector = flyweight;
    BOOST_CHECK_EQUAL_COLLECTIONS(begin(vector), end(vector), begin(x), end(x));
    //boost::iterator_range<char const*> range = flyweight;
    //BOOST_CHECK(!range.empty());
    //BOOST_CHECK_EQUAL(range.size(), x.size());
    //BOOST_CHECK_EQUAL_COLLECTIONS(range.begin(), range.end(), begin(x), end(x));

    auto modifyed = std::string{ "modifyed" };
    flyweight = modifyed;
    BOOST_CHECK_EQUAL(flyweight.size(), modifyed.size());
    std::char_traits<char>::compare(modifyed.data(), layout.value, modifyed.size());

    diversion::optional<std::string> ostr = flyweight;
    BOOST_CHECK_EQUAL(!ostr, false);
    BOOST_CHECK_EQUAL((*ostr).size(), modifyed.size());
    BOOST_CHECK_EQUAL(*ostr, modifyed);

    diversion::optional<std::vector<char>> ovec = flyweight;
    BOOST_CHECK_EQUAL(!ovec, false);
    BOOST_CHECK_EQUAL((*ovec).size(), modifyed.size());
    BOOST_CHECK_EQUAL(std::char_traits<char>::compare((*ovec).data(), modifyed.data(), (*ovec).size()), 0);

    //diversion::optional <boost::iterator_range<char const*>> orange = flyweight;
    //BOOST_CHECK_EQUAL(!orange, false);
    //BOOST_CHECK(!(*orange).empty());
    //BOOST_CHECK_EQUAL((*orange).size(), modifyed.size());
    //BOOST_CHECK_EQUAL_COLLECTIONS((*orange).begin(), (*orange).end(), begin(modifyed), end(modifyed));

    //auto modifyed = diversion::optional<std::string>{ "modifyed" };
    flyweight = diversion::optional<std::string>{};
    BOOST_CHECK(flyweight.is_null());
    BOOST_CHECK(!flyweight);
    BOOST_CHECK_EQUAL(flyweight.size(), 0);

    flyweight = diversion::optional<std::string>{ "modifyed" };
    BOOST_CHECK_EQUAL(flyweight.size(), modifyed.size());
    std::char_traits<char>::compare(modifyed.data(), layout.value, modifyed.size());
    
    auto modifyed1 = std::vector<char>{ 'm','o','d','i','f','y','e','d','-','1' };
    flyweight = modifyed1;
    BOOST_CHECK_EQUAL(!flyweight, false);
    BOOST_CHECK(!flyweight.is_null());
    BOOST_CHECK_EQUAL(flyweight.size(), modifyed1.size());
    BOOST_CHECK_EQUAL(std::char_traits<char>::compare(modifyed1.data(), layout.value, modifyed1.size()), 0);

    //auto modifyed2 = std::vector<char>( 200, 'j' );
    //flyweight = modifyed2;
    //BOOST_CHECK_EQUAL(!flyweight, false);
    //BOOST_CHECK(!flyweight.is_null());
    //BOOST_CHECK_EQUAL(flyweight.size(), flyweight.capacity());
    //BOOST_CHECK_EQUAL(std::char_traits<char>::compare(modifyed2.data(), layout.value, flyweight.size()), 0);

    flyweight = diversion::optional<std::vector<char>>{};
    BOOST_CHECK(flyweight.is_null());
    BOOST_CHECK(!flyweight);
    BOOST_CHECK_EQUAL(flyweight.size(), 0);

    //flyweight = boost::make_iterator_range(begin(modifyed), end(modifyed));
    //BOOST_CHECK_EQUAL(flyweight.size(), modifyed.size());
    //std::char_traits<char>::compare(modifyed.data(), layout.value, modifyed.size());

    //flyweight = boost::iterator_range<char const*>{};
    //BOOST_CHECK(!flyweight.is_null());
    //BOOST_CHECK_EQUAL(flyweight.size(), 0);

    //flyweight = diversion::optional <boost::iterator_range<char const*>>{};
    //BOOST_CHECK(flyweight.is_null());
    //BOOST_CHECK_EQUAL(flyweight.size(), 0);

    //auto modifyed3 = diversion::string_view{ "modifyed-3" };
    //flyweight = modifyed3;
    //BOOST_CHECK_EQUAL(!flyweight, false);
    //BOOST_CHECK(!flyweight.is_null());
    //BOOST_CHECK_EQUAL(flyweight.size(), modifyed3.size());
    //BOOST_CHECK_EQUAL(std::char_traits<char>::compare(modifyed3.data(), layout.value, modifyed3.size()), 0);

    
    flyweight = diversion::nullopt;
    BOOST_CHECK(!flyweight);
    BOOST_CHECK(flyweight.is_null());

    flyweight = nullptr;
    BOOST_CHECK(flyweight.is_null());
    BOOST_CHECK(!flyweight);
    BOOST_CHECK_EQUAL(flyweight.size(), 0);
}

BOOST_AUTO_TEST_CASE(ConstCharVarArrayFlyweightTest)
{
    auto layout = TestLayout<char[100]>{};
    auto x = std::string{ "test-string" };
    auto size = x.size();
    std::char_traits<char>::copy(layout.value, x.data(), size);
    //    layout.value[size] = '\0';
    layout.indicator = size * sizeof(char);
    auto flyweight = odbcx::details::make_flyweight_array<char>(static_cast<SQLLEN const*>(&layout.indicator),
        static_cast<SQLLEN const*>(&layout.indicator + (sizeof(layout) + sizeof(SQLLEN) - 1) / sizeof(SQLLEN)));

    BOOST_CHECK_EQUAL(!flyweight, false);
    BOOST_CHECK(!flyweight.is_null());
    BOOST_CHECK_EQUAL(flyweight.size(), size);
    std::string str = flyweight;
    BOOST_CHECK_EQUAL(str, x);
    //diversion::string_view view = flyweight;
    //BOOST_CHECK_EQUAL(view, x);
    std::vector<char> vector = flyweight;
    BOOST_CHECK_EQUAL_COLLECTIONS(begin(vector), end(vector), begin(x), end(x));
    //boost::iterator_range<char const*> range = flyweight;
    //BOOST_CHECK_EQUAL_COLLECTIONS(range.begin(), range.end(), begin(x), end(x));
}

BOOST_AUTO_TEST_CASE(CharVarArrayFlyweightTest)
{
    auto layout = TestLayout<char[100]>{};
    auto x = std::string{ "test-string" };
    auto size = x.size();
    std::char_traits<char>::copy(layout.value, x.data(), size);
    layout.indicator = size * sizeof(char);
    auto flyweight = odbcx::details::make_flyweight_array<char>(&layout.indicator, &layout.indicator + (sizeof(layout) + sizeof(SQLLEN) - 1) / sizeof(SQLLEN));

    BOOST_CHECK_EQUAL(!flyweight, false);
    BOOST_CHECK(!flyweight.is_null());
    BOOST_CHECK_EQUAL(flyweight.size(), size);

    std::string str = flyweight;
    BOOST_CHECK_EQUAL(str, x);
    //diversion::string_view view = flyweight;
    //BOOST_CHECK_EQUAL(view, x);
    std::vector<char> vector = flyweight;
    BOOST_CHECK_EQUAL_COLLECTIONS(begin(vector), end(vector), begin(x), end(x));
    //boost::iterator_range<char const*> range = flyweight;
    //BOOST_CHECK(!range.empty());
    //BOOST_CHECK_EQUAL(range.size(), x.size());
    //BOOST_CHECK_EQUAL_COLLECTIONS(range.begin(), range.end(), begin(x), end(x));

    auto modifyed = std::string{ "modifyed" };
    flyweight = modifyed;
    BOOST_CHECK_EQUAL(flyweight.size(), modifyed.size());
    std::char_traits<char>::compare(modifyed.data(), layout.value, modifyed.size());

    diversion::optional<std::string> ostr = flyweight;
    BOOST_CHECK_EQUAL(!ostr, false);
    BOOST_CHECK_EQUAL((*ostr).size(), modifyed.size());
    BOOST_CHECK_EQUAL(*ostr, modifyed);

    diversion::optional<std::vector<char>> ovec = flyweight;
    BOOST_CHECK_EQUAL(!ovec, false);
    BOOST_CHECK_EQUAL((*ovec).size(), modifyed.size());
    BOOST_CHECK_EQUAL(std::char_traits<char>::compare((*ovec).data(), modifyed.data(), (*ovec).size()), 0);

    //diversion::optional <boost::iterator_range<char const*>> orange = flyweight;
    //BOOST_CHECK_EQUAL(!orange, false);
    //BOOST_CHECK(!(*orange).empty());
    //BOOST_CHECK_EQUAL((*orange).size(), modifyed.size());
    //BOOST_CHECK_EQUAL_COLLECTIONS((*orange).begin(), (*orange).end(), begin(modifyed), end(modifyed));

    //auto modifyed = diversion::optional<std::string>{ "modifyed" };
    flyweight = diversion::optional<std::string>{};
    BOOST_CHECK(flyweight.is_null());
    BOOST_CHECK(!flyweight);
    BOOST_CHECK_EQUAL(flyweight.size(), 0);

    flyweight = diversion::optional<std::string>{ "modifyed" };
    BOOST_CHECK_EQUAL(flyweight.size(), modifyed.size());
    std::char_traits<char>::compare(modifyed.data(), layout.value, modifyed.size());

    auto modifyed1 = std::vector<char>{ 'm','o','d','i','f','y','e','d','-','1' };
    flyweight = modifyed1;
    BOOST_CHECK_EQUAL(!flyweight, false);
    BOOST_CHECK(!flyweight.is_null());
    BOOST_CHECK_EQUAL(flyweight.size(), modifyed1.size());
    BOOST_CHECK_EQUAL(std::char_traits<char>::compare(modifyed1.data(), layout.value, modifyed1.size()), 0);

    //auto modifyed2 = std::vector<char>( 200, 'j' );
    //flyweight = modifyed2;
    //BOOST_CHECK_EQUAL(!flyweight, false);
    //BOOST_CHECK(!flyweight.is_null());
    //BOOST_CHECK_EQUAL(flyweight.size(), flyweight.capacity());
    //BOOST_CHECK_EQUAL(std::char_traits<char>::compare(modifyed2.data(), layout.value, flyweight.size()), 0);

    flyweight = diversion::optional<std::vector<char>>{};
    BOOST_CHECK(flyweight.is_null());
    BOOST_CHECK(!flyweight);
    BOOST_CHECK_EQUAL(flyweight.size(), 0);

    //flyweight = boost::make_iterator_range(begin(modifyed), end(modifyed));
    //BOOST_CHECK_EQUAL(flyweight.size(), modifyed.size());
    //std::char_traits<char>::compare(modifyed.data(), layout.value, modifyed.size());

    //flyweight = boost::iterator_range<char const*>{};
    //BOOST_CHECK(!flyweight.is_null());
    //BOOST_CHECK_EQUAL(flyweight.size(), 0);

    //flyweight = diversion::optional <boost::iterator_range<char const*>>{};
    //BOOST_CHECK(flyweight.is_null());
    //BOOST_CHECK_EQUAL(flyweight.size(), 0);

    //auto modifyed3 = diversion::string_view{ "modifyed-3" };
    //flyweight = modifyed3;
    //BOOST_CHECK_EQUAL(!flyweight, false);
    //BOOST_CHECK(!flyweight.is_null());
    //BOOST_CHECK_EQUAL(flyweight.size(), modifyed3.size());
    //BOOST_CHECK_EQUAL(std::char_traits<char>::compare(modifyed3.data(), layout.value, modifyed3.size()), 0);


    flyweight = diversion::nullopt;
    BOOST_CHECK(!flyweight);
    BOOST_CHECK(flyweight.is_null());

    flyweight = nullptr;
    BOOST_CHECK(flyweight.is_null());
    BOOST_CHECK(!flyweight);
    BOOST_CHECK_EQUAL(flyweight.size(), 0);
}

BOOST_AUTO_TEST_SUITE_END()