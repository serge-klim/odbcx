#pragma once
#include "odbcx/details/diversion.hpp"
#include "odbcx/odbcx.hpp"
#include <boost/fusion/include/adapt_struct.hpp>
#include <vector>
#include <cstdint>

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

struct TestOptional
{
	diversion::optional<int> id;
	diversion::optional<SQL_TIMESTAMP_STRUCT> ts;
	diversion::optional<std::vector<char>> target;
	diversion::optional<std::string> messagetype;
	diversion::optional<long> n;
	std::vector<std::uint8_t> pb;
};

struct TestNoPB
{
	char target[100];
	std::vector<char> target1;
	char messagetype[100];
	std::string messagetype1;
};

struct TestStatic
{
	char target[100];
	char messagetype[100];
	std::uint8_t pb[1024 * 100];
};

struct TestStaticX
{
	char target[100];
	char messagetype[100];
	std::uint8_t pb[1024 * 100];
	long length;
};

struct TestPBHybrid
{
	char target[100];
	char messagetype[100];
	std::uint8_t pb_s[1024 * 100];
	std::vector<std::uint8_t> pb;
};

struct BlobOnly
{
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

BOOST_FUSION_ADAPT_STRUCT(
	data::TestOptional,
	id,
	ts,
	target,
	messagetype,
	n,
	pb
	)

BOOST_FUSION_ADAPT_STRUCT(
	data::TestNoPB,
	target,
	target1,
	messagetype,
	messagetype1
	)

BOOST_FUSION_ADAPT_STRUCT(
	data::TestStatic,
	target,
	messagetype,
	pb
	)

BOOST_FUSION_ADAPT_STRUCT(
	data::TestStaticX,
	target,
	messagetype,
	pb,
	length
	)

BOOST_FUSION_ADAPT_STRUCT(
	data::TestPBHybrid,
	target,
	messagetype,
	pb_s,
	pb
)

BOOST_FUSION_ADAPT_STRUCT(
	data::BlobOnly,
	pb
)

