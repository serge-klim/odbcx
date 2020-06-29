#pragma once
#include "odbcx/adapter.hpp"
#include "odbcx/details/odbc.hpp"
#include "odbcx/details/diversion.hpp"
#include <boost/fusion/include/adapt_struct.hpp>
#include <vector>
#include <string>
#include <chrono>
#include <ctime>
#include <cstdint>

namespace data
{

struct BizarreInt
{
    BizarreInt() = default;
    BizarreInt(int value) : is_null{ false }, value{ value }{}
    bool is_null = true;
    int value = 0;
};

struct BizarreDouble
{
    BizarreDouble() = default;
    BizarreDouble(double value) : is_null{ false }, value{ value }{}
    bool is_null = true;
    double value = .0;
};

struct BizarreString
{
    std::string value;
};

template<typename T>
struct BizarreVector
{
    std::vector<T> value;
};

struct BizarreTimepoint
{
    std::time_t time;
};

struct Test
{
    int id;
    std::chrono::system_clock::time_point ts;
    std::string type;
    std::vector<char> target;
    float sum;
    int value;
    std::vector<std::uint8_t> bin_data;
};

struct TestOptional
{
    diversion::optional<int> id;
//    diversion::optional<SQL_TIMESTAMP_STRUCT> ts;
    diversion::optional<std::chrono::system_clock::time_point> ts;
    diversion::optional<std::string> type;
    diversion::optional<std::vector<char>> target;
    diversion::optional<float> sum;
    diversion::optional<int> value;
    diversion::optional<std::vector<std::uint8_t>> bin_data;
};

struct TestEnum
{
    int id;
    std::chrono::system_clock::time_point ts;
    std::string type;
    std::vector<char> target;
    float sum;
    enum {
        zero,
        one,
        two,
        three,
        four
    } value;
    std::vector<std::uint8_t> bin_data;
};

struct TestEnumOptional
{
    diversion::optional<int> id;
    diversion::optional<std::chrono::system_clock::time_point> ts;
    diversion::optional<std::string> type;
    diversion::optional<std::vector<char>> target;
    diversion::optional<float> sum;
    enum class Value{
        zero,
        one,
        two,
        three,
        four
    };
    diversion::optional<Value> value;
    diversion::optional<std::vector<std::uint8_t>> bin_data;
};


struct TestNoData
{
    int id;
    SQL_TIMESTAMP_STRUCT ts;
    std::string type;
    std::vector<char> target;
    int value;
};

struct TestNoDataOptional
{
    diversion::optional<int> id;
    diversion::optional<SQL_TIMESTAMP_STRUCT> ts;
//    diversion::optional<std::chrono::system_clock::time_point> ts;
    diversion::optional<std::string> type;
    diversion::optional<std::vector<char>> target;
    diversion::optional<double> sum;
    diversion::optional<int> value;
};

struct TestStaticNoData
{
    int id;
    SQL_TIMESTAMP_STRUCT ts;
    char type[51];
    char target[256];
    float sum;
    int value;
};

struct TestStaticEnumNoData
{
    int id;
    SQL_TIMESTAMP_STRUCT ts;
    char type[51];
    char target[256];
    float sum;
    enum class Value {
        zero,
        one,
        two,
        three,
        four
    } value;
};

struct TestSimpleStatic
{
    int id;
    SQL_TIMESTAMP_STRUCT ts;
    int value;
};

struct TestNoDataAdaptable
{
    int id;
    std::chrono::system_clock::time_point ts;
    std::string type;
    std::vector<char> target;
    int value;
};

struct TestNotOptimal
{
    std::vector<std::uint8_t> bin_data;
    int id;
    std::chrono::system_clock::time_point ts;
    std::string type;
    std::vector<char> target;
    float sum;
    int value;
};

struct TestOptionalNotOptimal
{
    diversion::optional<std::vector<std::uint8_t>> bin_data;
    diversion::optional<int> id;
    diversion::optional<std::chrono::system_clock::time_point> ts;
    diversion::optional<std::string> type;
    diversion::optional<std::vector<char>> target;
    diversion::optional<float> sum;
    diversion::optional<int> value;
};

struct TestMixedNoData
{
    int id;
    std::chrono::system_clock::time_point ts;
    std::string type;
    char target[256];
    diversion::optional<float> sum;
    int value;
};

struct TestMixedNotOptimal
{
    std::vector<std::uint8_t> bin_data;
    int id;
    std::chrono::system_clock::time_point ts;
    std::string type;
    char target[256];
    diversion::optional<float> sum;
    int value;
};

struct TestAdaptedNoData
{
    diversion::optional<int> id;
    BizarreTimepoint ts;
    BizarreString type;
    BizarreVector<char> target;
    BizarreDouble sum;
    BizarreInt value;
};


} // namespace data

BOOST_FUSION_ADAPT_STRUCT(
    data::Test,
    id,
    ts,
    type,
    target,
    sum,
    value,
    bin_data
)

BOOST_FUSION_ADAPT_STRUCT(
    data::TestOptional,
    id,
    ts,
    type,
    target,
    sum,
    value,
    bin_data
)

BOOST_FUSION_ADAPT_STRUCT(
    data::TestEnum,
    id,
    ts,
    type,
    target,
    sum,
    value,
    bin_data
)

BOOST_FUSION_ADAPT_STRUCT(
    data::TestEnumOptional,
    id,
    ts,
    type,
    target,
    sum,
    value,
    bin_data
)

BOOST_FUSION_ADAPT_STRUCT(
    data::TestNoData,
    id,
    ts,
    type,
    target,
    value
)

BOOST_FUSION_ADAPT_STRUCT(
    data::TestNoDataOptional,
    id,
    ts,
    type,
    target,
    sum,
    value
)

BOOST_FUSION_ADAPT_STRUCT(
	data::TestStaticNoData,
    id,
    ts,
    type,
	target,
    sum,
	value
	)

BOOST_FUSION_ADAPT_STRUCT(
	data::TestStaticEnumNoData,
    id,
    ts,
    type,
	target,
    sum,
	value
	)

BOOST_FUSION_ADAPT_STRUCT(
	data::TestSimpleStatic,
    id,
    ts,
	value
	)

BOOST_FUSION_ADAPT_STRUCT(
	data::TestNoDataAdaptable,
    id,
    ts,
    type,
	target,
	value
	)

BOOST_FUSION_ADAPT_STRUCT(
    data::TestNotOptimal,
    bin_data,
    id,
    ts,
    type,
    target,
    sum,
    value
)

BOOST_FUSION_ADAPT_STRUCT(
    data::TestOptionalNotOptimal,
    bin_data,
    id,
    ts,
    type,
    target,
    sum,
    value
)

BOOST_FUSION_ADAPT_STRUCT(
    data::TestMixedNoData,
    id,
    ts,
    type,
    target,
    sum,
    value
)

BOOST_FUSION_ADAPT_STRUCT(
    data::TestMixedNotOptimal,
    bin_data,
    id,
    ts,
    type,
    target,
    sum,
    value
)

BOOST_FUSION_ADAPT_STRUCT(
    data::TestAdaptedNoData,
    id,
    ts,
    type,
    target,
    sum,
    value
)


namespace odbcx {

template<>
struct Adapter<data::BizarreInt>
{
    using type = diversion::optional<int>;
    data::BizarreInt operator()(type const& value) const noexcept
    {
       return !value ? data::BizarreInt{} : data::BizarreInt{ *value };
    }
    type operator()(data::BizarreInt const& value) const /*noexcept*/
    {
        return value.is_null ? diversion::nullopt : diversion::make_optional(value.value);
    }
};  


template<>
struct Adapter<data::BizarreDouble>
{
    using type = diversion::optional<double>;
    data::BizarreDouble operator()(type const& value) const noexcept
    {
        return !value ? data::BizarreDouble{} : data::BizarreDouble{ *value };
    }
    type operator()(data::BizarreDouble const& value) const /*noexcept*/
    {
        return value.is_null ? diversion::nullopt : diversion::make_optional(value.value);
    }
};

template<>
struct Adapter<data::BizarreString>
{
    using type = std::string;
    data::BizarreString operator()(type const& value) const noexcept
    {
        return {value};
    }
    type operator()(data::BizarreString const& value) const /*noexcept*/
    {
        return value.value;
    }
};  

template<typename T>
struct Adapter<data::BizarreVector<T>>
{
    using type = std::vector<T>;
    data::BizarreVector<T> operator()(type const& value) const noexcept
    {
        return { value };
    }
    type operator()(data::BizarreVector<T> const& value) const /*noexcept*/
    {
        return value.value;
    }
};


template<>
struct Adapter<data::BizarreTimepoint>
{
    using type = std::chrono::system_clock::time_point;
    data::BizarreTimepoint operator()(type const& value) const noexcept
    {
        return { std::chrono::system_clock::to_time_t(value) };
    }
    type operator()(data::BizarreTimepoint const& value) const /*noexcept*/
    {
        return std::chrono::system_clock::from_time_t(value.time);
    }
};


} /*namespace odbcx*/