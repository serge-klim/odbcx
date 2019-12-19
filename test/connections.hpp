#include <odbcx/handle.hpp>
#include <boost/fusion/include/all.hpp>
#include <boost/fusion/adapted/std_tuple.hpp>
#include <boost/test/data/test_case.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <tuple>
#include <utility>
#include <limits>

namespace test{

enum class OdbcDriverType
{
    unknown, mssql, oracle, psql, mysql
};

using OdbcDriverVersion = std::tuple<unsigned int, unsigned int, unsigned int, unsigned int>;

using Connection = std::tuple<odbcx::handle::Dbc, test::OdbcDriverType, test::OdbcDriverVersion>;
std::vector<Connection> const& active_connection();

struct ApplyFilter
{
    ApplyFilter(Connection const& connection) : connection_(connection) {}
    template<typename Filter>
    bool operator()(Filter const& filter) const noexcept { return filter(connection_); }
private:
    Connection const& connection_;
};

template<typename ...Filter>
class DBConnections
{
public:
	using sample = std::tuple<odbcx::handle::adapter::Dbc, OdbcDriverType, OdbcDriverVersion const&>;
    enum { arity = std::tuple_size<sample>::value };
    using Data = std::vector<sample>;
    DBConnections()
    {
        auto filters = std::tuple<Filter...>{};
        for (auto const& connection : active_connection())
        {
            if(boost::fusion::all(filters, ApplyFilter{ connection }))
            //if (filter(connection))
                connections_.emplace_back(odbcx::handle::adapter::Dbc{ std::get<0>(connection) }, std::get<1>(connection), std::get<2>(connection));
        }
        //auto const& connections = active_connection();
        //std::transform(std::begin(connections), std::end(connections), std::back_inserter(connections_), [](Connection const& connection)
        //    {
        //        return std::make_tuple(odbcx::handle::adapter::Dbc{ std::get<0>(connection) }, std::get<1>(connection), std::get<2>(connection));
        //    });
    }
    boost::unit_test::data::size_t size() const { return connections_.size(); }
    using iterator = Data::const_iterator;
    iterator begin() const { return connections_.cbegin(); }
private:
    Data connections_;
};

using AnyDBConnection = DBConnections<>;

template<OdbcDriverType Type, unsigned int Ver = (std::numeric_limits<unsigned int>::max)()>
struct Except
{
    bool operator()(Connection const& connection) const noexcept
    {
        return std::get<1>(connection) != Type || std::get<0>(std::get<2>(connection)) > Ver;
    }
};

} // namespace test

namespace boost { namespace unit_test { namespace data {

namespace monomorphic {
  template <typename ...Filter>
  struct is_dataset<test::DBConnections<Filter...>> : boost::mpl::true_ {};
}
}}}

BOOST_TEST_DONT_PRINT_LOG_VALUE(std::reference_wrapper<odbcx::handle::Dbc const>)
BOOST_TEST_DONT_PRINT_LOG_VALUE(odbcx::handle::adapter::Dbc)

namespace std{
//inline std::ostream& operator << (std::ostream& out, std::reference_wrapper<odbcx::handle::Dbc const>) { return out; };
std::ostream& operator << (std::ostream& out, test::OdbcDriverType const& type);
std::ostream& operator << (std::ostream& out, test::OdbcDriverVersion const& version);
}


