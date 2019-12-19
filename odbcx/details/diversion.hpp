#pragma once
#include <string>

#ifdef ODBCX_PREFER_BOOST_TYPES__
#define ODBCX_PREFER_BOOST_STRING_VIEW__
#define ODBCX_PREFER_BOOST_OPTIONAL__
#define ODBCX_PREFER_BOOST_VARIANT__
#endif //ODBCX_PREFER_BOOST_TYPES__

#if defined(ODBCX_PREFER_BOOST_STRING_VIEW__) || !defined(__cpp_lib_string_view)
#include <boost/utility/string_view.hpp>
namespace diversion { using boost::string_view; using boost::basic_string_view;}
#else
#include <string_view>
namespace diversion { using std::string_view; using std::basic_string_view;}
#endif
#if defined(ODBCX_PREFER_BOOST_OPTIONAL__) || !defined(__cpp_lib_optional)
#include <boost/optional.hpp>
namespace diversion { using boost::optional; using boost::make_optional; using nullopt_t = boost::none_t; const auto nullopt = boost::none; }
#else
#include<optional>
namespace diversion { using std::optional; using std::make_optional; using std::nullopt_t; using std::nullopt; }
#endif

#if defined(ODBCX_PREFER_BOOST_VARIANT__) || !defined(__cpp_lib_variant)
#include <boost/variant.hpp>
namespace diversion { 
using boost::variant; 
//using boost::apply_visitor; 
template<typename ...Args> auto visit(Args&& ...args) -> decltype(boost::apply_visitor(std::forward<Args>(args)...)) { return boost::apply_visitor(std::forward<Args>(args)...); }
}
#else
#include<variant>
namespace diversion { using std::variant; using std::visit; }
#endif

namespace diversion {

using std::to_string; /*boost::lexical_cast<std::string>*/

}/*diversion*/
