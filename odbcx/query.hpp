// Copyright (c) 2018-2019 Serge Klimov serge.klim@outlook.com

#pragma once

#include "odbcx/cursor.hpp"
#include "odbcx/attribute.hpp"
#include "odbcx/details/query.hpp"
#include "odbcx/details/diversion.hpp"
#include <boost/fusion/include/accumulate.hpp>
#include <boost/fusion/include/is_sequence.hpp>
#include <boost/fusion/include/mpl.hpp>
#include <boost/fusion/support/tag_of.hpp>
#include <boost/mpl/range_c.hpp>
#include <utility>
#include <string>
#include <tuple>
#include <type_traits>

namespace odbcx { inline namespace v1 {

template<typename FirstOut, typename ...RestOut, typename ...Atts, typename ...Params>
auto query(handle::Handle<SQL_HANDLE_STMT, attribute::Attributes<Atts...>>&& stmt, std::string const& text, Params&& ...params)
    -> Cursor<typename details::MakeSequence<FirstOut, RestOut...>::type, attribute::Attributes<Atts...>>
{
    query(stmt, text, std::forward<Params>(params)...);
    return { std::move(stmt) };
}

template<typename FirstOut, typename ...RestOut, typename ...Atts, typename ...Params>
auto query(handle::Adapter<SQL_HANDLE_STMT, attribute::Attributes<Atts...>> stmt, std::string const& text, Params&& ...params)
    -> Cursor<typename details::MakeSequence<FirstOut, RestOut...>::type, attribute::Merge<attribute::Attributes<Atts...>,attribute::synthetic::SharedHandle>>
{
    query(stmt, text, std::forward<Params>(params)...);
    return ( std::move(stmt) );
}

template<typename FirstOut, typename ...RestOut, typename Dbc, typename ...Atts, typename ...Params>
auto query(Dbc const& dbc, attribute::Attributes<Atts...> attributes, std::string const& text, Params&& ...params)
    -> typename std::enable_if < handle::IsDbc<Dbc>::value, 
                    decltype(query<FirstOut, RestOut...>(handle::allocate<SQL_HANDLE_STMT>(dbc, attributes), text, std::forward<Params>(params)...))
                >::type
{
    return query<FirstOut, RestOut...>(handle::allocate<SQL_HANDLE_STMT>(dbc, attributes), text, std::forward<Params>(params)...);
}

template<typename FirstOut, typename ...RestOut, typename Dbc, typename ...Params>
auto query(Dbc const& dbc, std::string const& text, Params&& ...params)
    -> typename std::enable_if <handle::IsDbc<Dbc>::value, 
                                 Cursor<typename details::MakeSequence<FirstOut, RestOut...>::type, typename handle::AttachedAttributes<Dbc>::type>
                //decltype(query<FirstOut, RestOut...>(std::declval<handle::Stmt&&>(), text, std::forward<Params>(params)...))
                //decltype(query<FirstOut, RestOut...>(handle::allocate<SQL_HANDLE_STMT>(dbc, attribute::none), text, std::forward<Params>(params)...))
            >::type
{
    return query<FirstOut, RestOut...>(dbc, attribute::none, text, std::forward<Params>(params)...);
}

template<typename ...Out, typename Cursor, typename ...Params>
auto query(Cursor&& cursor, Params&& ...params)
    -> typename std::enable_if < details::IsCursor<Cursor>::value, decltype(query<Out...>(std::forward<Cursor>(cursor).handle(), std::forward<Params>(params)...))>::type
{
    return query<Out...>(std::forward<Cursor>(cursor).handle(), std::forward<Params>(params)...);
}

template<typename Cursor, typename ...Params>
auto query(Cursor&& cursor, Params&& ...params)
    -> typename std::enable_if < details::IsCursor<Cursor>::value, decltype(query(std::forward<Cursor>(cursor).handle(), std::forward<Params>(params)...))>::type
{
    return query(std::forward<Cursor>(cursor).handle(), std::forward<Params>(params)...);
}

template<typename FirstOut, typename ...RestOut, typename Handle, typename ...Params>
auto query_one(Handle&& handle, std::string const& text, Params&& ...params)
     -> typename details::FetchOneRes<typename details::MakeSequence<FirstOut, RestOut...>::type>::type
{
    auto cursor = query<FirstOut, RestOut...>(std::forward<Handle>(handle), text, std::forward<Params>(params)...);
    return cursor.fetch_one();
}

enum class SelectClause
{
    From,
    As,
    Where,
    GroupBy,
    Having,
    OrderBy
};

template<typename OutSequence, typename InputParams = std::tuple<>, SelectClause Clause = SelectClause::From> 
class SelectQuery
{
public:
	using Alias = std::pair<std::string::size_type, std::string::size_type>;

	SelectQuery(Alias&& alias, std::string tail)
		: tail_{ std::move(tail) }, alias_{ std::forward<Alias>(alias) }{}

	SelectQuery(Alias&& alias, std::string tail, InputParams&& params)
		: tail_{ std::move(tail) }, alias_{ std::forward<Alias>(alias) }, params_( std::forward<InputParams>(params) ){}

	SelectQuery<OutSequence, InputParams, SelectClause::As> as(std::string const& alias)&&
	{
		static_assert(Clause != SelectClause::As, "SQL statement can't have more than one table aliases");
		static_assert(Clause != SelectClause::Where, "as can't follow by where clause");
		static_assert(Clause != SelectClause::GroupBy, "as can't follow by group by clause");
		static_assert(Clause != SelectClause::Having, "as can't follow by having clause");
		static_assert(Clause != SelectClause::OrderBy, "as can't follow by order by clause");
		static_assert(!(Clause > SelectClause::OrderBy), "Oops! just wrong");

		tail_ += ' ';
		auto a = std::make_pair(tail_.length(), alias.length());
		tail_ += alias;
		return { std::move(a), std::move(tail_), std::move(params_) };
	}

	template<typename ...Params>
	auto where(std::string const& condition, Params&& ...params) &&
		-> SelectQuery<OutSequence, decltype(std::tuple_cat(std::declval<InputParams>(), std::forward_as_tuple(std::forward<Params>(params)...))), SelectClause::Where>
	{
		static_assert(details::in::InputParametersVerify<Params...>::value, "input parameter can't be temporary!");
		static_assert(Clause != SelectClause::Where, "SQL statement can't have more than one where clauses");
		static_assert(Clause != SelectClause::GroupBy, "where can't follow by group by clause");
		static_assert(Clause != SelectClause::Having, "where can't follow by having clause");
		static_assert(Clause != SelectClause::OrderBy, "where can't follow by order by clause");
		static_assert(!(Clause > SelectClause::OrderBy), "Oops! just wrong");
		return make_result<SelectClause::Where>( std::move(alias_), std::move(tail_), " WHERE " + condition, std::move(params_), std::forward<Params>(params)...);
	}

	template<typename ...Params>
	auto group_by(std::string const& condition, Params&& ...params) &&
		->SelectQuery<OutSequence, decltype(std::tuple_cat(std::declval<InputParams>(), std::tuple<Params const&...>{std::forward<Params>(params)...})), SelectClause::GroupBy >
	{
		static_assert(details::in::InputParametersVerify<Params...>::value, "input parameter can't be temporary!");
		static_assert(Clause != SelectClause::GroupBy, "SQL statement can't have more than one group by clauses");
		static_assert(Clause != SelectClause::Having, "group by can't follow by having clause");
		static_assert(Clause != SelectClause::OrderBy, "group by can't follow by order by clause");
		static_assert(!(Clause > SelectClause::OrderBy), "Oops! just wrong");
		return make_result<SelectClause::GroupBy>( std::move(alias_), std::move(tail_), " GROUP BY " + condition, std::move(params_), std::forward<Params>(params)...);
	}

	template<typename ...Params>
	auto having(std::string const& condition, Params&& ...params) &&
		->SelectQuery<OutSequence, decltype(std::tuple_cat(std::declval<InputParams>(), std::tuple<Params const&...>{std::forward<Params>(params)...})), SelectClause::Having >
	{
		static_assert(details::in::InputParametersVerify<Params...>::value, "input parameter can't be temporary!");
		static_assert(Clause != SelectClause::Having, "SQL statement can't have more than one having clauses");
		static_assert(Clause != SelectClause::OrderBy, "having can't follow by order by clause");
		static_assert(!(Clause > SelectClause::OrderBy), "Oops! just wrong");
		return make_result<SelectClause::Having>( std::move(alias_), std::move(tail_), " HAVING " + condition, std::move(params_), std::forward<Params>(params)...);
	}

	template<typename ...Params>
	auto order_by(std::string const& condition, Params&& ...params) &&
		->SelectQuery<OutSequence, decltype(std::tuple_cat(std::declval<InputParams>(), std::forward_as_tuple(std::forward<Params>(params)...))), SelectClause::OrderBy >
	{
		static_assert(details::in::InputParametersVerify<Params...>::value, "input parameter can't be temporary!");
		static_assert(Clause != SelectClause::OrderBy, "SQL statement can't have more than one order by clauses");
		static_assert(!(Clause > SelectClause::OrderBy), "Oops! just wrong");
		return make_result<SelectClause::OrderBy>( std::move(alias_), std::move(tail_), " ORDER BY " + condition, std::move(params_), std::forward<Params>(params)...);
	}

	template<typename Handle, typename ...Atts>
	auto/*ExecResult*/ exec(Handle&& handle, attribute::Attributes<Atts...> attributes = attribute::none) const
		-> decltype(odbcx::query<OutSequence>(std::forward<Handle>(handle), attributes, std::declval<std::string const&>(), std::declval<InputParams const&>()))
	{
		auto alias_view = diversion::string_view{ tail_.data() + alias_.first, alias_.second };

		auto query_base = std::string{ "SELECT " };
		query_base.append(alias_view.data(), alias_view.length());
		query_base += '.';
		query_base += boost::fusion::extension::struct_member_name<OutSequence, 0>::call();
		using range = boost::mpl::range_c<size_t, 1, boost::fusion::result_of::size<OutSequence>::type::value>;
		auto query = boost::fusion::accumulate(range{}, query_base, details::columns::NameGenerator<OutSequence>{std::move(alias_view)}/*[](std::string str, auto i) -> std::string
																											{
																											return str + ',' + boost::fusion::extension::struct_member_name<Sequence, i>::call();
																											}*/);
		query += ' ';
		query += tail_;
		return odbcx::query<OutSequence>(std::forward<Handle>(handle), std::move(attributes), query, params_);
	}

private:
	template<SelectClause NextLayer, typename ...Params1, typename ...Params2>
	static auto make_result(Alias&& alias, std::string&& tail, std::string&& clause, std::tuple<Params1...>&& params1, Params2&& ...params2)
		->SelectQuery<OutSequence, decltype(std::tuple_cat(std::forward<std::tuple<Params1...>>(params1), std::forward_as_tuple(std::forward<Params2>(params2)...))), NextLayer>
	{
		static_assert(details::in::InputParametersVerify<Params2...>::value, "input parameter can't be temporary!");
		return { std::forward<Alias>(alias), std::forward<std::string>(tail) + std::forward<std::string>(clause),
																	std::tuple_cat(std::forward<std::tuple<Params1...>>(params1), std::forward_as_tuple(std::forward<Params2>(params2)...)) };
	}
private:
	std::string tail_;
	Alias alias_;
	InputParams params_;
};

template<typename OutSequence>
struct Select
{
	static_assert(std::is_same<typename boost::fusion::traits::tag_of<OutSequence>::type, boost::fusion::struct_tag>::value, "fused structure is expected here");
	//using ExecResult = Statement<OutSequence>;

	SelectQuery<OutSequence> from(std::string const& table)&&
	{
		auto from = std::string{ "FROM " };
		auto alias = std::make_pair(from.length(), table.length());
		from += table;
		return { std::move(alias), std::move(from) };
	}
};

template<typename OutSequence>
using select = Select<OutSequence>;

}/*inline namespace v1*/} /*namespace odbcx*/

