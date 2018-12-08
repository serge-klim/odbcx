#include "odbcx/query.h"
#include "dd.h"
#include "tests.h"

int main()
{
	Fixture fixture;
	auto statement = odbcx::select<data::TestStatic>{}.from("test").where("target=?", std::string{"test"}).exec(fixture.dbc);
	return 0;
}
