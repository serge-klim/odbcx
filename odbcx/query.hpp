// Copyright (c) 2018-2019 Serge Klimov serge.klim@outlook.com

#pragma once
#include "details/query.hpp"
#include <boost/fusion/include/std_tuple.hpp>
#include <boost/fusion/include/accumulate.hpp>
#include <boost/fusion/include/at_c.hpp>
#include <boost/fusion/include/size.hpp>
#include <boost/fusion/include/tag_of_fwd.hpp>
#include <boost/fusion/include/is_sequence.hpp>
#include <boost/fusion/include/mpl.hpp>
#include <boost/mpl/range_c.hpp>
#include "odbcx/bindings/in.hpp"
#include "odbcx/bindings/out.hpp"
#include "odbcx/details/statement.hpp"
#include "odbcx/utility.hpp"
#include <boost/utility/string_view.hpp>
#include <boost/optional.hpp>
#include <algorithm>
#include <utility>
#include <tuple>
#include <string>
#include <cassert>

namespace odbcx { inline namespace v0 {

inline void query(handle::Stmt const& stmt, std::string const& text)
{
	call(&SQLPrepare, stmt, const_cast<SQLCHAR*>(reinterpret_cast<SQLCHAR const*>(text.data())), SQLINTEGER(text.size()));
	call(&SQLExecute, stmt);
}

template<typename ...Params>
void query(handle::Stmt const& stmt, std::string const& text, Params&& ...params)
{
	assert(std::count(cbegin(text), cend(text), '?') == sizeof...(params) && "hmm! seems parameters number mismatches placeholders!");
	SQLLEN lengths[sizeof...(params)];
	details::in::InputParameters{}.bind(lengths, stmt, std::forward<Params>(params)...);
	query(stmt, text);
}

template<typename ...Params>
handle::Stmt query(handle::Dbc const& dbc, std::string const& text, Params&& ...params)
{
	auto stmt = details::allocate_statement_handle(dbc);
	query(stmt, text, std::forward<Params>(params)...);
	return stmt;
}

template<typename ...Out>
Statement<typename details::MakeSequence<Out...>::type> query(handle::Stmt&& stmt, std::string const& text)
{
	return { std::move(stmt), text };
}

template<typename ...Out, typename ...Params>
auto query(handle::Stmt&& stmt, std::string const& text, Params&& ...params)
    -> typename std::enable_if<sizeof...(Out) != 0, Statement<typename details::MakeSequence<Out...>::type>>::type
{
	assert(std::count(cbegin(text), cend(text), '?') == sizeof...(params) && "hmm! seems parameters number mismatches placeholders!");
	SQLLEN lengths[sizeof...(params)];
	details::in::InputParameters{}.bind(lengths, stmt, std::forward<Params>(params)...);
	return { std::move(stmt), text };
}

template<typename ...Out, typename ...Params>
auto query(handle::Dbc const& dbc, std::string const& text, Params&& ...params)
    -> typename std::enable_if<sizeof...(Out) != 0, Statement<typename details::MakeSequence<Out...>::type>>::type
{
	return query<Out...>(details::allocate_statement_handle(dbc), text,  std::forward<Params>(params)...);
}

template<typename Out/*typename ...Out*/, typename Handle, typename ...Params>
auto query_one(Handle&& handle, std::string const& text, Params&& ...params)
	-> std::enable_if_t<boost::fusion::traits::is_sequence<Out>::value, diversion::optional<Out>>
{
	return query<Out>(std::move(handle), text, std::forward<Params>(params)...).fetch_one();
}

template<typename Out/*typename ...Out*/, typename Handle, typename ...Params>
auto query_one(Handle&& handle, std::string const& text, Params&& ...params)
	-> std::enable_if_t<!boost::fusion::traits::is_sequence<Out>::value, diversion::optional<Out>>
{
	auto value = query<std::tuple<Out>>(std::move(handle), text, std::forward<Params>(params)...).fetch_one();
	return !value ? diversion::nullopt : diversion::make_optional(std::get<0>(value.value()));
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

template<typename PrevLayer, typename InputParams = std::tuple<>, SelectClause Clause = SelectClause::From>
class SelectQuery
{
public:
	using ExecResult = typename PrevLayer::ExecResult;
	using Alias = std::pair<std::string::size_type, std::string::size_type>;

	SelectQuery(PrevLayer&& prev_layer, Alias&& alias, std::string tail)
		: prev_layer_{ std::forward<PrevLayer>(prev_layer) }, tail_{ std::move(tail) }, alias_{ std::forward<Alias>(alias) }{}

	SelectQuery(PrevLayer&& prev_layer, Alias&& alias, std::string tail, InputParams&& params)
		: prev_layer_{ std::forward<PrevLayer>(prev_layer) }, tail_{ std::move(tail) }, alias_{ std::forward<Alias>(alias) }, params_{ std::forward<InputParams>(params) }{}

	SelectQuery<PrevLayer, InputParams, SelectClause::As> as(std::string const& alias) &&
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
		return {std::move(prev_layer_), std::move(a), std::move(tail_), std::move(params_) };
	}

	template<typename ...Params>
	auto where(std::string const& condition, Params&& ...params) &&
		-> SelectQuery<PrevLayer, decltype(std::tuple_cat(std::declval<InputParams>(), std::forward_as_tuple(std::forward<Params>(params)...))), SelectClause::Where>
	{
		static_assert(details::in::InputParametersVerify<Params...>::value, "input parameter can't be temporary!");
		static_assert(Clause != SelectClause::Where, "SQL statement can't have more than one where clauses");
		static_assert(Clause != SelectClause::GroupBy, "where can't follow by group by clause");
		static_assert(Clause != SelectClause::Having, "where can't follow by having clause");
		static_assert(Clause != SelectClause::OrderBy, "where can't follow by order by clause");
		static_assert(!(Clause > SelectClause::OrderBy), "Oops! just wrong");
		return make_result<SelectClause::Where>(std::move(prev_layer_), std::move(alias_), std::move(tail_), " WHERE " + condition, std::move(params_), std::forward<Params>(params)...);
	}

	template<typename ...Params>
	auto group_by(std::string const& condition, Params&& ...params) &&
		->SelectQuery<PrevLayer, decltype(std::tuple_cat(std::declval<InputParams>(), std::tuple<Params const&...>{std::forward<Params>(params)...})), SelectClause::GroupBy >
	{
		static_assert(details::in::InputParametersVerify<Params...>::value, "input parameter can't be temporary!");
		static_assert(Clause != SelectClause::GroupBy, "SQL statement can't have more than one group by clauses");
		static_assert(Clause != SelectClause::Having, "group by can't follow by having clause");
		static_assert(Clause != SelectClause::OrderBy, "group by can't follow by order by clause");
		static_assert(!(Clause > SelectClause::OrderBy), "Oops! just wrong");
		return make_result<SelectClause::GroupBy>(std::move(prev_layer_), std::move(alias_), std::move(tail_), " GROUP BY " + condition, std::move(params_), std::forward<Params>(params)...);
	}

	template<typename ...Params>
	auto having(std::string const& condition, Params&& ...params) &&
		->SelectQuery<PrevLayer, decltype(std::tuple_cat(std::declval<InputParams>(), std::tuple<Params const&...>{std::forward<Params>(params)...})), SelectClause::Having >
	{
		static_assert(details::in::InputParametersVerify<Params...>::value, "input parameter can't be temporary!");
		static_assert(Clause != SelectClause::Having, "SQL statement can't have more than one having clauses");
		static_assert(Clause != SelectClause::OrderBy, "having can't follow by order by clause");
		static_assert(!(Clause > SelectClause::OrderBy), "Oops! just wrong");
		return make_result<SelectClause::Having>(std::move(prev_layer_), std::move(alias_), std::move(tail_), " HAVING " + condition, std::move(params_), std::forward<Params>(params)...);
	}

	template<typename ...Params>
	auto order_by(std::string const& condition, Params&& ...params) &&
		->SelectQuery<PrevLayer, decltype(std::tuple_cat(std::declval<InputParams>(), std::forward_as_tuple(std::forward<Params>(params)...))), SelectClause::OrderBy >
	{
		static_assert(details::in::InputParametersVerify<Params...>::value, "input parameter can't be temporary!");
		static_assert(Clause != SelectClause::OrderBy, "SQL statement can't have more than one order by clauses");
		static_assert(!(Clause > SelectClause::OrderBy), "Oops! just wrong");
		return make_result<SelectClause::OrderBy>(std::move(prev_layer_), std::move(alias_), std::move(tail_), " ORDER BY " + condition, std::move(params_) , std::forward<Params>(params)...);
	}

	template<typename DBHandle>
	ExecResult exec(DBHandle&& stmt)
	{
		return exec_(std::forward<DBHandle>(stmt));
	}
private:
	template<SelectClause NextLayer, typename ...Params1, typename ...Params2>
	static auto make_result(PrevLayer&& prev_layer, Alias&& alias, std::string&& tail, std::string&& clause, std::tuple<Params1...>&& params1, Params2&& ...params2)
		->SelectQuery<PrevLayer, decltype(std::tuple_cat(std::forward<std::tuple<Params1...>>(params1), std::forward_as_tuple(std::forward<Params2>(params2)...))), NextLayer>
	{
		static_assert(details::in::InputParametersVerify<Params2...>::value, "input parameter can't be temporary!");
		return { std::forward<PrevLayer>(prev_layer), std::forward<Alias>(alias), std::forward<std::string>(tail) + std::forward<std::string>(clause), 
																	std::tuple_cat(std::forward<std::tuple<Params1...>>(params1), std::forward_as_tuple(std::forward<Params2>(params2)...)) };
	}

	template<typename DBHandle, typename ...Params>
	auto exec_(DBHandle&& stmt, Params&& ...params) -> std::enable_if_t<sizeof...(Params) != std::tuple_size<InputParams>::value, ExecResult>
	{
		return exec_(std::forward<DBHandle>(stmt), std::get<sizeof...(Params)>(params_), std::forward<Params>(params)...);
	}

	template<typename DBHandle, typename ...Params>
	auto exec_(DBHandle&& stmt, Params&& ...params) -> std::enable_if_t<sizeof...(Params) == std::tuple_size<InputParams>::value, ExecResult>
	{
		return prev_layer_.exec(std::forward<DBHandle>(stmt), alias_, tail_, std::forward<Params>(params)...);
	}
private:
	PrevLayer prev_layer_;
	std::string tail_;
	Alias alias_;
	InputParams params_;
};

template<typename OutSequence>
struct Select
{
	template<typename PrevLayer, typename InputParams, SelectClause> friend class SelectQuery;
	static_assert(std::is_same<typename boost::fusion::traits::tag_of<OutSequence>::type, boost::fusion::struct_tag>::value, "fused structure is expected here");
	using ExecResult = Statement<OutSequence>;

	SelectQuery<Select<OutSequence>> from(std::string const& table) &&
	{
		auto from = std::string{ "FROM " };
		auto alias = std::make_pair(from.length(), table.length());
		from += table;
		return { std::move(*this), std::move(alias), std::move(from) };
	}
private:
	template<typename DBHandle, typename ...Params>
	ExecResult exec(DBHandle&& handle, std::pair<std::string::size_type, std::string::size_type> const& alias, std::string const& tail, Params&& ...params)
	{
		auto alias_view = boost::string_view{ tail.data() + alias.first, alias.second };

		auto query_base = std::string{ "SELECT " };
		query_base.append(alias_view.data() , alias_view.length());
		query_base += '.';
		query_base += boost::fusion::extension::struct_member_name<OutSequence, 0>::call();
		using range = boost::mpl::range_c<size_t, 1, boost::fusion::result_of::size<OutSequence>::type::value>;
		auto query = boost::fusion::accumulate(range{}, query_base, details::out::NameGenerator<OutSequence>{std::move(alias_view)}/*[](std::string str, auto i) -> std::string
																											{
																											return str + ',' + boost::fusion::extension::struct_member_name<Sequence, i>::call();
																											}*/);
		query += ' ';
		query += tail;
		return odbcx::query<OutSequence>(std::move(handle), query, std::forward<Params>(params)...);
	}
};


template<typename OutSequence>
using select = Select<OutSequence>;

}/*inline namespace v0*/} //namespace odbcx