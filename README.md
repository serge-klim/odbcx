### odbcx

[![Build Status](https://travis-ci.com/serge-klim/odbcx.svg?branch=master)](https://travis-ci.com/serge-klim/odbcx)
[![Build status](https://ci.appveyor.com/api/projects/status/w9rhekv0oacw33kj?svg=true)](https://ci.appveyor.com/project/serge-klim/odbcx)
[![codecov](https://codecov.io/gh/serge-klim/odbcx/branch/master/graph/badge.svg)](https://codecov.io/gh/serge-klim/odbcx)

C++ compile time odbc binding generator.

### Requirements

* C++11 compatible compiler (clang 3.6+, gcc 4.8.5+, msvc-14+).
* Following [boost](http://www.boost.org) libraries:
  - [MP11](http://www.boost.org/doc/libs/1_68_0/libs/mp11/doc/html/mp11.html)
  - [MPL](http://www.boost.org/doc/libs/1_68_0/libs/mpl/doc/index.html)
  - [Fusion](http://www.boost.org/doc/libs/1_68_0/libs/fusion/doc/html/)
  - [Range](https://www.boost.org/doc/libs/1_68_0/libs/range/doc/html/index.html)
  - [Iterator](https://www.boost.org/doc/libs/1_68_0/libs/iterator/doc/index.html)
 

### How it works:


Let's say that there is test table defined like this:

```
	 CREATE TABLE test (
	     ID INTEGER GENERATED by default on null as IDENTITY,
	     TS TIMESTAMP,
	     TARGET VARCHAR2(100),
	     MESSAGETYPE VARCHAR2(100),
	     N INTEGER,
	     DATA BLOB
	 );
```

First of odbc connection has to be established: 
```
    auto env = odbcx::handle::allocate<SQL_HANDLE_ENV>();
    odbcx::handle::set_attribute(env, SQL_ATTR_ODBC_VERSION, SQL_OV_ODBC3_80);
    auto  dbc = odbcx::handle::allocate<SQL_HANDLE_DBC>(env);
    odbcx::connect(dbc, "Driver={PostgreSQL ANSI};server=localhost;database=test;UID=user;PWD=password;");
```

now let's clenup the test table to make sure that the rest of example will work:
```
    odbcx::query(dbc, "delete from test");
```

and make sure there is no records in the test table:
```
    auto n = odbcx::query_one<long>(dbc, "SELECT count(id) FROM test");
    assert(n.get() == 0);
```
`odbcx::query_one<long>` will generate binding for long, prepares statement, execute it and fetch result. It returns `std/boost::optional<long>`. 

let's insert some data into the test table:
```
        auto type1 = std::string{ "type 1" };
        odbcx::query(dbc, "insert into test (target, messagetype, n) values(?,?,?)", "first", type1, 500);
        odbcx::query(dbc, "insert into test (target, messagetype, pb) values(?,?,?)", std::string{ "second" },
						  		  	   				 	type1, std::vector<std::uint8_t>(100 * 1024, 0xF));
        auto empty = std::string{};
        odbcx::query(dbc, "insert into test (target, messagetype, pb) values(?,?,?)",
						  		  	   				 empty, type1, std::vector<std::uint8_t>{});
```
for any arguments on top of connection handle and SQL `odbcx::query` will generate input bindings and execute prepareded query.

```
    auto cursor = odbcx::query<std::tuple<int, SQL_TIMESTAMP_STRUCT, std::string, std::vector<std::uint8_t>>>
	 								(dbc, "SELECT id, ts, target, pb FROM test where messagetype = ?", type1);
    auto range = cursor.fetch();
    for (auto const& rec : range)
    {
        auto const& id = std::get<int>(rec);
        auto const& ts = std::get<SQL_TIMESTAMP_STRUCT>(rec);
        auto const& target = std::get<std::string>(rec);
        auto const& blob = std::get<std::vector<std::uint8_t>>(rec);
    }
```
`cursor.fetch()` will return `std::tuple<int, SQL_TIMESTAMP_STRUCT, std::string, std::vector<std::uint8_t>>` [boost::iterator_range](https://www.boost.org/doc/libs/1_67_0/libs/range/doc/html/range/reference/utilities/iterator_range.html)



Then let's define what we want to query from the table, that's where [Boost.Fusion](https://www.boost.org/doc/libs/1_50_0/libs/fusion/doc/html/fusion/adapted/adapt_struct.html) can be very handy:

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
```

In this case I defined whole table.
Now l'ets query the table:

```
   auto cursor = auto odbcx::select<data::Test>{}.from("test")).exec(dbc);
```

And iterate through result:
```
   auto range = cursor.fetch();
   for (auto const& rec : range)
   {
	   //...
   }

```

query could be more complex than simple select:
```
    auto target = std::string{ "target x" };
    auto cursor1 = odbcx::select<data::Test>{}.from("test").where("target=?", target).exec(dbc);
    auto cursor2 = odbcx::select<data::Test>{}.from("test").as("alias").where("alias.target=?", target).exec(dbc);

```

nullable fields can be wrapped in std/boost::optional