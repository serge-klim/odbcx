#include "odbcx/query.hpp"
#include "tests.hpp"

int main()
{
	Fixture fixture;
	odbcx::query(fixture.dbc, "insert into test (id) values(?)", std::string{"Oops!"});
	return 0;
}
