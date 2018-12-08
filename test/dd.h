#pragma once
#include "odbcx/odbcx.h"
#include <boost/fusion/include/adapt_struct.hpp>
#include <vector>
#include <cstdint>

namespace data
{

//mssql
//CREATE TABLE test(
//	id INT IDENTITY(1,1),
//	ts [datetime2](7),
//	TARGET [varchar](100),
//	MESSAGETYPE [varchar](100),
//	N INT,
//	PB [image]
//) 
// oracle
//CREATE TABLE MARGINMAN.test (
//	IX INTEGER GENERATED by default on null as IDENTITY,
//	TS TIMESTAMP,
//	TARGET VARCHAR2(100),
//	MESSAGETYPE VARCHAR2(100),
//	N INTEGER,
//	PB BLOB
//);

//select id, ts, target, messagetype, pb, datalength(pb) as pb_size from test

struct Test
{
	int id;
	SQL_TIMESTAMP_STRUCT ts;
	std::vector<char> target;
	std::string messagetype;
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


