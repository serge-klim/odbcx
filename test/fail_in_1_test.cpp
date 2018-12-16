#include "odbcx/query.hpp"
#include "tests.hpp"

int main()
{
	Fixture fixture;
	odbcx::query(fixture.dbc, "insert into test (id, n) values(?,?)", "xxx", 500);
	return 0;
}
