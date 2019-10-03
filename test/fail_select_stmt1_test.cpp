#include "odbcx/query.hpp"
#include "dd.hpp"
#include "tests.hpp"

int main()
{
	Fixture fixture;
	auto statement = odbcx::select<data::TestStatic>{}.from("MMDPQ").where("target=?", std::string{"test"}).exec(fixture.dbc);
	return 0;
}
