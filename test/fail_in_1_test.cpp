#include "odbcx/query.h"
#include "tests.h"

int main()
{
	Fixture fixture;
	odbcx::query(fixture.dbc, "insert into test (id, n) values(?,?)", "xxx", 500);
	return 0;
}
