#include "odbcx/query.hpp"
#include "dpq.hpp"
#include "tests.hpp"

int main()
{
	Fixture fixture;
	auto statement = odbcx::select<data::DpqStatic>{}.from("MMDPQ").where("target=?", std::string{"test"}).exec(fixture.dbc);
	return 0;
}
