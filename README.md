### odbcx

[![Build Status](https://travis-ci.com/serge-klim/odbcx.svg?branch=master)](https://travis-ci.com/serge-klim/odbcx)
[![Build status](https://ci.appveyor.com/api/projects/status/w9rhekv0oacw33kj?svg=true)](https://ci.appveyor.com/project/serge-klim/odbcx)
[![codecov](https://codecov.io/gh/serge-klim/odbcx/branch/master/graph/badge.svg)](https://codecov.io/gh/serge-klim/odbcx)

C++ compile time odbc binding generator.

### Requirements

* C++11 compatible compiler (clang 3.6+, gcc 4.8.5+, msvc-14+).
* Following [boost](http://www.boost.org) libraries:
    - [MPL](http://www.boost.org/doc/libs/1_67_0/libs/mpl/doc/index.html)
	- [Fusion](http://www.boost.org/doc/libs/1_67_0/libs/fusion/doc/html/)
	- [Range](https://www.boost.org/doc/libs/1_67_0/libs/range/doc/html/index.html)
	- [Iterator](https://www.boost.org/doc/libs/1_66_0/libs/iterator/doc/index.html)
 

### How it works:

Let's say that there is test table defined like this (PostgreSQL):

```
	CREATE TABLE test (
	  ID SERIAL,
	  TS TIMESTAMP,
	  TARGET VARCHAR(100),
	  MESSAGETYPE VARCHAR(100),
	  N INTEGER,
	  pb BYTEA 
	);
```

To start playing  with this table let's establish odbc connection:
```
	auto env = odbcx::handle::allocate<SQL_HANDLE_ENV>();
	odbcx::SQLSetEnvAttr(env, SQL_ATTR_ODBC_VERSION, SQL_OV_ODBC3_80);
	auto  dbc = odbcx::handle::allocate<SQL_HANDLE_DBC>(env);
	odbcx::connect(dbc, "Driver={PostgreSQL ANSI};server=localhost;database=test;UID=user;PWD=password;");
```

and clenup the test table, to make sure that the rest of example will work as expected:
```
    odbcx::query(dbc, "delete from test");
```

Now make sure there is no records in the test table:
```
    auto n = odbcx::query_one<long>(dbc, "SELECT count(id) FROM test");
	assert(n.get() == 0)
```
`odbcx::query_one<long>` will generate output binding for `long`, prepares statement, execute it and fetch result. It returns `std/boost::optional<long>` (any NULLable value can be wrapped in std/boost::optional<long>`). 

Then let's insert some data into the test table:
```
	auto type1 = std::string{ "type 1" };
	odbcx::query(dbc, "insert into test (target, messagetype, n) values(?,?,?)", "first", type1, 500);
	odbcx::query(dbc, "insert into test (target, messagetype, pb) values(?,?,?)", std::string{ "second" },
													type1, std::vector<std::uint8_t>(100 * 1024, 0xF));
	auto empty = std::string{};
	odbcx::query(dbc, "insert into test (target, messagetype, pb) values(?,?,?)",
												 empty, type1, std::vector<std::uint8_t>{});
```
For any arguments on top of connection handle and SQL `odbcx::query` will generate input bindings and execute prepareded query.
And finally let's fetch just inserted data:
```
	auto range = fetch_range(
			   odbcx::query<int, SQL_TIMESTAMP_STRUCT, std::string, std::vector<std::uint8_t>>
									(dbc, "SELECT id, ts, target, pb FROM test where messagetype = ?", type1));
	for (auto const& rec : range)
	{
		auto const& id = std::get<int>(rec);
		auto const& ts = std::get<SQL_TIMESTAMP_STRUCT>(rec);
		auto const& target = std::get<std::string>(rec);
		auto const& blob = std::get<std::vector<std::uint8_t>>(rec);
	}
```
`odbcx::fetch_range` will return single pass [boost::iterator_range](https://www.boost.org/doc/libs/1_67_0/libs/range/doc/html/range/reference/utilities/iterator_range.html) dereferencable as `std::tuple<int, SQL_TIMESTAMP_STRUCT, std::string, std::vector<std::uint8_t>>`.
odbc relies on [Boost.Fusion](https://www.boost.org/doc/libs/1_50_0/libs/fusion/doc/html/fusion/adapted/adapt_struct.html). So this will work too: 

```
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
	//...
	auto range = odbcx::fetch_range(odbcx::query<Test>(dbc, "SELECT id, ts, target, pb FROM test where messagetype = ?", type1));
```
In fact if there is only one table used in the query, for fusioned structures this will work:  
```
	auto range1 = fetch_range(odbcx::select<data::Test>{}.from("test").where("target=?", type1).exec(dbc));
	auto range2 = fetch_range(odbcx::select<data::Test>{}.from("test").where("target=?", type1).order_by("id ASC").exec(dbc));
	auto range3 = fetch_range(odbcx::select<data::Test>{}.from("test").as("alias").where("alias.target='test'").order_by("id ASC").exec(dbc));
```
Here all ranges are dereferencable as `data::Test`.

complete example: [simple1.cpp](https://github.com/serge-klim/odbcx/blob/master/examples/select.cpp)

