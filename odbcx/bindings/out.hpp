#pragma once
#include "ttraits.hpp"
#include "odbcx/details/diversion.hpp"
#include "odbcx/utility.hpp"
#include <boost/fusion/include/mpl.hpp>
#include <boost/fusion/adapted/struct/detail/extension.hpp>
#include <boost/fusion/include/is_sequence.hpp>
#include "boost/mpl/range_c.hpp"
#include <boost/mpl/void.hpp>
#include <boost/mpl/bool_fwd.hpp>
#include <boost/mpl/begin_end.hpp>  
#include <boost/mpl/find_if.hpp>
#include <boost/mpl/not.hpp>
#include <boost/utility/string_view.hpp>
#include <type_traits>
#include <algorithm>
#include <cstring>
#include <cstdint>
#include <string>



namespace odbcx { inline namespace v0 {

using NoBind = boost::mpl::void_;

namespace details { namespace out {

template<typename T, typename Enabled = boost::mpl::true_> struct Bind;

template<typename T>
constexpr T const* data_cast(SQLLEN const* ptr)
{
	return reinterpret_cast<T const*>(reinterpret_cast<char const*>(ptr + 1) + sizeof(SQLLEN) % alignof(T));
}

template<>
struct Bind<NoBind, boost::mpl::true_>
{
	void column(handle::Stmt const & /*stmt*/, SQLUSMALLINT /*column*/, SQLPOINTER /*data*/, SQLLEN* /*indicator*/)	{}
	std::size_t dyn_column(handle::Stmt const& /*stmt*/, SQLUSMALLINT /*column*/, std::size_t offset) { return offset; }
	using ValueType = NoBind;
	ValueType construct(SQLLEN const* row) const {	assert(*row == 0); return {};}
	ValueType construct(handle::Stmt const& /*stmt*/, SQLUSMALLINT /*column*/) const { return {}; }
	void copy2(SQLLEN const* /*row*/, NoBind& /*out*/) {}
	void read2(handle::Stmt const& stmt, SQLUSMALLINT column, NoBind& out) const { out = construct(stmt, column); }
};

template<typename T>
std::size_t bind_column(handle::Stmt const& stmt, SQLUSMALLINT column, SQLSMALLINT type, std::size_t offset, std::size_t size)
{
	assert(std::size_t(offset) % alignof(SQLLEN) == 0 && "expected to be aligned to SQLLEN(indicator type)");
	auto indicator = reinterpret_cast<SQLLEN*>(offset);
	auto data = const_cast<T*>(data_cast<T>(indicator));
	call(&SQLBindCol, stmt, column, type, reinterpret_cast<char*>(data) + bindings_offset, size, indicator + 1 /*bindings_offset*/);
	return std::size_t(data) + sizeof(char) * size;
}

template<>
struct Bind<SQL_TIMESTAMP_STRUCT, boost::mpl::true_>
{
	void column(handle::Stmt const& stmt, SQLUSMALLINT column, SQLPOINTER data, SQLLEN* indicator)
	{
		call(&SQLBindCol, stmt, column, SQL_C_TYPE_TIMESTAMP, data, sizeof(SQL_TIMESTAMP_STRUCT), indicator);
	}

	std::size_t dyn_column(handle::Stmt const& stmt, SQLUSMALLINT column, std::size_t offset)
	{
		return bind_column<SQL_TIMESTAMP_STRUCT>(stmt, column, SQL_C_TYPE_TIMESTAMP, offset, sizeof(SQL_TIMESTAMP_STRUCT));
	}

	using ValueType = SQL_TIMESTAMP_STRUCT;
	ValueType construct(SQLLEN const* row) const
	{
		assert(*row == SQL_NULL_DATA || *row == sizeof(SQL_TIMESTAMP_STRUCT));
		return *row == SQL_NULL_DATA ? SQL_TIMESTAMP_STRUCT{ 0 } : *data_cast<SQL_TIMESTAMP_STRUCT>(row);
	}
    
	ValueType construct(handle::Stmt const& stmt, SQLUSMALLINT column) const 
    { 
        SQL_TIMESTAMP_STRUCT res{ 0 };
        SQLLEN read = 0;
        if (call(&SQLGetData, stmt, column, SQL_C_TYPE_TIMESTAMP, &res, sizeof(SQL_TIMESTAMP_STRUCT), &read) == SQL_NO_DATA)
            throw std::runtime_error("SQLGetData can be called once only");
        return res;
    }

	void copy2(SQLLEN const* row, SQL_TIMESTAMP_STRUCT& out) const { out = SQL_NULL_DATA == *row ? SQL_TIMESTAMP_STRUCT{0} : *data_cast<SQL_TIMESTAMP_STRUCT>(row); }
	void read2(handle::Stmt const& stmt, SQLUSMALLINT column, SQL_TIMESTAMP_STRUCT& out) const { out = construct(stmt, column); }
};

template<typename T> 
struct Bind<T, typename boost::mpl::or_<std::is_integral<T>, std::is_floating_point<T>>::type>
{
	void column(handle::Stmt const& stmt, SQLUSMALLINT column, SQLPOINTER data, SQLLEN* indicator)
	{		
		call(&SQLBindCol, stmt, column, CType<T>::value, data, CTypeSizeOf<T>::value, indicator);
	}

	std::size_t dyn_column(handle::Stmt const& stmt, SQLUSMALLINT column, std::size_t offset)
	{
		return bind_column<T>(stmt, column, CType<T>::value, offset, CTypeSizeOf<T>::value);
	}

	using ValueType = T;

	ValueType construct(SQLLEN const* row) const
	{
		assert( *row == sizeof(ValueType));
		return SQL_NULL_DATA == *row ? 0 : *data_cast<ValueType>(row);
	}

    ValueType construct(handle::Stmt const& stmt, SQLUSMALLINT column) const
    {
        T res {0};
        SQLLEN read = 0;
        if (call(&SQLGetData, stmt, column, CType<T>::value, &res, CTypeSizeOf<T>::value, &read) == SQL_NO_DATA)
            throw std::runtime_error("SQLGetData can be called once only");
        return res;
    }

	void copy2(SQLLEN const* row, ValueType& out) const {	out = SQL_NULL_DATA == *row ? 0 : *data_cast<ValueType>(row); }
	void read2(handle::Stmt const& stmt, SQLUSMALLINT column, ValueType& out) const { out = construct(stmt, column); }
};

template<typename T>
struct Bind<T, boost::mpl::bool_<std::is_enum<T>::value>> : Bind<typename std::underlying_type<T>::type> 
{
    using UnderlyingType =  typename std::underlying_type<T>::type;
    using ValueType = T;
    ValueType construct(SQLLEN const* row) const
    {
        assert(*row == sizeof(T));
        return static_cast<ValueType>(SQL_NULL_DATA == *row ? 0 : *data_cast<UnderlyingType>(row));
    }
    ValueType construct(handle::Stmt const& stmt, SQLUSMALLINT column) const
    {
        UnderlyingType res{ 0 };
        SQLLEN read = 0;
        if (call(&SQLGetData, stmt, column, CType<UnderlyingType>::value, &res, CTypeSizeOf<UnderlyingType>::value, &read) == SQL_NO_DATA)
            throw std::runtime_error("SQLGetData can be called once only");
        return static_cast<ValueType>(res);
    }

    void copy2(SQLLEN const* row, ValueType& out) const { out = static_cast<ValueType>( SQL_NULL_DATA == *row ? 0 : *data_cast<UnderlyingType>(row)); }
    void read2(handle::Stmt const& stmt, SQLUSMALLINT column, ValueType& out) const { out = construct(stmt, column); }
};


template<std::size_t N>
struct Bind<char[N], boost::mpl::true_>
{
	void column(handle::Stmt const& stmt, SQLUSMALLINT column, SQLPOINTER data, SQLLEN* indicator)
	{
		call(&SQLBindCol, stmt, column, SQL_C_CHAR, data, N, indicator);
	}

	std::size_t dyn_column(handle::Stmt const& stmt, SQLUSMALLINT column, std::size_t offset)
	{
		return bind_column<char>(stmt, column, SQL_C_CHAR, offset, sizeof(char) * N);
	}

	using ValueType = std::string;
	ValueType construct(SQLLEN const* row) const
	{
		if (*row == SQL_NULL_DATA)
			return {};
		auto val = data_cast<char>(row);
		return { val, std::size_t(*row) };
	}

	ValueType construct(handle::Stmt const& stmt, SQLUSMALLINT column) const 
    {
        char out[N];
        SQLLEN read = 0;
        if (call(&SQLGetData, stmt, column, SQL_C_CHAR, out, N * sizeof(char), &read) == SQL_NO_DATA)
            throw std::runtime_error("SQLGetData can be called once only");
        switch (read)
        {
            case SQL_NO_TOTAL:
                throw std::length_error{""};
                break;
            case SQL_NULL_DATA:
                read = 0;
                break;
        }
        assert(N >= std::size_t(read));
        return ValueType{ out, std::size_t(read) };
    }

	void copy2(SQLLEN const* row, char(&out)[N]) const
	{
		auto size = SQL_NULL_DATA == *row ? 0 : std::size_t(*row);
		auto begin = data_cast<char>(row);
		std::copy(begin, begin + size, out);
		std::fill(out + size, out + N, '\0');
	}

	void read2(handle::Stmt const& stmt, SQLUSMALLINT column, char(&out)[N]) const 
    { 
        SQLLEN read = 0;
        if(call(&SQLGetData, stmt, column, SQL_C_CHAR, out, N * sizeof(char), &read) == SQL_NO_DATA)
            throw std::runtime_error("SQLGetData can be called once only");
        switch (read)
        {
            case SQL_NO_TOTAL:
                throw std::length_error{""};
            case SQL_NULL_DATA:
                read = 0;
                //[[fallthrough]];
            default:
                assert(N >= std::size_t(read));
                std::memset(out + read, 0, N - read);
        }
    }
};

template<std::size_t N>
struct Bind<std::uint8_t[N], boost::mpl::true_>
{
	using SQLType = typename std::conditional < details::BinarySizeLimit < N, std::integral_constant<SQLSMALLINT, SQL_LONGVARBINARY>, std::integral_constant<SQLSMALLINT, SQL_LONGVARBINARY> >::type;

	void column(handle::Stmt const& stmt, SQLUSMALLINT column, SQLPOINTER data, SQLLEN* indicator)
	{
		call(&SQLBindCol, stmt, column, SQL_C_BINARY, data, N, indicator);
	}

	std::size_t dyn_column(handle::Stmt const& stmt, SQLUSMALLINT column, std::size_t offset)
	{
		return bind_column<std::uint8_t>(stmt, column, SQL_C_BINARY, offset, sizeof(std::uint8_t) * N);
	}

	using ValueType = std::vector<std::uint8_t>;
	ValueType construct(SQLLEN const* row) const
	{
		if (*row == SQL_NULL_DATA)
			return {};
		auto val = data_cast<std::uint8_t>(row);
		return { val, val + std::size_t(*row) };
	}
	ValueType construct(handle::Stmt const& stmt, SQLUSMALLINT column) const { return read_data<std::uint8_t>(stmt, column, SQL_C_BINARY); }

	void copy2(SQLLEN const* row, std::uint8_t (&out)[N]) const
	{
		auto size = SQL_NULL_DATA == *row ? 0 : std::size_t(*row);
		auto begin = data_cast<std::uint8_t>(row);
		std::copy(begin, begin + size, out);
		std::fill(out + size, out + N, 0);
	}
	void read2(handle::Stmt const& stmt, SQLUSMALLINT column, std::uint8_t(&out)[N])
    {
        SQLLEN read = 0;
        if(call(&SQLGetData, stmt, column, SQL_C_BINARY, out, N * sizeof(std::uint8_t), &read) == SQL_NO_DATA)
            throw std::runtime_error("SQLGetData can be called once only");
        switch (read)
        {
            case SQL_NO_TOTAL:
                throw std::length_error{""};
            case SQL_NULL_DATA:
                read = 0;
                //[[fallthrough]];
            default:
                assert(N >= std::size_t(read));
                std::memset(out + read, 0, N - read);
        }
    }
};


template<typename T, typename U = void>
struct StaticallyBindable : std::false_type {};

template<typename T>
struct StaticallyBindable<T, decltype(std::declval<Bind<T>>().column(std::declval<handle::Stmt>(), 0, nullptr, nullptr))> : std::true_type {};

template<typename Sequence>
struct IsSequenceStaticallyBindable :
	std::is_same<typename boost::mpl::find_if<Sequence, boost::mpl::not_<StaticallyBindable<boost::mpl::placeholders::_1>>>::type,
														typename boost::mpl::end<Sequence>::type>
{
	static_assert(boost::fusion::traits::is_sequence<Sequence>::value, "fusion sequence expected");
};

template<typename Sequence>
struct StaticBindings
{
	static_assert(boost::fusion::traits::is_sequence<Sequence>::value, "fusion sequence expected");
	static_assert(IsSequenceStaticallyBindable<Sequence>::value, "fusion sequence supposed to be dynamically bindable");
	struct Row
	{
		Sequence value;
		SQLLEN indicators[boost::fusion::result_of::size<Sequence>::value];
	};
	static StaticBindings bind(handle::Stmt const& stmt)
	{
		bind_<0>(stmt);
		SQLSetStmtAttr(stmt, SQL_ATTR_ROW_BIND_TYPE, reinterpret_cast<SQLPOINTER>(sizeof(Row)), 0);
		return {};
	}
private:
	template<std::size_t N>
	static auto bind_(handle::Stmt const& stmt) ->typename std::enable_if<boost::fusion::result_of::size<Sequence>::value != N, void>::type
	{
		auto row = reinterpret_cast<Row const*>(bindings_offset);
		auto const& value = boost::fusion::at_c<N>(row->value);
		Bind<typename boost::fusion::result_of::value_at_c<Sequence, N>::type>{}.column(stmt, N + 1, SQLPOINTER(&value), const_cast<SQLLEN*>(row->indicators + N));
		bind_<N + 1>(stmt);
	}

	template<std::size_t N>
	static auto bind_(handle::Stmt const& /*stmt*/) ->typename std::enable_if<boost::fusion::result_of::size<Sequence>::value == N, void>::type {}
};



template<typename T, typename Enabled = boost::mpl::true_> struct DynamicBind : Bind<T> {};

template<typename T>
std::size_t bind_column_dyn(handle::Stmt const& stmt, SQLUSMALLINT column, SQLSMALLINT type, std::size_t offset)
{
		assert(offset % alignof(SQLLEN) == 0 && "Oops! offset should be aligned to SQLLEN(indicator type)");
		SQLSMALLINT name_length = 0;
		SQLSMALLINT type_;
		SQLULEN data_size;
		SQLSMALLINT data_digits;
		SQLSMALLINT nullable;

		call(&SQLDescribeCol,
							stmt,                  // Select Statement (Prepared)
							column,                   // Columnn Number
							nullptr,                  // Column Name (returned)
							0,                        // size of Column Name buffer
							&name_length,             // Actual size of column name
							&type_,                    // SQL Data type of column
							&data_size,               // Data size of column in table
							&data_digits,             // Number of decimal digits
							&nullable);               // Whether column nullable

		constexpr SQLULEN MaxBindableDataSize = 8000;
		return data_size == 0 || data_size > MaxBindableDataSize
			 ? offset
			 : bind_column<T>(stmt, column, type, offset, data_size);
}

template<typename T>
struct DynamicBind<std::basic_string<T>, boost::mpl::true_>
{
	std::size_t dyn_column(handle::Stmt const& stmt, SQLUSMALLINT column, std::size_t offset)
	{
		return bind_column_dyn<T>(stmt, column, SQL_CHAR, offset);
	}

	using ValueType = std::basic_string<T>;
	ValueType construct(SQLLEN const* row) const
	{
		auto begin = data_cast<T>(row);
		return { begin, std::size_t(*row) };
	}

	ValueType construct(handle::Stmt const& stmt, SQLUSMALLINT column) const
	{
		auto buffer = read_data<T>(stmt, column, SQL_CHAR);
		return { cbegin(buffer), cend(buffer) };
	}

	void copy2(SQLLEN const* row, ValueType& out) const
	{
		if (*row != SQL_NULL_DATA)
		{
			auto begin = data_cast<T>(row);
			out.assign(begin, begin + *row);
		}
	}

	void read2(handle::Stmt const& stmt, SQLUSMALLINT column, ValueType& out) const
	{
		out = construct(stmt, column);
	}
};

template<typename T>
struct DynamicBindVector
{
	using ValueType = std::vector<T>;
	ValueType construct(SQLLEN const* row) const
	{
		auto begin = data_cast<T>(row);
		return { begin, begin + *row };
	}

	ValueType construct(handle::Stmt const& stmt, SQLUSMALLINT column) const
	{
		return read_data<T>(stmt, column, SQL_C_BINARY);
	}

	void copy2(SQLLEN const* row, std::vector<T>& out) const
	{
		if (*row != SQL_NULL_DATA)
		{
			auto begin = data_cast<T>(row);
			out.assign(begin, begin + *row);
		}
	}

	void read2(handle::Stmt const& stmt, SQLUSMALLINT column, std::vector<T>& out) const
	{
		out = read_data<T>(stmt, column, SQL_C_BINARY);
	}
};

template<>
struct DynamicBind<std::vector<char>, boost::mpl::true_> : DynamicBindVector<char>
{
	std::size_t dyn_column(handle::Stmt const& stmt, SQLUSMALLINT column, std::size_t offset)
	{
		return bind_column_dyn<char>(stmt, column, SQL_CHAR, offset);
	}
};

template<typename T>
struct DynamicBind<std::vector<T>, boost::mpl::bool_<sizeof(T) == sizeof(std::uint8_t)>> : DynamicBindVector<T>
{
	std::size_t dyn_column(handle::Stmt const& stmt, SQLUSMALLINT column, std::size_t offset)
	{
		return bind_column_dyn<T>(stmt, column, SQL_C_BINARY, offset);
	}
};

template<typename T, typename U = std::size_t>
struct DynamicallyBindable : StaticallyBindable<T> {};

template<typename T>
struct DynamicBind<diversion::optional<T>, boost::mpl::bool_<DynamicallyBindable<T>::value>> : DynamicBind<T>
{
	using Base = DynamicBind<T>;
	using DynamicBind<T>::dyn_column;

	using ValueType = diversion::optional<typename Base::ValueType>;
	ValueType construct(SQLLEN const* row) const
	{
		auto begin = data_cast<T>(row);
		return begin == SQL_NULL_DATA ?  diversion::nullopt : diversion::make_optional(Base::construct(row));
	}

	ValueType construct(handle::Stmt const& /*stmt*/, SQLUSMALLINT /*column*/) const
	{
#pragma message("revisit it!!!!!!!!!!!!!!")
		assert("not implemented yet!!!");
		return diversion::nullopt;
	}

	void copy2(SQLLEN const* row, ValueType& out) const
	{
		if (*row != SQL_NULL_DATA)
		{
			out = diversion::make_optional(typename Base::ValueType{});
			Base::copy2(row, out.value());
		}
	}

	void read2(handle::Stmt const& stmt, SQLUSMALLINT column, ValueType& out) const
	{
		out = construct(stmt, column);
	}
};

template<typename T>
struct DynamicallyBindable<T, decltype(std::declval<DynamicBind<T>>().dyn_column(std::declval<handle::Stmt>(), 0, 0))> : std::true_type {};

template<typename Sequence>
struct IsSequenceDynamicallyBindable :
	std::is_same<typename boost::mpl::find_if<Sequence, boost::mpl::not_<DynamicallyBindable<boost::mpl::placeholders::_1>>>::type,
														typename boost::mpl::end<Sequence>::type>
{
	static_assert(boost::fusion::traits::is_sequence<Sequence>::value, "fusion sequence expected");
};



template<typename Sequence>
class DynamicBindings
{
	static_assert(boost::fusion::traits::is_sequence<Sequence>::value, "fusion sequence expected");
	static_assert(IsSequenceDynamicallyBindable<Sequence>::value, "fusion sequence supposed to be dynamically bindable");
	using Offsets = std::vector<std::size_t>;

	DynamicBindings(Offsets&& offsets) : offsets_{ std::move(offsets) } {}
public:
	DynamicBindings(DynamicBindings&&) = default;
	DynamicBindings& operator= (DynamicBindings&&) = default;

	bool bulk_fetch() const { return boost::fusion::result_of::size<Sequence>::value == offsets_.size();}
	bool empty() const { return offsets_.empty(); }
	std::size_t size() const { return offsets_.size(); }
    std::size_t row_size() const { return offsets_.empty() ? 1 : offsets_.back(); }

	static DynamicBindings bind(handle::Stmt const& stmt)
	{
        using FirstDynamicallyBindable = typename boost::mpl::find_if<Sequence, boost::mpl::not_<StaticallyBindable<boost::mpl::placeholders::_1>>>::type;
        using RestBindable = boost::mpl::iterator_range<FirstDynamicallyBindable, typename boost::mpl::end<Sequence>::type>;
        using IsOrderOptimal = std::is_same<typename boost::mpl::find_if<RestBindable, StaticallyBindable<boost::mpl::placeholders::_1>>::type, typename boost::mpl::end<Sequence>::type>;
        validate<Sequence>(IsOrderOptimal{});
		Offsets offsets;
		bind_<0>(stmt, 0, offsets);
		if(!offsets.empty())
			SQLSetStmtAttr(stmt, SQL_ATTR_ROW_BIND_TYPE, reinterpret_cast<SQLPOINTER>(offsets.back()), 0);
		return { std::move(offsets) };
	}

    Sequence value(handle::Stmt const& stmt, char const* row) const
    {
        Sequence val;
        init<0>(stmt, row, val);
        return val;
    }

	template<std::size_t N>
	using ValueType = typename DynamicBind<typename boost::fusion::result_of::value_at_c<Sequence, N>::type>::ValueType;

	template<std::size_t N>
	ValueType<N> get(handle::Stmt const& stmt, char const* row) const
	{
		auto bind = DynamicBind<typename boost::fusion::result_of::value_at_c<Sequence, N>::type>{};
		if (offsets_.size() > N)
		{
			auto offset = N == 0 ? 0 : offsets_[N - 1];
			auto indicator = reinterpret_cast<SQLLEN const*>(row + offset);
			return bind.construct(indicator);
		}

		return bind.construct(stmt, SQLUSMALLINT(N + 1));
	}
private:
    template<typename T>
    inline static void validate(std::true_type) {}
    template<typename T>
    [[deprecated("for better performance please consider placing statically bindable fields like int, float, enums etc before dynamically bindable like std::string std::vector<> etc.")]]
    inline static void validate(std::false_type) {}

	template<std::size_t N>
	static auto bind_(handle::Stmt const& stmt, std::size_t offset, Offsets& offsets) -> typename std::enable_if<boost::fusion::result_of::size<Sequence>::value != N, void>::type
	{
		std::size_t next = (DynamicBind<typename boost::fusion::result_of::value_at_c<Sequence, N>::type>{}.dyn_column(stmt, N + 1, offset) + alignof(SQLLEN) - 1) / alignof(SQLLEN);
		next *= alignof(SQLLEN);
		if (next != offset)
		{
			offsets.emplace_back(next);
			bind_<N + 1>(stmt, next, offsets);
		}
	}

	template<std::size_t N>
	static auto bind_(handle::Stmt const& /*stmt*/, std::size_t offset, Offsets& offsets) -> typename std::enable_if<boost::fusion::result_of::size<Sequence>::value == N, void>::type {}

	template<std::size_t N>
	auto init(handle::Stmt const& stmt, char const* row, Sequence& sequence) const -> typename std::enable_if<boost::fusion::result_of::size<Sequence>::value != N, void>::type
	{
		if (offsets_.size() > N)
		{
			auto offset = N == 0 ? 0 : offsets_[N - 1];
			auto indicator = reinterpret_cast<SQLLEN const*>(row + offset);
			DynamicBind<typename boost::fusion::result_of::value_at_c<Sequence, N>::type>{}.copy2(indicator, boost::fusion::at_c<N>(sequence));
			init<N + 1>(stmt, row, sequence);
		}
		else
			read<N>(stmt, sequence);
	}

	template<std::size_t N>
	auto read(handle::Stmt const& stmt, Sequence& sequence) const -> typename std::enable_if<boost::fusion::result_of::size<Sequence>::value != N, void>::type
	{
		DynamicBind<typename boost::fusion::result_of::value_at_c<Sequence, N>::type>{}.read2(stmt, SQLUSMALLINT(N + 1), boost::fusion::at_c<N>(sequence));
		read<N + 1>(stmt, sequence);
	}

	template<std::size_t N>
	auto init(handle::Stmt const& /*stmt*/, char const* /*row*/, Sequence& /*sequence*/) const -> typename std::enable_if<boost::fusion::result_of::size<Sequence>::value == N, void>::type {}

	template<std::size_t N>
	auto read(handle::Stmt const& /*stmt*/, Sequence& /*sequence*/) const -> typename std::enable_if<boost::fusion::result_of::size<Sequence>::value == N, void>::type {}
private:
	Offsets offsets_;
};

template<typename Sequence>
struct NameGenerator
{
	NameGenerator(boost::string_view alias) : alias_{ std::move(alias) } {}

	static_assert(boost::fusion::traits::is_sequence<Sequence>::value, "fusion sequence expected");
	template<typename T>
	std::string operator()(std::string const& str, const T& t) const
	{
		auto res = str;
		res += ',';
		res.append(alias_.data(), alias_.length());
		res += '.';
		res += boost::fusion::extension::struct_member_name<Sequence, T::value>::call();
		return res;
	}
private:
	boost::string_view alias_;
};


}  /*namespace out*/}  /*namespace details*/ }/*inline namespace v0*/} //namespace odbcx
