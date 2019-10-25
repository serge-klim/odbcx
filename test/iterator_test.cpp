#include <boost/test/unit_test.hpp>
#include "tests.hpp"
#include "dd.hpp"
#include "odbcx/iterator.hpp"
#include "odbcx/query.hpp"
#include <iterator>
#include <string>
#include <vector>
#include <cstdint>


BOOST_FIXTURE_TEST_SUITE(IteratorsTestSuite, Fixture)

BOOST_AUTO_TEST_CASE(SelectStatementStaticIteratorB1DistanceTest)
{
	auto n = odbcx::query_one<long>(dbc, "SELECT count(id) FROM test").value();
	BOOST_CHECK_NE(n, 0);

	auto range = odbcx::fetch_range<1>(odbcx::select<data::TestStatic>{}.from("test").exec(dbc));
	BOOST_CHECK(range.begin() != range.end());
	BOOST_CHECK_EQUAL(std::distance(range.begin(), range.end()), n);
}

BOOST_AUTO_TEST_CASE(SelectStatementStaticIteratorB2DistanceTest)
{
	auto n = odbcx::query_one<long>(dbc, "SELECT count(id) FROM test").value();
	BOOST_CHECK_NE(n, 0);

	auto range = odbcx::fetch_range<2>(odbcx::select<data::TestStatic>{}.from("test").exec(dbc));
	BOOST_CHECK(range.begin() != range.end());
	BOOST_CHECK_EQUAL(std::distance(range.begin(), range.end()), n);
}

BOOST_AUTO_TEST_CASE(SelectStatementStaticIteratorB5DistanceTest)
{
	auto n = odbcx::query_one<long>(dbc, "SELECT count(id) FROM test").value();
	BOOST_CHECK_NE(n, 0);

	auto range = odbcx::fetch_range<5>(odbcx::select<data::TestStatic>{}.from("test").exec(dbc));
	BOOST_CHECK(range.begin() != range.end());
	BOOST_CHECK_EQUAL(std::distance(range.begin(), range.end()), n);
}

BOOST_AUTO_TEST_CASE(SelectStatementStaticIteratorB7DistanceTest)
{
	auto n = odbcx::query_one<long>(dbc, "SELECT count(id) FROM test").value();
	BOOST_CHECK_NE(n, 0);

	auto range = odbcx::fetch_range<7>(odbcx::select<data::TestStatic>{}.from("test").exec(dbc));
	BOOST_CHECK(range.begin() != range.end());
	BOOST_CHECK_EQUAL(std::distance(range.begin(), range.end()), n);
}

BOOST_AUTO_TEST_CASE(SelectStatementStaticIteratorDistanceTest)
{
	auto n = odbcx::query_one<long>(dbc, "SELECT count(id) FROM test").value();
	BOOST_CHECK_NE(n, 0);

	auto range = fetch_range(odbcx::select<data::TestStatic>{}.from("test").exec(dbc));
	BOOST_CHECK(range.begin() != range.end());
	BOOST_CHECK_EQUAL(std::distance(range.begin(), range.end()), n);
}

BOOST_AUTO_TEST_CASE(SelectStatementStaticIteratorEmtyResultDistanceTest)
{
	auto n = odbcx::query_one<long>(dbc, "SELECT count(id) FROM empty").value();
	BOOST_CHECK_EQUAL(n, 0);
	auto range = fetch_range(odbcx::select<data::TestStatic>{}.from("empty").exec(dbc));
	BOOST_CHECK(range.begin() == range.end());
	BOOST_CHECK_EQUAL(std::distance(range.begin(), range.end()), n);
	BOOST_CHECK_EQUAL(std::distance(range.begin(), range.end()), 0);
}


BOOST_AUTO_TEST_CASE(SelectStatementStaticIteratorDerefTest)
{
	auto n = odbcx::query_one<long>(dbc, "SELECT count(id) FROM test").value();
	BOOST_CHECK_NE(n, 0);

	auto range = fetch_range(odbcx::select<data::TestStatic>{}.from("test").exec(dbc));
	BOOST_CHECK(range.begin() != range.end());
	auto data = std::vector<data::TestStatic>{ range.begin(), range.end() };
	BOOST_CHECK_EQUAL(data.size(), n);
	{
		auto range = fetch_range(odbcx::select<data::TestStatic>{}.from("test").exec(dbc));
		BOOST_CHECK(std::equal(range.begin(), range.end(), cbegin(data), cend(data), [](auto const& l, auto const& r)
		{
			return std::string{ l.target } == std::string{ r.target }
				&& std::string{ l.messagetype } == std::string{ r.messagetype }
			&& std::equal(l.pb, l.pb + sizeof(l.pb) / sizeof(l.pb[0]), r.pb, r.pb + sizeof(r.pb) / sizeof(r.pb[0]));
		}));
	}
}

BOOST_AUTO_TEST_CASE(SelectStatementDynamicIteratorB1DistanceTest)
{
	auto n = odbcx::query_one<long>(dbc, "SELECT count(id) FROM test").value();
	BOOST_CHECK_NE(n, 0);

	auto range = odbcx::fetch_range<1>(odbcx::select<data::Test>{}.from("test").exec(dbc));
	BOOST_CHECK(range.begin() != range.end());
	BOOST_CHECK_EQUAL(std::distance(range.begin(), range.end()), n);
}

BOOST_AUTO_TEST_CASE(SelectStatementDynamicIteratorB2DistanceTest)
{
	auto n = odbcx::query_one<long>(dbc, "SELECT count(id) FROM test").value();
	BOOST_CHECK_NE(n, 0);

	auto range = odbcx::fetch_range<2>(odbcx::select<data::Test>{}.from("test").exec(dbc));
	BOOST_CHECK(range.begin() != range.end());
	BOOST_CHECK_EQUAL(std::distance(range.begin(), range.end()), n);
}

BOOST_AUTO_TEST_CASE(SelectStatementDynamicIteratorB5DistanceTest)
{
	auto n = odbcx::query_one<long>(dbc, "SELECT count(id) FROM test").value();
	BOOST_CHECK_NE(n, 0);

	auto range = odbcx::fetch_range<5>(odbcx::select<data::Test>{}.from("test").exec(dbc));
	BOOST_CHECK(range.begin() != range.end());
	BOOST_CHECK_EQUAL(std::distance(range.begin(), range.end()), n);
}

BOOST_AUTO_TEST_CASE(SelectStatementDynamicIteratorB7DistanceTest)
{
	auto n = odbcx::query_one<long>(dbc, "SELECT count(id) FROM test").value();
	BOOST_CHECK_NE(n, 0);

	auto range = odbcx::fetch_range<7>(odbcx::select<data::Test>{}.from("test").exec(dbc));
	BOOST_CHECK(range.begin() != range.end());
	BOOST_CHECK_EQUAL(std::distance(range.begin(), range.end()), n);
}

BOOST_AUTO_TEST_CASE(SelectStatementDynamicIteratorDistanceTest)
{
	auto n = odbcx::query_one<long>(dbc, "SELECT count(id) FROM test").value();
	BOOST_CHECK_NE(n, 0);

	auto range = fetch_range(odbcx::select<data::Test>{}.from("test").exec(dbc));
	BOOST_CHECK(range.begin() != range.end());
	BOOST_CHECK_EQUAL(std::distance(range.begin(), range.end()), n);
}

BOOST_AUTO_TEST_CASE(SelectStatementDynamicIteratorEmtyResultDistanceTest)
{
	auto n = odbcx::query_one<long>(dbc, "SELECT count(id) FROM empty").value();
	BOOST_CHECK_EQUAL(n, 0);
	auto range = fetch_range(odbcx::select<data::Test>{}.from("empty").exec(dbc));
	BOOST_CHECK(range.begin() == range.end());
	BOOST_CHECK_EQUAL(std::distance(range.begin(), range.end()), n);
	BOOST_CHECK_EQUAL(std::distance(range.begin(), range.end()), 0);
}


BOOST_AUTO_TEST_CASE(SelectStatementDynamicIteratorDerefTest)
{
	auto n = odbcx::query_one<long>(dbc, "SELECT count(id) FROM test").value();
	BOOST_CHECK_NE(n, 0);

	auto range = fetch_range(odbcx::select<data::Test>{}.from("test").exec(dbc));
	BOOST_CHECK(range.begin() != range.end());
	auto data = std::vector<data::Test>{ range.begin(), range.end() };
	BOOST_CHECK_EQUAL(data.size(), n);
	{
		auto range = fetch_range(odbcx::select<data::Test>{}.from("test").exec(dbc));
		BOOST_CHECK(std::equal(range.begin(), range.end(), cbegin(data), cend(data), [](auto const& l, auto const& r)
		{
			return std::string{ cbegin(l.target), cend(l.target) } == std::string{ cbegin(r.target), cend(r.target) }
				&& l.messagetype  ==  r.messagetype 
					&& std::equal(cbegin(l.pb), cend(l.pb), cbegin(r.pb), cend(r.pb))
			;
		}));
	}
}

BOOST_AUTO_TEST_CASE(SelectStatementDynamicIteratorDerefOnNotOptimalDataLayoutTest)
{
    auto n = odbcx::query_one<long>(dbc, "SELECT count(id) FROM test").value();
    BOOST_CHECK_NE(n, 0);

    auto range = fetch_range(odbcx::select<data::TestNotOptimalDataLayout>{}.from("test").exec(dbc));
    BOOST_CHECK(range.begin() != range.end());
    auto data = std::vector<data::TestNotOptimalDataLayout>{ range.begin(), range.end() };
    BOOST_CHECK_EQUAL(data.size(), n);
    {
        auto range = fetch_range(odbcx::select<data::Test>{}.from("test").exec(dbc));
        BOOST_CHECK(std::equal(range.begin(), range.end(), cbegin(data), cend(data), [](auto const& l, auto const& r)
            {
                return std::string{ cbegin(l.target), cend(l.target) } == std::string{ r.target }
                    && l.messagetype == r.messagetype
                    && std::equal(cbegin(l.pb), cend(l.pb), cbegin(r.pb), cend(r.pb))
                    ;
            }));
    }
}

BOOST_AUTO_TEST_CASE(SelectStatementDynamicIteratorDerefOnNotOptimalDataLayoutAsTupleTest)
{
    auto n = odbcx::query_one<long>(dbc, "SELECT count(id) FROM test").value();
    BOOST_CHECK_NE(n, 0);
    enum class Enum {};
    using NotOptimalDataLayout = std::tuple<std::string, std::vector<std::uint8_t>, int, Enum, SQL_TIMESTAMP_STRUCT>;
    auto range = odbcx::fetch_range(odbcx::query<NotOptimalDataLayout>(dbc, "SELECT messagetype, pb, id, id, ts FROM test"));
    BOOST_CHECK(range.begin() != range.end());
    auto data = std::vector<NotOptimalDataLayout>{ range.begin(), range.end() };
    BOOST_CHECK_EQUAL(data.size(), n);
    {
        auto range = fetch_range(odbcx::select<data::Test>{}.from("test").exec(dbc));
        BOOST_CHECK(std::equal(range.begin(), range.end(), cbegin(data), cend(data), [](auto const& l, auto const& r)
            {
                return l.id == std::get<2>(r)
                    && l.id == static_cast<int>(std::get<3>(r))
                    && l.messagetype == std::get<0>(r)
                    && std::equal(cbegin(l.pb), cend(l.pb), cbegin(std::get<1>(r)), cend(std::get<1>(r)))
                    ;
            }));
    }
}

BOOST_AUTO_TEST_CASE(SelectStringIteratorDerefTest)
{
	auto n = odbcx::query_one<long>(dbc, "SELECT count(id) FROM test").value();
	BOOST_CHECK_NE(n, 0);

	using Data = std::tuple<std::string>;
	auto range = fetch_range(odbcx::query<Data>(dbc, "select messagetype from test"));
	BOOST_CHECK(range.begin() != range.end());
	auto data = std::vector<Data>{ range.begin(), range.end() };
	BOOST_CHECK_EQUAL(data.size(), n);
	{
		auto range = fetch_range(odbcx::query<Data>(dbc, "select messagetype from test"));
		BOOST_CHECK(std::equal(range.begin(), range.end(), cbegin(data), cend(data), [](auto const& l, auto const& r)
		{
			return std::get<0>(l) == std::get<0>(r);
		}));
	}
}

BOOST_AUTO_TEST_CASE(SelectVectorCharIteratorDerefTest)
{
	auto n = odbcx::query_one<long>(dbc, "SELECT count(id) FROM test").value();
	BOOST_CHECK_NE(n, 0);

	using Data = std::tuple<std::vector<char>>;
	auto range = fetch_range(odbcx::query<Data>(dbc, "select messagetype from test"));
	BOOST_CHECK(range.begin() != range.end());
	auto data = std::vector<Data>{ range.begin(), range.end() };
	BOOST_CHECK_EQUAL(data.size(), n);
	{
		auto range = fetch_range(odbcx::query<Data>(dbc, "select messagetype from test"));
		BOOST_CHECK(std::equal(range.begin(), range.end(), cbegin(data), cend(data), [](auto const& l, auto const& r)
		{
			return std::get<0>(l) == std::get<0>(r);
		}));
	}
}

BOOST_AUTO_TEST_CASE(SelectStatementDynamicNoBlobIteratorDerefTest)
{
	auto n = odbcx::query_one<long>(dbc, "SELECT count(id) FROM test").value();
	BOOST_CHECK_NE(n, 0);

	auto range = fetch_range(odbcx::query<data::TestNoPB>(dbc, "select target, target as target1, messagetype, messagetype as messagetype1 from test")) ;
	BOOST_CHECK(range.begin() != range.end());
	auto data = std::vector<data::TestNoPB>{ range.begin(), range.end() };
	BOOST_CHECK_EQUAL(data.size(), n);
	{
		auto range = fetch_range(odbcx::query<data::TestNoPB>(dbc, "select target, target as target1, messagetype, messagetype as messagetype1 from test"));
		BOOST_CHECK(std::equal(range.begin(), range.end(), cbegin(data), cend(data), [](auto const& l, auto const& r)
		{
			BOOST_CHECK_EQUAL( (std::string{ l.target }), (std::string{ cbegin(l.target1) , cend(l.target1) }));
			BOOST_CHECK_EQUAL((std::string{ r.target }) , (std::string{ cbegin(r.target1) , cend(r.target1) }));
			BOOST_CHECK_EQUAL(std::string{ l.messagetype }, l.messagetype1);
			BOOST_CHECK_EQUAL(std::string{ r.messagetype }, r.messagetype1);

			return std::string{ l.target } == std::string{ r.target }
					&& std::string{ cbegin(l.target1) , cend(l.target1) } == std::string{ cbegin(r.target1) , cend(r.target1) }
							&& std::string{ l.messagetype } == std::string{ r.messagetype }
								&& l.messagetype1 == r.messagetype1;
		}));
	}
}

BOOST_AUTO_TEST_CASE(SelectBlobTest)
{
    auto n = odbcx::query_one<long>(dbc, "SELECT count(id) FROM test").value();
    BOOST_CHECK_NE(n, 0);
    
    auto range = odbcx::fetch_range(odbcx::select<data::BlobOnly>{}.from("test").exec(dbc));
    BOOST_CHECK(range.begin() != range.end());
    BOOST_CHECK_EQUAL(std::distance(range.begin(), range.end()), n);

    {
        auto range = odbcx::fetch_range(odbcx::select<data::BlobOnly>{}.from("test").exec(dbc));
        auto empty = 0;
        auto notempty = 0;
        for (auto const& rec : range)
        {
            if (rec.pb.empty())
                ++empty;
            else
                ++notempty;
        }
        BOOST_CHECK_NE(empty, 0);
        BOOST_CHECK_NE(notempty, 0);
    }
}

BOOST_AUTO_TEST_CASE(Select2BlobsTest)
{
    auto n = odbcx::query_one<long>(dbc, "SELECT count(id) FROM test").value();
    BOOST_CHECK_NE(n, 0);

    auto range = odbcx::fetch_range(odbcx::query<std::tuple<std::vector<std::uint8_t>, std::vector<std::uint8_t>>>(dbc, "SELECT pb, pb FROM test"));
    BOOST_CHECK(range.begin() != range.end());
    BOOST_CHECK_EQUAL(std::distance(range.begin(), range.end()), n);

    {
        auto range = odbcx::fetch_range(odbcx::query<std::tuple<std::vector<std::uint8_t>, std::vector<std::uint8_t>>>(dbc, "SELECT pb, pb FROM test"));
        auto empty = 0;
        auto notempty = 0;
        for (auto const& rec : range)
        {
            BOOST_CHECK_EQUAL(std::get<0>(rec).size(), std::get<1>(rec).size());
            if (std::get<0>(rec).empty())
                ++empty;
            else
                ++notempty;
        }
        BOOST_CHECK_NE(empty, 0);
        BOOST_CHECK_NE(notempty, 0);
    }
}

BOOST_AUTO_TEST_SUITE_END()
