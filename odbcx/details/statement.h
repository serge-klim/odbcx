#pragma once
#include "odbcx/details/diversion.h"
#include "odbcx/bindings/out.h"
#include "odbcx/odbcx.h"
#include <boost/fusion/include/at_c.hpp>
#include <boost/fusion/include/accumulate.hpp>
#include <boost/fusion/include/size.hpp>
#include <boost/fusion/include/is_sequence.hpp>
#include <boost/fusion/include/mpl.hpp>
#include <boost/mpl/range_c.hpp>
#include <boost/iterator/counting_iterator.hpp>
#include <boost/iterator/iterator_adaptor.hpp>
#include <boost/iterator/transform_iterator.hpp>
#include <boost/optional.hpp>
#include <utility>
#include <algorithm>
#include <vector>
#include <type_traits>
#include <cassert>

namespace odbcx { inline namespace v0 {

namespace details {

class Statement
{
protected:
	Statement(handle::Stmt&& stmt, std::string const& query) : stmt_{ std::move(stmt) }
	{
		call(&SQLPrepare, stmt_, const_cast<SQLCHAR*>(reinterpret_cast<SQLCHAR const*>(query.data())), SQLINTEGER(query.size()));
		call(&SQLExecute, stmt_);
	}
	Statement(Statement&&) = default;
	Statement& operator=(Statement&&) = default;
	handle::Stmt const& Handle() const { return stmt_; }
	std::size_t fetch2(void* buffer, size_t n, SQLSMALLINT orientation, SQLLEN offset )
	{
		SQLLEN row_offset = SQLLEN(buffer) - details::bindings_offset;
		SQLSetStmtAttr(stmt_, SQL_ATTR_ROW_BIND_OFFSET_PTR, &row_offset);
		SQLSetStmtAttr(stmt_, SQL_ATTR_ROW_ARRAY_SIZE, reinterpret_cast<SQLPOINTER>(n));
		SQLULEN fetched = 0;
		SQLSetStmtAttr(stmt_, SQL_ATTR_ROWS_FETCHED_PTR, &fetched);
//		https://support.oracle.com/knowledge/Oracle%20Database%20Products/1472987_1.html
//		auto statuses = std::vector<SQLUSMALLINT>(n);
//		SQLSetStmtAttr(stmt, SQL_ATTR_ROW_STATUS_PTR, statuses.data(), 0);
		//if (call(&SQLFetchScroll, stmt_, orientation, offset) == SQL_NO_DATA)
		//	fetched = 0;
		//statuses.resize(fetched);
		//assert(std::find_if(begin(statuses), end(statuses), [](auto status)
		//{
		//	return SQL_ROW_SUCCESS != status;
		//}) == end(statuses));
		//return statuses;
		return call(&SQLFetchScroll, stmt_, orientation, offset) == SQL_NO_DATA ? 0 : std::size_t{ fetched };
	}
private:
	handle::Stmt stmt_;
};

} //namespace details


template<typename Sequence> class StaticallyBindableRecordset;

template<typename Sequence>
class StaticallyBindableStatement : details::Statement
{
	static_assert(boost::fusion::traits::is_sequence<Sequence>::value, "fusion sequence expected");
	static_assert(details::out::IsSequenceStaticallyBindable<Sequence>::value, "fusion sequence supposed to be statically bindable");
public:
	using Bindings = details::out::StaticBindings<Sequence>;
	using Recordset = StaticallyBindableRecordset<Sequence>;
	StaticallyBindableStatement(handle::Stmt&& stmt, std::string const& query) : Statement{ std::move(stmt), query }
	{
		Bindings::bind(Handle());
	}
	using details::Statement::Handle;
	Recordset fetch(SQLSMALLINT orientation = SQL_FETCH_NEXT, SQLLEN offset = 0, std::size_t n = 256);
	diversion::optional<Sequence> fetch_one(SQLSMALLINT orientation = SQL_FETCH_NEXT, SQLLEN offset = 0);
};

template<typename Sequence>
class StaticallyBindableRecordset
{
	using Statement = StaticallyBindableStatement<Sequence>;
	using Row = typename Statement::Bindings::Row;
	using Rows = std::vector<Row>;
	struct Value { Sequence const& operator()(Row const& row) const { return row.value; } };
public:
	StaticallyBindableRecordset() = default;
	StaticallyBindableRecordset(StaticallyBindableRecordset const&) = default;
	StaticallyBindableRecordset(StaticallyBindableRecordset&&) = default;
	StaticallyBindableRecordset& operator=(StaticallyBindableRecordset const&) = default;
	StaticallyBindableRecordset& operator=(StaticallyBindableRecordset&&) = default;

	StaticallyBindableRecordset(std::vector<Row>&& rows) : rows_(std::move(rows)) {}

	bool empty() const { return rows_.empty(); }	
	std::size_t size() const { return rows_.size(); }
	Sequence const& operator[](std::size_t index) const { assert(index < size());  return rows_[index].value; }
	using const_iterator = boost::transform_iterator<Value, typename Rows::const_iterator, Sequence const& >;
	const_iterator cbegin() const { return const_iterator{ std::cbegin(rows_), Value{} }; }
	const_iterator cend() const { return const_iterator{ std::cend(rows_), Value{} }; }

	static void bind(handle::Stmt const& stmt)
	{
		bind_<0>(stmt);
		SQLSetStmtAttr(stmt, SQL_ATTR_ROW_BIND_TYPE, reinterpret_cast<SQLPOINTER>(sizeof(Row)), 0);
	}
private:
	template<std::size_t N>
	static auto bind_(handle::Stmt const& stmt) ->typename std::enable_if<boost::fusion::result_of::size<Sequence>::value != N, void>::type
	{
		auto row = reinterpret_cast<Row const*>(details::bindings_offset);
		auto const& value = boost::fusion::at_c<N>(row->value);
		details::out::Bind<typename boost::fusion::result_of::value_at_c<Sequence, N>::type>{}.column(stmt, N + 1, SQLPOINTER(&value), const_cast<SQLLEN*>(row->indicators + N));
		bind_<N + 1>(stmt);
	}

	template<std::size_t N>
	static auto bind_(handle::Stmt const& /*stmt*/) ->typename std::enable_if<boost::fusion::result_of::size<Sequence>::value == N, void>::type {}
private:
	Rows rows_;
};

template<typename Sequence> class DynamicallyBindableRecordset;

template<typename Sequence>
class DynamicallyBindableStatement : details::Statement
{
	static_assert(boost::fusion::traits::is_sequence<Sequence>::value, "fusion sequence expected");
	static_assert(details::out::IsSequenceDynamicallyBindable<Sequence>::value, "fusion sequence expected to be dynamically bindable");
public:
	using Bindings = details::out::DynamicBindings<Sequence>;
	using Recordset = DynamicallyBindableRecordset<Sequence>;
	DynamicallyBindableStatement(handle::Stmt&& stmt, std::string const& query) 
		: Statement{ std::move(stmt), query }, bindings_{ Bindings::bind(Handle()) }
	{
	}
	using details::Statement::Handle;
	Bindings const& bindings() const { return bindings_; }
	Recordset fetch(SQLSMALLINT orientation = SQL_FETCH_NEXT, SQLLEN offset = 0, std::size_t n = 256);
	diversion::optional<Sequence> fetch_one(SQLSMALLINT orientation = SQL_FETCH_NEXT, SQLLEN offset = 0);
private:
	Bindings bindings_;
};


template<typename Sequence>
class DynamicallyBindableRecordset
{
	static_assert(boost::fusion::traits::is_sequence<Sequence>::value, "fusion sequence expected");
	static_assert(details::out::IsSequenceDynamicallyBindable<Sequence>::value, "fusion sequence expected to be dynamically bindable");
	using Statement = DynamicallyBindableStatement<Sequence>;

	class Iterator
		: public boost::iterator_adaptor<Iterator, boost::counting_iterator<size_t>, Sequence, boost::random_access_traversal_tag, Sequence const>
	{
		using Base = boost::iterator_adaptor<Iterator, boost::counting_iterator<size_t>, Sequence, boost::random_access_traversal_tag, Sequence const>;
	public:
		Iterator()
			: Base(boost::counting_iterator<size_t>{0}) {}

		explicit Iterator(DynamicallyBindableRecordset const& recordset, size_t n = 0)
			: Base{ boost::counting_iterator<size_t>{n} }, recorset_{ &recordset } { assert(recorset_->size() >= n); }

		Sequence const dereference() const
		{
			auto ix = *Base::base();
			assert(recorset_!=nullptr);
			assert(recorset_->size() >= ix);
			return recorset_->operator[](ix);
		}
	private:
		DynamicallyBindableRecordset const* recorset_ = nullptr;
	};
public:
	DynamicallyBindableRecordset() = default;
	DynamicallyBindableRecordset(Statement& statement, std::vector<char>&& buffer)
		: statement_(&statement), buffer_(std::move(buffer))	{}
	DynamicallyBindableRecordset(DynamicallyBindableRecordset const&) = default;
	DynamicallyBindableRecordset(DynamicallyBindableRecordset&&) = default;
	DynamicallyBindableRecordset& operator=(DynamicallyBindableRecordset const&) = default;
	DynamicallyBindableRecordset& operator=(DynamicallyBindableRecordset&&) = default;


	bool empty() const { return buffer_.empty(); }
	std::size_t size() const { assert(buffer_.size() % statement_->bindings().row_size() == 0); return buffer_.size() / statement_->bindings().row_size(); }

	using const_iterator = Iterator;
	const_iterator cbegin() const { return const_iterator{*this, 0 }; }
	const_iterator cend() const { return const_iterator{ *this, size() }; }

	template<std::size_t N>
	using ValueType = typename Statement::Bindings::template ValueType<N>;

	template<std::size_t N>
	ValueType<N> get(std::size_t n) const
	{ 
		assert(size() > n && "row number is out of range");
		auto const& bindings = statement_->bindings();
		auto row = buffer_.data() + bindings.row_size() * n;
		return bindings.template get<N>(statement_->Handle(), row);
	}
	Sequence operator[](std::size_t n) const
	{
		assert(size() > n && "row number is out of range");
		auto const& bindings = statement_->bindings();
		auto row = buffer_.data() + bindings.row_size() * n;
		return bindings.value(statement_->Handle(), row);
	}
private:
	Statement* statement_ = nullptr;
	std::vector<char> buffer_;
};

template<typename Sequence>
using Recordset = typename std::conditional<details::out::IsSequenceStaticallyBindable<Sequence>::value, StaticallyBindableRecordset<Sequence>, DynamicallyBindableRecordset<Sequence> >::type;

template<typename Sequence>
using Statement = typename std::conditional<details::out::IsSequenceStaticallyBindable<Sequence>::value, StaticallyBindableStatement<Sequence>, DynamicallyBindableStatement<Sequence> >::type;

template<typename Sequence>
typename StaticallyBindableRecordset<Sequence>::const_iterator cbegin(StaticallyBindableRecordset<Sequence> const& recordset) { return recordset.cbegin(); }

template<typename Sequence>
typename StaticallyBindableRecordset<Sequence>::const_iterator cend(StaticallyBindableRecordset<Sequence> const& recordset) { return recordset.cend(); }

template<typename Sequence>
typename StaticallyBindableRecordset<Sequence>::const_iterator begin(StaticallyBindableRecordset<Sequence> const& recordset) { return recordset.cbegin(); }

template<typename Sequence>
typename StaticallyBindableRecordset<Sequence>::const_iterator end(StaticallyBindableRecordset<Sequence> const& recordset) { return recordset.cend(); }


template<typename Sequence>
typename DynamicallyBindableRecordset<Sequence>::const_iterator cbegin(DynamicallyBindableRecordset<Sequence> const& recordset) { return recordset.cbegin(); }

template<typename Sequence>
typename DynamicallyBindableRecordset<Sequence>::const_iterator cend(DynamicallyBindableRecordset<Sequence> const& recordset) { return recordset.cend(); }

template<typename Sequence>
typename DynamicallyBindableRecordset<Sequence>::const_iterator begin(DynamicallyBindableRecordset<Sequence> const& recordset) { return recordset.cbegin(); }

template<typename Sequence>
typename DynamicallyBindableRecordset<Sequence>::const_iterator end(DynamicallyBindableRecordset<Sequence> const& recordset) { return recordset.cend(); }

}/*inline namespace v0*/} //namespace odbcx

template<typename Sequence>
typename odbcx::v0::StaticallyBindableStatement<Sequence>::Recordset odbcx::v0::StaticallyBindableStatement<Sequence>::fetch(SQLSMALLINT orientation /*= SQL_FETCH_NEXT*/, SQLLEN offset /*= 0*/, std::size_t n /*= 256*/)
{
	auto rows = std::vector<typename Bindings::Row>(n);
	rows.resize(fetch2(rows.data(), n, orientation, offset));
	return { std::move(rows) };
}

template<typename Sequence>
diversion::optional<Sequence> odbcx::v0::StaticallyBindableStatement<Sequence>::fetch_one(SQLSMALLINT orientation /*= SQL_FETCH_NEXT*/, SQLLEN offset /*= 0*/)
{
	auto row = typename Bindings::Row{};
	auto fetched = fetch2(&row, 1, orientation, offset);
	assert(fetched == 0 || fetched == 1);
	return fetched != 0
		? diversion::make_optional(std::move(row.value))
		: diversion::nullopt;
}

template<typename Sequence>
typename odbcx::v0::DynamicallyBindableStatement<Sequence>::Recordset odbcx::v0::DynamicallyBindableStatement<Sequence>::fetch(SQLSMALLINT orientation /*= SQL_FETCH_NEXT*/, SQLLEN offset /*= 0*/, std::size_t n/* = 256*/)
{
	n = bindings().bulk_fetch() ? n : 1;
	auto row_size = bindings().row_size();
	auto buffer = std::vector<char>(row_size * n);
	assert(!buffer.empty());
#pragma message("it is actually valid case, has to be fixed!!!!!!!!!!!!!")
	assert(bindings().bulk_fetch() || n == 1);
	auto fetched = fetch2(buffer.data(), n, orientation, offset);
	buffer.resize(row_size * fetched);
	return { *this, std::move(buffer)};
}

template<typename Sequence>
diversion::optional<Sequence> odbcx::v0::DynamicallyBindableStatement<Sequence>::fetch_one(SQLSMALLINT orientation /*= SQL_FETCH_NEXT*/, SQLLEN offset /*= 0*/)
{
	std::vector<char> buffer(bindings().row_size());
	auto fetched = fetch2(buffer.data(), 1, orientation, offset);
	assert(fetched == 0 || fetched == 1);
	return fetched != 0
			 ? diversion::make_optional(bindings().value(Handle(), buffer.data()))
			 : diversion::nullopt;
}

