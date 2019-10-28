// Copyright (c) 2018-2019 Serge Klimov serge.klim@outlook.com

#pragma once
#include "details/statement.hpp"
#include <boost/iterator/iterator_facade.hpp>
#include <boost/range/iterator_range.hpp>
#include <memory>
#include <cassert>

namespace odbcx { inline namespace v0 {

template<typename Sequence, size_t BufferSize = 100>
class Iterator
	: public boost::iterator_facade<Iterator<Sequence, BufferSize>,
	typename std::iterator_traits<typename Recordset<Sequence>::const_iterator>::value_type,
	boost::single_pass_traversal_tag,
	typename std::iterator_traits<typename Recordset<Sequence>::const_iterator>::reference
	>
{
	using Statement = odbcx::Statement<Sequence>;
	using Recordset = odbcx::Recordset<Sequence>;

	struct State : std::enable_shared_from_this<State>
	{
		State(Statement&& s) : stmt{ std::move(s) }, recordset{ stmt.fetch(SQL_FETCH_NEXT, 0, BufferSize) } {}
		Statement stmt;
		Recordset recordset;
		typename Recordset::const_iterator i;
	};
public:
	Iterator() {}
	Iterator(Statement&& stmt) : state_{ make_state(std::move(stmt)) }	{}

	void increment() 
	{
		assert(!state_ == false);
		auto end = cend(state_->recordset);
		assert(state_->i != end);
		if (++state_->i == end)
		{
			state_->recordset = state_->stmt.fetch(SQL_FETCH_NEXT, 0, BufferSize);
			if (!state_->recordset.empty())
				state_->i = cbegin(state_->recordset);
			else
				state_.reset();
		}
	}

	typename std::iterator_traits<typename Recordset::const_iterator>::reference dereference() const 
	{
		assert(!state_ == false);
		return *state_->i;
	}

	bool equal(Iterator const & other) const 
	{
		return state_ == other.state_;
	}
private:
	static std::shared_ptr<State> make_state(Statement&& stmt)
	{
		auto res = std::make_shared<State>(std::move(stmt));
		if (res->recordset.empty())
			return {};
		res->i = cbegin(res->recordset);
		return res;
	}
private:
	std::shared_ptr<State> state_;
};


template<size_t BufferSize, typename Sequence>
boost::iterator_range<Iterator<Sequence, BufferSize>> fetch_range(odbcx::StaticallyBindableStatement<Sequence>&& statement)
{
	return boost::make_iterator_range(Iterator<Sequence, BufferSize>{ std::move(statement) }, Iterator<Sequence, BufferSize>{});
}

template<typename Sequence>
boost::iterator_range<Iterator<Sequence>> fetch_range(odbcx::StaticallyBindableStatement<Sequence>&& statement)
{
	return boost::make_iterator_range(Iterator<Sequence>{ std::move(statement) }, Iterator<Sequence>{});
}


template<size_t BufferSize, typename Sequence>
boost::iterator_range<Iterator<Sequence, BufferSize>> fetch_range(odbcx::DynamicallyBindableStatement<Sequence>&& statement)
{
	return boost::make_iterator_range(Iterator<Sequence, BufferSize>{ std::move(statement) }, Iterator<Sequence, BufferSize>{});
}


template<typename Sequence>
boost::iterator_range<Iterator<Sequence>> fetch_range(odbcx::DynamicallyBindableStatement<Sequence>&& statement)
{
	return boost::make_iterator_range(Iterator<Sequence>{ std::move(statement) }, Iterator<Sequence>{});
}


}/*inline namespace v0*/} //namespace odbcx
