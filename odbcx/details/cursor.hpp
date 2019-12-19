// Copyright (c) 2018-2019 Serge Klimov serge.klim@outlook.com

#pragma once
#include "iterator.hpp"
#include "odbcx/bindings/columns.hpp"
#include "odbcx/attribute.hpp"
#include "odbcx/handle.hpp"
#include "bookmark.hpp"
#include <boost/range/iterator_range.hpp>
#include <boost/fusion/include/size.hpp>
#include <boost/fusion/include/value_at.hpp>
#include <boost/fusion/include/at_c.hpp>
#include <boost/fusion/include/is_sequence.hpp>
#include <boost/mp11/function.hpp>
#include <vector>
#include <algorithm>
#include <memory>

namespace odbcx { inline namespace v1 { namespace details {

template<typename Attributes = attribute::None>
using HasBookmarks = attribute::Contains<Attributes, decltype(attribute::bookmarks)>;

template<typename Attributes = attribute::None>
using IsForwardOnlyCursor =  boost::mp11::mp_or<
                                        attribute::Contains<Attributes, decltype(attribute::cursor_forward_only)>,
                                        attribute::Contains < Attributes, decltype(odbcx::attribute::nonscrollable)>,
                                        boost::mp11::mp_and<
                                            boost::mp11::mp_not<attribute::Contains<Attributes, decltype(odbcx::attribute::scrollable)>>,
                                            boost::mp11::mp_not<attribute::Contains<Attributes, decltype(odbcx::attribute::cursor_static)>>,
                                            boost::mp11::mp_not<attribute::Contains<Attributes, decltype(odbcx::attribute::cursor_keyset_driven)>>,
                                            boost::mp11::mp_not<attribute::Contains<Attributes, decltype(odbcx::attribute::cursor_dynamic)>>
                                            >
                                        >;
//https://msdn.microsoft.com/en-us/data/ms712466(v=vs.95)
template<typename Attributes = attribute::None>
using IsReadOnlyCursor = boost::mp11::mp_or<
                                            attribute::Contains < Attributes, decltype(odbcx::attribute::concur_read_only)>,
                                            IsForwardOnlyCursor<Attributes>
                                        >;


template<typename T> struct BufferSize { constexpr std::size_t operator()(std::size_t) const noexcept { return 100; } };
template<std::size_t N> struct BufferSize<odbcx::attribute::synthetic::value::BufferSizeHintRecords<N>> { constexpr std::size_t operator()(std::size_t) const noexcept { return N; } };
template<std::size_t N> struct BufferSize<odbcx::attribute::synthetic::value::BufferSizeHintBytes<N>> 
{ 
    constexpr std::size_t operator()(std::size_t row_size) const noexcept 
    { 
        return (std::max)(row_size/N, std::size_t(1)); 
    } 
};

template<typename Sequence, typename = std::true_type>
struct FetchOneRes
{
    static_assert(boost::fusion::result_of::size<Sequence>::value != 1, "Oops!");
    using type = diversion::optional<Sequence>;
    static type value(Sequence&& sequence) { return diversion::make_optional(std::forward<Sequence>(sequence)); }
};

template<typename Sequence>
struct FetchOneRes<Sequence, std::integral_constant<bool,boost::fusion::result_of::size<Sequence>::value == 1>>
{
    static_assert(boost::fusion::result_of::size<Sequence>::value == 1, "Oops!");
    using type = diversion::optional<typename boost::fusion::result_of::value_at_c<Sequence, 0>::type>;
    static type value(Sequence&& sequence) { return diversion::make_optional(boost::fusion::at_c<0>(sequence)); }
};


template<typename Attributes = attribute::None>
class UtilityCursor
{
    using HandleAttributes = attribute::Remove<Attributes, attribute::synthetic::Id<attribute::synthetic::id::SharedHandle>>;
    static_assert(!attribute::Contains<HandleAttributes, attribute::synthetic::SharedHandle>::value, "Oops! seems attribute::Remove<Attributes, attribute::synthetic::Id<attribute::synthetic::id::SharedHandle>> is broken");
protected:
    using Stmt = typename std::conditional<std::is_same<HandleAttributes, Attributes>::value
                                                    , handle::Handle<SQL_HANDLE_STMT, HandleAttributes>
                                                    , handle::Adapter<SQL_HANDLE_STMT, HandleAttributes>>::type;
    UtilityCursor(Stmt&& stmt) : stmt_{ std::forward<Stmt>(stmt) } {}
    Stmt const& handle() const & { return stmt_; }
public:
    Stmt handle() && 
    { 
        call(&SQLFreeStmt, stmt_, SQL_CLOSE);
        call(&SQLFreeStmt, stmt_, SQL_UNBIND);
        call(&SQLFreeStmt, stmt_, SQL_RESET_PARAMS);
        return std::move(stmt_); 
    }
protected:
    std::size_t fetch2(void* buffer, size_t n, SQLSMALLINT orientation, SQLLEN offset )
    {
        SQLLEN row_offset = SQLLEN(buffer) - binding_offset____();
        handle::set_attribute(stmt_, SQL_ATTR_ROW_BIND_OFFSET_PTR, &row_offset);
        handle::set_attribute(stmt_, SQL_ATTR_ROW_ARRAY_SIZE, n);
        return fetch(orientation, offset);
    }

    std::size_t fetch(SQLSMALLINT orientation, SQLLEN offset = 0)
    {
        SQLULEN fetched = 0;
        handle::set_attribute(stmt_, SQL_ATTR_ROWS_FETCHED_PTR, &fetched);
//      https://support.oracle.com/knowledge/Oracle%20Database%20Products/1472987_1.html
//      auto statuses = std::vector<SQLUSMALLINT>(n);
//      SQLSetStmtAttr(stmt, SQL_ATTR_ROW_STATUS_PTR, statuses.data(), 0);
        //if (call(&SQLFetchScroll, stmt_, orientation, offset) == SQL_NO_DATA)
        //  fetched = 0;
        //statuses.resize(fetched);
        //assert(std::find_if(begin(statuses), end(statuses), [](auto status)
        //{
        //  return SQL_ROW_SUCCESS != status;
        //}) == end(statuses));
        //return statuses;
        return call(&SQLFetchScroll, stmt_, orientation, offset) == SQL_NO_DATA ? 0 : std::size_t{ fetched };
    }
    constexpr std::size_t buffer_size(std::size_t rec_size) const noexcept
    {
        return BufferSize<attribute::ValueOf<Attributes, attribute::synthetic::Id<attribute::synthetic::id::BufferSizeHint>>>{}(rec_size);
    }
    static constexpr SQLLEN binding_offset____() noexcept { return sizeof(SQLLEN); /* DynamicLayout<T>::binding_offset____();*/ }
private:
    Stmt stmt_;
};


template<typename Cursor, typename Sequence, typename Attributes = attribute::None>
class StaticallyBoundCursor : UtilityCursor<Attributes>
{
    static_assert(boost::fusion::traits::is_sequence<Sequence>::value, "fusion sequence expected");
    static_assert(details::columns::IsSequenceStaticallyBindable<Sequence>::value, "fusion sequence supposed to be statically bindable");
    using Base = UtilityCursor<Attributes>;
public:
    StaticallyBoundCursor(typename Base::Stmt&& stmt) : UtilityCursor<Attributes>{ std::forward<typename Base::Stmt>(stmt) } { Bindings::bind(Base::handle()); }
    using Base::handle;
protected:
    using Bindings = details::columns::StaticBindings<Sequence, HasBookmarks<Attributes>::value>;
    using Row = typename Bindings::Row;
    using Data = std::vector<Row>;
    class Buffer
    {
    public:
        using value_type = Sequence;
        using reference_type = Sequence const&;
        Buffer(Cursor* cursor, std::size_t low = 1, Data&& data = Data{}) : cursor_{ cursor }, low_{ low }, data_{ std::forward<Data>(data) } {}
        constexpr Cursor* cursor() const noexcept { return cursor_; }
        bool empty() const { return data_.empty(); }
        std::size_t low() const { return low_; }
        std::size_t high() const { return low() + size(); }
        std::size_t size() const { return data_.size(); }
        reference_type operator[](std::size_t i) const { assert(size() > i && "index is out of range"); return data_[i].value; }
        VarBookmark const& bookmark(std::size_t i) const { assert(size() > i && "index is out of range"); return data_[i].bookmark; }
    private:
        Cursor* cursor_;
        std::size_t low_;
        Data data_;
    };
    constexpr Bindings bindings() const noexcept { return {}; }

    Data fetch(SQLSMALLINT orientation = SQL_FETCH_NEXT, SQLLEN offset = 0)
    {
        auto n = Base::buffer_size(sizeof(Row));
        auto rows = Data(n);
        auto fetched = Base::fetch2(rows.data(), rows.size(), orientation, offset);
        rows.resize(fetched);
        return rows;
    }
public:
    auto fetch_one(SQLSMALLINT orientation = SQL_FETCH_NEXT, SQLLEN offset = 0)
        -> typename FetchOneRes<Sequence>::type
    {
        Row row;
        auto fetched = Base::fetch2(&row/* just one in this case row*/, 1, orientation, offset);
        return fetched == 0 ? diversion::nullopt : FetchOneRes<Sequence>::value(std::move(row.value));
    }
};

template<typename Cursor, typename Sequence, typename Attributes = attribute::None>
class DynamicallyBoundCursor : UtilityCursor<Attributes>
{
    using Base = UtilityCursor<Attributes>;
public:
    DynamicallyBoundCursor(typename Base::Stmt&& stmt) : UtilityCursor<Attributes>{ std::forward<typename Base::Stmt>(stmt) } , bindings_{ Bindings::bind(Base::handle()) } {}
    using Base::handle;
protected:
    using Bindings = details::columns::DynamicBindings<Sequence, HasBookmarks<Attributes>::value>;
    using Data = std::vector<SQLLEN>;
    class Buffer
    {
    public:
        using value_type = Sequence;
        using reference_type = Sequence;
        Buffer(Cursor* cursor, std::size_t low = 1, Data&& data = Data{}) : cursor_{ cursor }, low_{ low }, data_{ std::forward<Data>(data) } {}
        constexpr Cursor* cursor() const noexcept { return cursor_; }
        bool empty() const { return data_.empty(); }
        std::size_t low() const { return low_; }
        std::size_t high() const { return low() + size(); }
        std::size_t size() const { return data_.size() / row_size(); }
        reference_type operator[](std::size_t i) const
        {
            assert(size() > i && "index is out of range");
            auto const& bindings = cursor()->bindings();
            auto row = data_.data() + bindings.row_size() * i;
            return bindings.value(cursor()->handle(), row);
        }
        VarBookmark const& bookmark(std::size_t i) const 
        { 
            assert(size() > i && "index is out of range"); 
            auto const& bindings = cursor()->bindings();
            auto row = data_.data() + bindings.row_size() * i;
            return bindings.bookmark(row);
        }
    private:
        std::size_t row_size() const { return cursor()->bindings().row_size(); }
    private:
        Cursor* cursor_;
        std::size_t low_;
        Data data_;
    };
    constexpr Bindings const& bindings() const noexcept { return bindings_; }
    Data fetch(SQLSMALLINT orientation = SQL_FETCH_NEXT, SQLLEN offset = 0)
    {
        auto row_size = bindings_.row_size();
        auto n = bindings_.bulk_fetch() ? Base::buffer_size(row_size * sizeof(SQLLEN)) : 1;
        auto buffer = Data(n * row_size );
        assert(!buffer.empty() && "binding empty buffer");
        auto fetched = Base::fetch2(buffer.data(), n, orientation, offset);
        buffer.resize(fetched* row_size);
        return buffer;
    }
public:
    auto fetch_one(SQLSMALLINT orientation = SQL_FETCH_NEXT, SQLLEN offset = 0)
        -> typename FetchOneRes<Sequence>::type
    {
        auto buffer = Data(bindings_.row_size());
        assert(!buffer.empty() && "binding empty buffer");
        auto fetched = Base::fetch2(buffer.data(), 1, orientation, offset);
        return fetched == 0 ? diversion::nullopt : FetchOneRes<Sequence>::value(bindings().value(handle(), buffer.data()));
    }
private:
    Bindings bindings_;
};

template<typename Cursor, typename Sequence, typename Attributes = attribute::None>
using CursorBase = typename std::conditional<details::columns::IsSequenceStaticallyBindable<Sequence>::value, 
                                                        details::StaticallyBoundCursor<Cursor,Sequence, Attributes>,
                                                        details::DynamicallyBoundCursor<Cursor, Sequence,Attributes> >::type;


template<typename Sequence, typename Attributes = attribute::None>
class ForwardOnlyCursor : public CursorBase<ForwardOnlyCursor<Sequence, Attributes>, Sequence, Attributes>
{
    friend ForwardIterator<ForwardOnlyCursor<Sequence, Attributes>, boost::single_pass_traversal_tag>;
    using Base = CursorBase<ForwardOnlyCursor<Sequence, Attributes>, Sequence, Attributes>;
    class Buffer : public Base::Buffer
    {
    protected:
        using Base::Buffer::cursor;
    public:
        //using Base::Buffer::Buffer;
        Buffer(ForwardOnlyCursor* cursor, std::size_t low = 1, typename Base::Data&& data = typename Base::Data{})
            : Base::Buffer{ cursor, low, std::forward<typename Base::Data>(data) }{}
        ~Buffer()
        {
            if (!!next_)
            {
                assert(cursor()->buffer_.expired() || cursor()->buffer_.lock().get() == this);
                cursor()->buffer_ = next_;
            }
        }
        using Base::Buffer::low;
        using Base::Buffer::high;
        std::shared_ptr<Buffer> next() const
        {
            if (!next_)
            {
                auto data = cursor()->Base::fetch(SQL_FETCH_NEXT);
                if (!data.empty())
                    next_ = std::make_shared<Buffer>(cursor(), high(), std::move(data));
            }
            return next_;
        }
    private:
        mutable std::shared_ptr<Buffer> next_;
    };
    friend Buffer;
public:
    using value_type = typename Buffer::value_type;
    using reference_type = typename Buffer::reference_type;
    using CursorBase<ForwardOnlyCursor<Sequence, Attributes>, Sequence, Attributes>::CursorBase;
    using const_iterator = ForwardIterator<ForwardOnlyCursor<Sequence, Attributes>, boost::single_pass_traversal_tag>;
    boost::iterator_range<const_iterator> fetch()  
    {
        auto buffer = fetch_buffer();
        return { !buffer ? const_iterator{} : const_iterator{1, std::move(buffer)}, const_iterator{} };
    }
protected:
    std::shared_ptr<Buffer> fetch_buffer()
    {
        auto res = buffer_.lock();
        if (!res)
            res = Buffer{ this }.next();
        assert(buffer_.lock() == res);
        return res;
    }
private:
    std::weak_ptr<Buffer> buffer_;
};

template<typename Sequence, typename Attributes = attribute::None>
class ScrollableCursor : public CursorBase<ScrollableCursor<Sequence, Attributes>, Sequence, Attributes>
{
    static_assert(!IsForwardOnlyCursor<Attributes>::value, "invalid attributes");
    friend ForwardIterator<ScrollableCursor<Sequence, Attributes>, boost::forward_traversal_tag>;
    using Base = CursorBase<ScrollableCursor<Sequence, Attributes>, Sequence, Attributes>;
    class Buffer : public Base::Buffer
    {
        using Base::Buffer::cursor;
    public:
        using Base::Buffer::low;
        using Base::Buffer::high;

//        using Base::Buffer::Buffer;
        Buffer(ScrollableCursor* cursor, std::size_t low = 1, typename Base::Data&& data = typename Base::Data{})
            : Base::Buffer{ cursor, low, std::forward<typename Base::Data>(data) }{}

        std::shared_ptr<Buffer> next() const
        {
           return cursor()->fetch_buffer(high());
        }
//        std::shared_ptr<Buffer> prev() const
//        {
//            assert(low() != 0);
//            if (low() == 1)
//                return {};
//#pragma message("!!!!!!!!!!parametrize buffer size!!!!!!!!!!!!!!!")
//            auto size = BufferSize + 1;
//            auto from = low() > size ? low() - size : 0;
//            return cursor()->fetch_buffer(from + 1);
//        }
    };
    friend Buffer;
public:
    using value_type = typename Buffer::value_type;
    using reference_type = typename Buffer::reference_type;
#pragma message("!!!!!!!!!!assert on statement attribute to make sure that cursor is scrollable!!!!!!!!!!!!!!!")
    using CursorBase<ScrollableCursor<Sequence, Attributes>, Sequence, Attributes>::CursorBase;
    using const_iterator = ForwardIterator<ScrollableCursor<Sequence, Attributes>, boost::forward_traversal_tag>;
    boost::iterator_range<const_iterator> fetch()
    {
        auto buffer = fetch_buffer();
        return { !buffer ? const_iterator{} : const_iterator{1, std::move(buffer)}, const_iterator{} };
    }
protected:
    VarBookmark const& bookmark(const_iterator const& i) const { return i.bookmark(); }
    std::shared_ptr<Buffer> fetch_buffer(std::size_t pos = 1)
    {
        auto data = pos == pos_
                    ? Base::fetch(SQL_FETCH_NEXT)
                    : Base::fetch(SQL_FETCH_ABSOLUTE, SQLLEN(pos));
        auto res = std::shared_ptr<Buffer>{};
        if(!data.empty())
        {
            res = std::make_shared<Buffer>(this, pos, std::move(data));
            pos_ = pos + res->size();
        }
        return res;
    }
private:
    std::size_t pos_ = 1;
};

template<typename Sequence, typename Attributes = attribute::None>
class ModifiableCursor : public ScrollableCursor<Sequence, Attributes>
{
    static_assert(!IsReadOnlyCursor<Attributes>::value, "invalid attributes");
    using Base = ScrollableCursor<Sequence, Attributes>;
public:
   // ~ModifiableCursor() { apply_changes(); }
    using ScrollableCursor<Sequence, Attributes>::ScrollableCursor;
    using iterator = typename Base::const_iterator;
    void /*iterator*/ remove(iterator const& i)
    {
        auto bookmark = Base::bookmark(i);
        assert(bookmark.valid() && "mysql dosn't fetch bookmarks by defult");
        delete_.push_back(std::move(bookmark));
        //return ++i;
    }
    void insert(Sequence&& value)
    {
        Base::bindings().emplace(insert_, std::forward<Sequence>(value));
    }

    template<typename ...Args>
    auto insert(Args&&... args) 
        -> decltype(std::declval<typename Base::Bindings>().emplace(std::declval<typename Base::Data&>(), std::forward<Args>(args)...))
    {
        return Base::bindings().emplace(insert_, std::forward<Args>(args)...);
    }
    template<typename Iterator>
    auto insert(Iterator begin, Iterator end)
        -> decltype(std::declval<typename Base::Bindings>().emplace(std::declval<typename Base::Data&>(), std::declval<typename Iterator::value_type > ()))
    {
        for(;begin != end; ++begin)
            Base::bindings().emplace(insert_, *begin);
    }

    void apply_changes()
    {
        auto const& bindings = Base::bindings();
        auto const& stmt = Base::handle();
        SQLULEN fetched = 0;
        handle::set_attribute(stmt, SQL_ATTR_ROWS_FETCHED_PTR, &fetched);
        auto row_offset = SQLLEN(0);
        handle::set_attribute(stmt, SQL_ATTR_ROW_BIND_OFFSET_PTR, &row_offset);
        if (auto size = delete_.size())
        {
            call(&SQLFreeStmt, stmt, SQL_UNBIND);
            //call(&SQLFreeStmt, stmt, SQL_RESET_PARAMS);
            VarBookmark::bind(stmt, delete_.data());
            handle::set_attribute(stmt, SQL_ATTR_ROW_BIND_TYPE, sizeof(VarBookmark));
            assert(row_offset == 0);
            handle::set_attribute(stmt, SQL_ATTR_ROW_ARRAY_SIZE, size);
            call(&SQLBulkOperations, stmt, SQL_DELETE_BY_BOOKMARK);
            delete_ = decltype(delete_){};
            bindings.rebind(stmt);
        }
        if (!insert_.empty())
        {
            row_offset = SQLLEN(insert_.data()) - binding_offset;
            assert(insert_.size() % bindings.row_size() == 0);
            handle::set_attribute(stmt, SQL_ATTR_ROW_ARRAY_SIZE, insert_.size() / bindings.row_size());
            call(&SQLBulkOperations, stmt, SQL_ADD);
            insert_ = decltype(insert_){};
        }
    }

    void cancel_changes()
    {
        insert_.swap(decltype(insert_){});
        delete_.swap(decltype(delete_){});
    }
private:
    typename Base::Data insert_;
    std::vector<VarBookmark> delete_;
};


template<typename Sequence, typename Attributes = attribute::None>
using ScrollableCursorType = typename std::conditional<IsReadOnlyCursor<Attributes>::value
                                            , ScrollableCursor<Sequence, Attributes>
                                            , ModifiableCursor<Sequence, Attributes>
                                           >::type;


template<typename T> struct IsCursor : std::false_type {};
template<typename Sequence, typename Attributes> struct IsCursor<ForwardOnlyCursor<Sequence, Attributes>> : std::true_type {};
template<typename Sequence, typename Attributes> struct IsCursor<ScrollableCursor<Sequence, Attributes>> : std::true_type {};
template<typename Sequence, typename Attributes> struct IsCursor<ModifiableCursor<Sequence, Attributes>> : std::true_type {};

template<typename ...Params>
struct MakeSequence { using type = std::tuple<Params...>; };

template<typename T>
struct MakeSequence<T> { using type = typename std::conditional<boost::fusion::traits::is_sequence<T>::value, T, std::tuple<T>>::type; };

} /*namespace details*/} /*inline namespace v1*/} //namespace odbcx
