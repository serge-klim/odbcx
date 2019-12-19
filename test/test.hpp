#pragma once
#include "connections.hpp"
#include "odbcx/handle.hpp"
#include "odbcx/utility.hpp"

namespace test{

void create_table(odbcx::handle::adapter::Dbc const& dbc, std::string const& table_name);
SQLLEN get_attribute(OdbcDriverType driver, odbcx::handle::adapter::Env const& env, SQLINTEGER id);
SQLLEN get_attribute(OdbcDriverType driver, odbcx::handle::adapter::Stmt const& stmt, SQLINTEGER id);

} // namespace test