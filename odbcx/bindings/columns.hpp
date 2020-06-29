// Copyright (c) 2018-2019 Serge Klimov serge.klim@outlook.com

#pragma once
#include "adapter.hpp"
#include "surrogate.hpp"
#include "flyweight.hpp"
#include "ttraits.hpp"
#include "odbcx/handle.hpp"
#include "odbcx/details/cast.hpp"
#include "odbcx/details/bookmark.hpp"
#include "odbcx/adapter.hpp"
#include "odbcx/utility.hpp"
#include "odbcx/details/diversion.hpp"
#include <boost/fusion/include/size.hpp>
#include <boost/fusion/include/value_at.hpp>
#include <boost/fusion/include/at_c.hpp>
#include <boost/fusion/adapted/struct/detail/extension.hpp>
#include <boost/fusion/include/is_sequence.hpp>
#include <boost/fusion/adapted/std_tuple.hpp>
#include <boost/fusion/sequence/convert.hpp>
#include <boost/fusion/include/mpl.hpp>
#include <boost/mp11/algorithm.hpp>
#include <boost/mp11/function.hpp>
#include <boost/mp11/integral.hpp>
#include <utility>
#include <algorithm>
#include <string>
#include <type_traits>
#include <cstring>
#include <cstdint>
#include <cstddef>
#include <cassert>

namespace odbcx { inline namespace v1 { namespace details { namespace columns {

template<typename T, typename Enabled = boost::mp11::mp_true> struct Bind;


template<>
struct Bind<NoBind, boost::mp11::mp_true>
{
    /*constexpr*/ void column(handle::adapter::Stmt const& /*stmt*/, SQLUSMALLINT /*column*/, SQLPOINTER /*data*/, SQLLEN* /*indicator*/) const noexcept {}
    constexpr std::size_t dyn_column(handle::adapter::Stmt const& /*stmt*/, SQLUSMALLINT /*column*/, std::size_t offset) const noexcept { return offset; }
    constexpr SQLLEN set_indicator(NoBind const&) const noexcept { return SQL_COLUMN_IGNORE; }
    constexpr NoBind instantiate(SQLLEN const* /*begin*/, SQLLEN const* /*end*/) const noexcept { return {}; }
    constexpr NoBind instantiate(handle::adapter::Stmt const& /*stmt*/, SQLUSMALLINT /*column*/) const noexcept { return {}; }
};


template<typename T>
std::size_t bind_column(handle::adapter::Stmt const& stmt, SQLUSMALLINT column, SQLSMALLINT type, std::size_t offset, std::size_t size)
{
    return DynamicLayout<T>::bind(stmt, column, type, offset, size);
}

template<typename T>
void rebind_column(handle::adapter::Stmt const& stmt, SQLUSMALLINT column, SQLSMALLINT type, std::size_t begin, std::size_t end)
{
    DynamicLayout<T>::rebind(stmt, column, type, begin, end);
}

template<>
struct Bind<SQL_TIMESTAMP_STRUCT, boost::mp11::mp_true>
{
	void column(handle::adapter::Stmt const& stmt, SQLUSMALLINT column, SQLPOINTER data, SQLLEN* indicator) const
	{
		call(&SQLBindCol, stmt, column, SQL_C_TYPE_TIMESTAMP, data, sizeof(SQL_TIMESTAMP_STRUCT), indicator);
	}

	std::size_t dyn_column(handle::adapter::Stmt const& stmt, SQLUSMALLINT column, std::size_t offset) const
	{
		return bind_column<SQL_TIMESTAMP_STRUCT>(stmt, column, SQL_C_TYPE_TIMESTAMP, offset, sizeof(SQL_TIMESTAMP_STRUCT));
	}

    void rebind(handle::adapter::Stmt const& stmt, SQLUSMALLINT column, std::size_t begin, std::size_t /*end*/) const
    { 
        bind_column<SQL_TIMESTAMP_STRUCT>(stmt, column, SQL_C_TYPE_TIMESTAMP, std::size_t(begin), sizeof(SQL_TIMESTAMP_STRUCT));
    }

    constexpr SQLLEN set_indicator(SQL_TIMESTAMP_STRUCT const&) const noexcept { return sizeof(SQL_TIMESTAMP_STRUCT); }

    Flyweight<SQL_TIMESTAMP_STRUCT const> instantiate(SQLLEN const* begin, SQLLEN const* end) { return make_flyweight<SQL_TIMESTAMP_STRUCT>(begin, end); }
    Flyweight<SQL_TIMESTAMP_STRUCT> instantiate(SQLLEN* begin, SQLLEN* end) { return make_flyweight<SQL_TIMESTAMP_STRUCT>(begin, end); }
    Surrogate<SQL_TIMESTAMP_STRUCT> instantiate(handle::adapter::Stmt const& stmt, SQLUSMALLINT column) const noexcept { return {stmt, column}; }
};

template<typename T> 
struct Bind<T, typename boost::mp11::mp_or<std::is_integral<T>, std::is_floating_point<T>>::type>
{   
	void column(handle::adapter::Stmt const& stmt, SQLUSMALLINT column, SQLPOINTER data, SQLLEN* indicator) const
	{		
		call(&SQLBindCol, stmt, column, CType<T>::value, data, CTypeSizeOf<T>::value, indicator);
	}

	std::size_t dyn_column(handle::adapter::Stmt const& stmt, SQLUSMALLINT column, std::size_t offset) const
	{
		return bind_column<T>(stmt, column, CType<T>::value, offset, CTypeSizeOf<T>::value);
	}

    void rebind(handle::adapter::Stmt const& stmt, SQLUSMALLINT column, std::size_t begin, std::size_t /*end*/) const
    {
        bind_column<T>(stmt, column, CType<T>::value, begin, CTypeSizeOf<T>::value);
    }
    constexpr SQLLEN set_indicator(T const&) const noexcept { return CTypeSizeOf<T>::value; }

    Flyweight<T const> instantiate(SQLLEN const* begin, SQLLEN const* end) { return make_flyweight<T>(begin, end); }
    Flyweight<T> instantiate(SQLLEN* begin, SQLLEN* end) { return make_flyweight<T>(begin, end); }
    Surrogate<T> instantiate(handle::adapter::Stmt const& stmt, SQLUSMALLINT column) const noexcept { return { stmt, column }; }
};

template<typename T>
struct Bind<T, boost::mp11::mp_bool<std::is_enum<T>::value>> : Bind<typename std::underlying_type<T>::type> 
{
    using UnderlyingType = typename std::underlying_type<T>::type;
    Flyweight<T const> instantiate(SQLLEN const* begin, SQLLEN const* end) { return make_flyweight<T>(begin, end); }
    Flyweight<T> instantiate(SQLLEN* begin, SQLLEN* end) { return make_flyweight<T>(begin, end); }
    Surrogate<T, details::CType<UnderlyingType>::value, details::CTypeSizeOf<UnderlyingType>::value> instantiate(handle::adapter::Stmt const& stmt, SQLUSMALLINT column) const noexcept { return { stmt, column }; }
};


template<std::size_t N>
struct Bind<char[N], boost::mp11::mp_true>
{
	void column(handle::adapter::Stmt const& stmt, SQLUSMALLINT column, SQLPOINTER data, SQLLEN* indicator)
	{
		call(&SQLBindCol, stmt, column, SQL_C_CHAR, data, N, indicator);
	}

	std::size_t dyn_column(handle::adapter::Stmt const& stmt, SQLUSMALLINT column, std::size_t offset)
	{
        return bind_column<char[N]> (stmt, column, SQL_C_CHAR, offset, N);
	}

    void rebind(handle::adapter::Stmt const& stmt, SQLUSMALLINT column, std::size_t begin, std::size_t /*end*/) const
    {
        bind_column<char[N]>(stmt, column, SQL_C_CHAR, begin, N);
    }

    SQLLEN set_indicator(char const* value) const noexcept { return SQLLEN(std::distance(value, std::find(value, value + N, '\0'))); }
    Flyweight<char const [N]> instantiate(SQLLEN const* begin, SQLLEN const* end) { return make_flyweight<char[N]>(begin, end); }
    Flyweight<char[N]> instantiate(SQLLEN* begin, SQLLEN* end) { return make_flyweight<char[N]>(begin, end); }
    Surrogate<char[N], SQL_C_CHAR> instantiate(handle::adapter::Stmt const& stmt, SQLUSMALLINT column) const noexcept { return { stmt, column }; }
};

template<std::size_t N>
struct Bind<std::uint8_t[N], boost::mp11::mp_true>
{
	using SQLType = typename std::conditional < details::max_bindable_data_size < N, std::integral_constant<SQLSMALLINT, SQL_LONGVARBINARY>, std::integral_constant<SQLSMALLINT, SQL_LONGVARBINARY> >::type;

	void column(handle::adapter::Stmt const& stmt, SQLUSMALLINT column, SQLPOINTER data, SQLLEN* indicator)
	{
		call(&SQLBindCol, stmt, column, SQL_C_BINARY, data, N, indicator);
	}

	std::size_t dyn_column(handle::adapter::Stmt const& stmt, SQLUSMALLINT column, std::size_t offset)
	{
		return bind_column<std::uint8_t[N]>(stmt, column, SQL_C_BINARY, offset, N);
	}

    void rebind(handle::adapter::Stmt const& stmt, SQLUSMALLINT column, std::size_t begin, std::size_t /*end*/) const
    {
        bind_column<uint8_t[N]>(stmt, column, SQL_C_BINARY, begin, N);
    }

    constexpr SQLLEN set_indicator(std::uint8_t const*) const noexcept { return SQLLEN(N); }
    Flyweight<std::uint8_t const [N]> instantiate(SQLLEN const* begin, SQLLEN const* end) { return make_flyweight<std::uint8_t[N]>(begin, end); }
    Flyweight<std::uint8_t[N]> instantiate(SQLLEN* begin, SQLLEN* end) { return make_flyweight<std::uint8_t[N]>(begin, end); }
    Surrogate<std::uint8_t[N], SQL_C_BINARY> instantiate(handle::adapter::Stmt const& stmt, SQLUSMALLINT column) const noexcept { return { stmt, column }; }
};


template<typename T, typename U = void>
struct StaticallyBindable : std::false_type {};

template<typename T>
struct StaticallyBindable<T, decltype(std::declval<Bind<T>>().column(std::declval<handle::adapter::Stmt const&>(), 0, nullptr, nullptr))> : std::true_type {};

template<typename Sequence>
using IsSequenceStaticallyBindable = boost::mp11::mp_all_of<typename boost::fusion::result_of::convert<boost::fusion::std_tuple_tag, Sequence>::type, StaticallyBindable>;


template<typename Sequence, bool WithBookmarks = false>
class StaticBindings
{
	static_assert(boost::fusion::traits::is_sequence<Sequence>::value, "fusion sequence expected");
	static_assert(IsSequenceStaticallyBindable<Sequence>::value, "fusion sequence supposed to be statically bindable");
	struct RawRow
	{
        RawRow() = default;
        RawRow(Sequence && value) : value(std::forward<Sequence>(value)) {}
		Sequence value;
		SQLLEN indicators[boost::fusion::result_of::size<Sequence>::value];
	};
    struct BMRow
    {
        BMRow() = default;
        BMRow(Sequence&& value) : value(std::forward<Sequence>(value)) { std::memset(&bookmark, 0, sizeof(bookmark)); }
        VarBookmark bookmark;
        Sequence value;
        SQLLEN indicators[boost::fusion::result_of::size<Sequence>::value];
    };

public:
    using Row = typename std::conditional<WithBookmarks, BMRow, RawRow>::type;
    static constexpr bool bulk_fetch() noexcept { return true; }
    static constexpr std::size_t row_size() noexcept { return 1; }

    struct BindCol
    {
        BindCol(handle::adapter::Stmt const& stmt) :stmt_(stmt) {}
        template<typename Ix>
        void operator()(Ix) const
        {
            auto row = reinterpret_cast<Row const*>(binding_offset);
            auto const& value = boost::fusion::at_c<Ix::value>(row->value);
            Bind<typename boost::fusion::result_of::value_at_c<Sequence, Ix::value>::type>{}.column(stmt_, Ix::value + 1, SQLPOINTER(&value), const_cast<SQLLEN*>(row->indicators + Ix::value));
        }
    private:
        handle::adapter::Stmt const& stmt_;
    };

	static StaticBindings bind(handle::adapter::Stmt const& stmt)
	{
        boost::mp11::mp_for_each<boost::mp11::mp_iota_c<boost::fusion::result_of::size<Sequence>::value>>(BindCol{stmt});
        if /*constexpr*/ (WithBookmarks)
        {
            auto row = reinterpret_cast<BMRow*>(binding_offset);
            VarBookmark::bind(stmt, &row->bookmark);
        }
        odbcx::handle::set_attribute(stmt, SQL_ATTR_ROW_BIND_TYPE, sizeof(Row));
		return {};
	}

    void rebind(handle::adapter::Stmt const& stmt) const { bind(stmt); } 

    struct SetRowIndicators
    {
        SetRowIndicators(Row& row) :row_{ &row } {}
        template<typename Ix>
        void operator()(Ix) const
        {
            auto const& value = boost::fusion::at_c<Ix::value>(row_->value);
            row_->indicators[Ix::value] = Bind<typename boost::fusion::result_of::value_at_c<Sequence, Ix::value>::type>{}.set_indicator(value);
        }
    private:
        Row* row_;
    };

    static void emplace(std::vector<Row>& data, Sequence&& value)
    {       
        data.emplace_back(std::forward<Sequence>(value));
        boost::mp11::mp_for_each<boost::mp11::mp_iota_c<boost::fusion::result_of::size<Sequence>::value>>(SetRowIndicators{data.back()});
        //boost::mp11::mp_for_each<boost::mp11::mp_iota_c<boost::fusion::result_of::size<Sequence>::value>>([&](auto ix)
        //    {
        //        auto const& value = boost::fusion::at_c<decltype(ix)::value>(row.value);
        //        row.indicators[decltype(ix)::value] = Bind<typename boost::fusion::result_of::value_at_c<Sequence, decltype(ix)::value>::type>{}.set_indicator(value);
        //    });
    }

    template<typename ...Args>
    static auto emplace(std::vector<Row>& data, Args&&...args)
        -> typename std::enable_if<std::is_constructible<Sequence, Args...>::value>::type
    {
        return emplace(data, Sequence(std::forward<Args>(args)...));
    }

    template<typename ...Args>
    static auto emplace(std::vector<Row>& data, Args&&...args)
        -> typename std::enable_if<!std::is_constructible<Sequence, Args...>::value
                                        && std::is_same<decltype(Sequence{ std::forward<Args>(args)... }), Sequence >::value>::type
    {
        return emplace(data, Sequence{ std::forward<Args>(args)... });
    }
};

template<typename T>
std::size_t bind_column_dyn(handle::adapter::Stmt const& stmt, SQLUSMALLINT column, SQLSMALLINT type, std::size_t offset)
{
	SQLSMALLINT name_length = 0;
	SQLSMALLINT type_;
	SQLULEN data_size;
	SQLSMALLINT data_digits;
	SQLSMALLINT nullable;

	call(&SQLDescribeCol,
						stmt,                     // Select Statement (Prepared)
						column,                   // Columnn Number
						nullptr,                  // Column Name (returned)
						0,                        // size of Column Name buffer
						&name_length,             // Actual size of column name
						&type_,                   // SQL Data type of column
						&data_size,               // Data size of column in table
						&data_digits,             // Number of decimal digits
						&nullable);               // Whether column nullable

	return data_size == 0 || data_size > details::max_bindable_data_size
			? offset
			: bind_column<T>(stmt, column, type, offset, data_size);
}

template<typename T, typename Enabled = boost::mp11::mp_true> struct DynamicBind : Bind<T> {};

template<typename T>
struct DynamicBind<std::basic_string<T>, boost::mp11::mp_true>
{
	std::size_t dyn_column(handle::adapter::Stmt const& stmt, SQLUSMALLINT column, std::size_t offset) { return bind_column_dyn<T>(stmt, column, SQL_CHAR, offset);}
    void rebind(handle::adapter::Stmt const& stmt, SQLUSMALLINT column, std::size_t begin, std::size_t end) const { rebind_column<T>(stmt, column, SQL_CHAR, begin, end); }
    FlyweightArray<T const> instantiate(SQLLEN const* begin, SQLLEN const* end) { return make_flyweight_array<T>(begin, end); }
    FlyweightArray<T> instantiate(SQLLEN* begin, SQLLEN* end) { return make_flyweight_array<T>(begin, end); }
    odbcx::SurrogateVector<T, SQL_C_CHAR> instantiate(handle::adapter::Stmt const& stmt, SQLUSMALLINT column) const noexcept { return { stmt, column }; }
};

template<>
struct DynamicBind<std::vector<char>, boost::mp11::mp_true>
{
	std::size_t dyn_column(handle::adapter::Stmt const& stmt, SQLUSMALLINT column, std::size_t offset) { return bind_column_dyn<char>(stmt, column, SQL_CHAR, offset); }
    void rebind(handle::adapter::Stmt const& stmt, SQLUSMALLINT column, std::size_t begin, std::size_t end) const { rebind_column<char>(stmt, column, SQL_CHAR, begin, end); }
    FlyweightArray<char const> instantiate(SQLLEN const* begin, SQLLEN const* end) { return make_flyweight_array<char>(begin, end); }
    FlyweightArray<char> instantiate(SQLLEN* begin, SQLLEN* end) { return make_flyweight_array<char>(begin, end); }
    odbcx::SurrogateVector<char, SQL_C_CHAR> instantiate(handle::adapter::Stmt const& stmt, SQLUSMALLINT column) const noexcept { return { stmt, column }; }
};

template<typename T>
struct DynamicBind<std::vector<T>, boost::mp11::mp_bool<sizeof(T) == sizeof(std::uint8_t)>>
{
	std::size_t dyn_column(handle::adapter::Stmt const& stmt, SQLUSMALLINT column, std::size_t offset) { return bind_column_dyn<T>(stmt, column, SQL_C_BINARY, offset); }
    void rebind(handle::adapter::Stmt const& stmt, SQLUSMALLINT column, std::size_t begin, std::size_t end) const { rebind_column<T>(stmt, column, SQL_C_BINARY, begin, end); }

    FlyweightArray<T const> instantiate(SQLLEN const* begin, SQLLEN const* end) { return make_flyweight_array<T>(begin, end); }
    FlyweightArray<T> instantiate(SQLLEN* begin, SQLLEN* end) { return make_flyweight_array<T>(begin, end); }
    odbcx::SurrogateVector<T, SQL_C_BINARY> instantiate(handle::adapter::Stmt const& stmt, SQLUSMALLINT column) const noexcept { return { stmt, column }; }
};


template<typename T, typename U = std::size_t>
struct DynamicallyBindable : StaticallyBindable<T> {};

template<typename T>
struct DynamicBind<T, boost::mp11::mp_bool<DynamicallyBindable<typename Adapter<T>::type>::value>> : DynamicBind<typename Adapter<T>::type>
{
    using Base = DynamicBind<typename Adapter<T>::type>;
    using DynamicBind<typename Adapter<T>::type>::dyn_column;
    using DynamicBind<typename Adapter<T>::type>::rebind;
    template<typename ...Args>
    auto instantiate(Args&&...args) -> AdapterProxy<T, decltype(std::declval<Base>().instantiate(std::forward<Args>(args)...))>
    {
        return { adapt<T>(Base::instantiate(std::forward<Args>(args)...)) };
    }
};

template<typename T>
struct DynamicBind<diversion::optional<T>, boost::mp11::mp_bool<DynamicallyBindable<T>::value>> : DynamicBind<T>
{
	using Base = DynamicBind<T>;
	using DynamicBind<T>::dyn_column;
    using DynamicBind<T>::rebind;
    using DynamicBind<T>::instantiate;
};

//https://docs.microsoft.com/en-us/sql/odbc/reference/appendixes/retrieve-numeric-data-sql-numeric-struct-kb222831?view=sql-server-ver15
//https://www.boost.org/doc/libs/1_71_0/libs/multiprecision/doc/html/index.html
//SQL_NUMERIC_STRUCT

template<typename T>
struct DynamicallyBindable<T, decltype(std::declval<DynamicBind<T>>().dyn_column(std::declval<handle::adapter::Stmt const&>(), 0, 0))> : std::true_type {};

template<typename Sequence>
using IsSequenceDynamicallyBindable = boost::mp11::mp_all_of<typename boost::fusion::result_of::convert<boost::fusion::std_tuple_tag, Sequence>::type, DynamicallyBindable>;


template<typename Sequence>
using HasArrays = boost::mp11::mp_any_of<typename boost::fusion::result_of::convert<boost::fusion::std_tuple_tag, Sequence>::type, std::is_array>;

template<typename Sequence, typename ...Args>
struct IsSequenceAssignableImpl
{
    static_assert(boost::fusion::traits::is_sequence<Sequence>::value, "fusion sequence expected");
    static_assert(boost::fusion::result_of::size<Sequence>::value == sizeof...(Args), "fusion sequence size should much Args number");

    using SequenceAsTuple = typename boost::fusion::result_of::convert<boost::fusion::std_tuple_tag, Sequence>::type;
    using Zypped = boost::mp11::mp_transform<std::pair, SequenceAsTuple, boost::mp11::mp_list<Args...> >;
    template<typename T>
    using IsAssignable = std::is_assignable<typename std::add_lvalue_reference<typename T::first_type>::type, typename T::second_type>;
    using type = boost::mp11::mp_all_of<Zypped, IsAssignable>;
};

template<typename Sequence, typename ...Args>
struct IsAssignableImpl
{
   using type = boost::mp11::mp_and<boost::mp11::mp_bool<boost::fusion::result_of::size<Sequence>::value == sizeof...(Args)>, typename IsSequenceAssignableImpl<Sequence, Args...>::type>;
};

template<typename L, typename R>
struct IsAssignableImpl<L, R>
{
    using type = std::is_assignable<typename std::add_lvalue_reference<L>::type, R>;
};

template<typename Sequence, typename ...Args>
using IsAssignable = typename IsAssignableImpl<Sequence, Args...>::type;

template<typename Sequence, bool WithBookmarks = false>
struct DynamicBindings
{
    static_assert(boost::fusion::traits::is_sequence<Sequence>::value, "fusion sequence expected");
    static_assert(IsSequenceDynamicallyBindable<Sequence>::value, "fusion sequence supposed to be dynamically bindable");
    using Offsets = std::vector<std::size_t>; // number of SQLLEN
    DynamicBindings(Offsets&& offsets) : offsets_{ std::move(offsets) } {}

    struct Initialize
    {
        template<typename Ix>
        void operator()(Ix) const { owner->initialize<Ix::value>(value, stmt, row); }
        DynamicBindings<Sequence, WithBookmarks> const* owner;
        Sequence& value;
        handle::adapter::Stmt const& stmt;
        SQLLEN const* row;
    };

    struct Assign
    {
        template<typename Ix>
        void operator()(Ix) const { owner->assign<Ix::value>(row, value); }
        DynamicBindings<Sequence, WithBookmarks> const* owner;
        SQLLEN* row;
        Sequence const& value;
    };
public:
    bool bulk_fetch() const { return boost::fusion::result_of::size<Sequence>::value <= offsets_.size(); }
    bool empty() const { return offsets_.empty(); }
    std::size_t size() const { return offsets_.size(); }
    std::size_t row_size() const { return offsets_.empty() ? 1 : offsets_.back(); }
    //static constexpr std::size_t binding_offset() noexcept { return 1; }

    constexpr VarBookmark const& bookmark(SQLLEN const* row) const noexcept
    {
        static_assert(WithBookmarks, "Oops! this bindings doesn't have any bookmarks!");
        return *reinterpret_cast<VarBookmark const*>(row);
    }

    //VarBookmark bookmark(SQLLEN const* row) const noexcept
    //{
    //    static_assert(WithBookmarks, "Oops! this bindings doesn't have any bookmarks!");
    //    VarBookmark bookmark;
    //    std::memcpy(&bookmark, row, sizeof(VarBookmark));
    //    return bookmark;
    //}

     Sequence value(handle::adapter::Stmt const& stmt, SQLLEN const* row) const {  return value<Sequence>(stmt, row); }

    static DynamicBindings bind(handle::adapter::Stmt const& stmt) 
    { 
        //using FirstDynamicallyBindable = typename boost::mpl::find_if<Sequence, boost::mpl::not_<StaticallyBindable<boost::mpl::placeholders::_1>>>::type;
        //using RestBindable = boost::mpl::iterator_range<FirstDynamicallyBindable, typename boost::mpl::end<Sequence>::type>;
        //using IsOrderOptimal = std::is_same<typename boost::mpl::find_if<RestBindable, StaticallyBindable<boost::mpl::placeholders::_1>>::type, typename boost::mpl::end<Sequence>::type>;
        //validate<Sequence>(IsOrderOptimal{});
        return { bind<WithBookmarks>(stmt) }; 
    }
    void rebind(handle::adapter::Stmt const& stmt) const { rebind<WithBookmarks>(stmt); }
    void emplace(std::vector<SQLLEN>& data, Sequence const& value) const
    {
        assert(bulk_fetch() && "bulk operation is not possible!");
        auto size = data.size();
        data.resize(size + row_size());
        boost::mp11::mp_for_each<boost::mp11::mp_iota_c<boost::fusion::result_of::size<Sequence>::value>>(Assign{ this, data.data() + size, value });
    }

    void emplace(std::vector<SQLLEN>& data, Sequence&& row) const
    {
        return emplace(data, static_cast<Sequence const&>(row));
    }

    template<typename ...Args>
    auto emplace(std::vector<SQLLEN>& data, Args&&...args) const
        -> typename std::enable_if<std::is_constructible<Sequence, Args...>::value>::type
    {
        return emplace(data, Sequence(std::forward<Args>(args)...));
    }

    template<typename ...Args>
    auto emplace(std::vector<SQLLEN>& data, Args&&...args) const
        -> typename std::enable_if<!std::is_constructible<Sequence, Args...>::value && IsAssignable<Sequence, Args...>::value > ::type
    {
        assert(bulk_fetch() && "bulk operation is not possible!");
        auto size = data.size();
        data.resize(size + row_size());
        assign(boost::mp11::make_integer_sequence<std::size_t, sizeof...(args)>(), data.data() + size, std::forward<Args>(args)...);
    }
private:
    template<typename T>
    inline static void validate(std::true_type) {}
    template<typename T>
    [[deprecated("for better performance please consider placing statically bindable fields like int, float, enums etc before dynamically bindable like std::string std::vector<> etc.")]]
    inline static void validate(std::false_type) {}

    template<bool HasBookmarks> static auto bind(handle::adapter::Stmt const& stmt)
        -> typename std::enable_if<HasBookmarks, Offsets>::type
    {
        Offsets offsets;
        offsets.reserve(boost::fusion::result_of::size<Sequence>::value + 1);
        auto offset = (VarBookmark::bind(stmt, binding_offset) + sizeof(SQLLEN) - 1) / sizeof(SQLLEN) - binding_offset /sizeof(SQLLEN);
        offsets.emplace_back(offset);
        bind_<0>(stmt, offset, offsets);
        odbcx::handle::set_attribute(stmt, SQL_ATTR_ROW_BIND_TYPE, offsets.back() * sizeof(SQLLEN));
        return offsets;
    }

    template<bool HasBookmarks> static auto bind(handle::adapter::Stmt const& stmt)
        -> typename std::enable_if<!HasBookmarks, Offsets>::type
    {
        Offsets offsets;
        offsets.reserve(boost::fusion::result_of::size<Sequence>::value);
        bind_<0>(stmt, 0, offsets);
        if (!offsets.empty())
            odbcx::handle::set_attribute(stmt, SQL_ATTR_ROW_BIND_TYPE, offsets.back() * sizeof(SQLLEN));
        return offsets;
    }

    template<std::size_t N>
    static auto bind_(handle::adapter::Stmt const& stmt, std::size_t offset, Offsets& offsets) -> typename std::enable_if<boost::fusion::result_of::size<Sequence>::value != N>::type
    {
        using Type = typename boost::fusion::result_of::value_at_c<Sequence, N>::type;
        std::size_t next = DynamicBind<Type>{}.dyn_column(stmt, N + 1, offset);
        if (next != offset)
        {
            offsets.emplace_back(next);
            bind_<N + 1>(stmt, next, offsets);
        }
    }

    template<std::size_t N>
    static auto bind_(handle::adapter::Stmt const& /*stmt*/, std::size_t offset, Offsets& offsets) -> typename std::enable_if<boost::fusion::result_of::size<Sequence>::value == N>::type {}

    template<bool HasBookmarks> auto rebind(handle::adapter::Stmt const& stmt) const 
        -> typename std::enable_if<HasBookmarks>::type
    {
        VarBookmark::bind(stmt, binding_offset);
        if (offsets_.size() != 1)
            rebind<false>(stmt);
    }

    struct Rebind
    {
        template<typename Ix>
        void operator()(Ix) const { owner->rebind<Ix::value>(stmt); }
        DynamicBindings<Sequence, WithBookmarks> const* owner;
        handle::adapter::Stmt const& stmt;
    };

    template<bool HasBookmarks> auto rebind(handle::adapter::Stmt const& stmt) const
        -> typename std::enable_if<!HasBookmarks>::type
    {
        if (!offsets_.empty())
        {
            //boost::mp11::mp_for_each<boost::mp11::mp_iota_c<boost::fusion::result_of::size<Sequence>::value>>(Rebind{ this, stmt });
            rebind_<0>(stmt);
            odbcx::handle::set_attribute(stmt, SQL_ATTR_ROW_BIND_TYPE, offsets_.back() * sizeof(SQLLEN));
        }
    }

//    template<std::size_t N>
//    auto rebind(handle::adapter::Stmt const& stmt) const
//    {
//        auto begin = (member_offset<N>() + binding_offset()) * sizeof(SQLLEN);
//        auto next = member_offset<N + 1>();
//        auto end = (next + binding_offset()) * sizeof(SQLLEN);
//        using Type = typename Adapter<typename boost::fusion::result_of::value_at_c<Sequence, N>::type>::type;
//        DynamicBind<Type>{}.rebind(stmt, N + 1, begin, end);
//        if (next != offsets_.back())
//            rebind_<N + 1>(stmt);
//    }
///////////////////////////////////////////////////
    template<std::size_t N>
    auto rebind_(handle::adapter::Stmt const& stmt) const -> typename std::enable_if<boost::fusion::result_of::size<Sequence>::value != N>::type
    {
        assert(N + 1 < offsets_.size());
        auto begin = member_offset<N>();
        auto end = member_offset<N + 1>();
        using Type = typename boost::fusion::result_of::value_at_c<Sequence, N>::type;
        DynamicBind<Type>{}.rebind(stmt, N + 1, begin, end);
        if (end != offsets_.back())
            rebind_<N+1>(stmt);
    }

    template<std::size_t N>
    auto rebind_(handle::adapter::Stmt const& /*stmt*/) const noexcept-> typename std::enable_if<boost::fusion::result_of::size<Sequence>::value == N>::type {}

    template<std::size_t N>
    auto construct(handle::adapter::Stmt const& stmt, SQLLEN const* row) const 
        -> typename boost::fusion::result_of::value_at_c<Sequence, N>::type
    {
        using Type = typename boost::fusion::result_of::value_at_c<Sequence, N>::type;
        if (offsets_.size() > N)
        {
            auto begin = row + member_offset<N>();
            auto end = row + member_offset<N+1>();
            return cast<Type>(DynamicBind<Type>{}.instantiate(begin, end));
        }
        return cast<Type>(DynamicBind<Type>{}.instantiate(stmt, SQLUSMALLINT(N + 1)));
    }

    template<typename ...Args>
    auto make_sequence(Args&&...args) const
        -> typename std::enable_if<!std::is_constructible<Sequence, Args...>::value, Sequence>::type
    {
        return { std::forward<Args>(args)... };
    }

    template<typename ...Args>
    auto make_sequence(Args&&...args) const
        -> typename std::enable_if<std::is_constructible<Sequence, Args...>::value, Sequence>::type
    {
        return Sequence(std::forward<Args>(args)...);
    }

    template<typename ...Args>
    auto make(handle::adapter::Stmt const& stmt, SQLLEN const* row, Args&&...args) const
        -> typename std::enable_if<sizeof...(Args)!= boost::fusion::result_of::size<Sequence>::value, Sequence>::type
    {
        return make(stmt, row, std::forward<Args>(args)..., construct<sizeof...(Args)>(stmt, row));
    }

    template<typename ...Args>
    auto make(handle::adapter::Stmt const& /*stmt*/, SQLLEN const* /*row*/, Args&&...args) const 
        -> typename std::enable_if<sizeof...(Args) == boost::fusion::result_of::size<Sequence>::value, Sequence>::type
    {
        return make_sequence(std::forward<Args>(args)...);
    }

    //template<std::size_t ...Ixs>
    //Sequence make(boost::mp11::index_sequence<Ixs...>, handle::adapter::Stmt const& stmt, SQLLEN const* row) const
    //{
    //    return make_sequence( construct<Ixs>(stmt, row)... );
    //}

    template<std::size_t N>
    auto initialize(Sequence& value, handle::adapter::Stmt const& stmt, SQLLEN const* row) const
        -> typename std::enable_if<!std::is_array<typename boost::fusion::result_of::value_at_c<Sequence, N>::type>::value>::type
    {
        boost::fusion::at_c<N>(value) = construct<N>(stmt, row);
    }

    template<std::size_t N>
    auto initialize(Sequence& value, handle::adapter::Stmt const& stmt, SQLLEN const* row) const
        -> typename std::enable_if<std::is_array<typename boost::fusion::result_of::value_at_c<Sequence, N>::type>::value>::type
    {
        using Type = typename boost::fusion::result_of::value_at_c<Sequence, N>::type;
        if (offsets_.size() > N)
        {
            auto begin = row + member_offset<N>();
            auto end = row + member_offset<N + 1>();
            copy2array(boost::fusion::at_c<N>(value), DynamicBind<Type>{}.instantiate(begin, end));
        }
        else
            copy2array(boost::fusion::at_c<N>(value), DynamicBind<Type>{}.instantiate(stmt, SQLUSMALLINT(N + 1)));
    }

    template<typename T>
    auto value(handle::adapter::Stmt const& stmt, SQLLEN const* row) const
        -> typename std::enable_if<HasArrays<T>::value, Sequence>::type
    {
        Sequence res;
        boost::mp11::mp_for_each<boost::mp11::mp_iota_c<boost::fusion::result_of::size<Sequence>::value>>(Initialize{ this, res, stmt, row });
        return res;
    }

    template<typename T>
    auto value(handle::adapter::Stmt const& stmt, SQLLEN const* row) const
        -> typename std::enable_if<!HasArrays<T>::value, Sequence>::type
    {
        return make(/*boost::mp11::make_integer_sequence<std::size_t, boost::fusion::result_of::size<Sequence>::value>{},*/ stmt, row);
    }

    template<typename...Args> static void call(Args&&...) noexcept {}

    template<std::size_t ...Ixs, typename ...Args>
    void assign(boost::mp11::index_sequence<Ixs...>, SQLLEN* row, Args&& ...args) const
    {
        call((assign<Ixs>(row, args),0)...);
    }

    template<std::size_t N, typename T>
    void assign(SQLLEN* row, T &&value)  const
    {
        auto begin = row + member_offset<N>();
        auto end = row + member_offset<N + 1>();
        using Type = typename boost::fusion::result_of::value_at_c<Sequence, N>::type;
        DynamicBind<Type>{}.instantiate(begin, end) = std::forward<T>(value);
    }

    template<std::size_t N>
    void assign(SQLLEN* row, Sequence const& value) const { assign<N>(row, boost::fusion::at_c<N>(value)); }

    template<std::size_t N> constexpr std::size_t member_offset() const noexcept
    {
        using Base = typename std::conditional<WithBookmarks, std::integral_constant<std::size_t, 1>, std::integral_constant<std::size_t, 0>>::type;
        return member_offset_<Base::value + N>();
    }

    template<std::size_t N> constexpr auto member_offset_() const noexcept 
        -> typename std::enable_if<N != 0, std::size_t>::type
    { 
        static_assert(N != 0, "Oops! member_offset_<0>() specialization has been removed!"); 
        return offsets_[N - 1]; 
    }
    template<std::size_t N> constexpr auto member_offset_() const noexcept -> typename std::enable_if<N == 0, std::size_t>::type { return 0; }
private:
    Offsets offsets_;
};


template<typename Sequence>
struct NameGenerator
{
	NameGenerator(diversion::string_view alias) : alias_{ std::move(alias) } {}

	static_assert(boost::fusion::traits::is_sequence<Sequence>::value, "fusion sequence expected");
	template<typename T>
	std::string operator()(std::string const& str, const T&) const
	{
		auto res = str;
		res += ',';
		res.append(alias_.data(), alias_.length());
		res += '.';
		res += boost::fusion::extension::struct_member_name<Sequence, T::value>::call();
		return res;
	}
private:
    diversion::string_view alias_;
};

}  /*namespace columns*/}  /*namespace details*/ }/*inline namespace v1*/} /*namespace odbcx*/

