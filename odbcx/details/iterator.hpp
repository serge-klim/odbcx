// Copyright (c) 2018-2019 Serge Klimov serge.klim@outlook.com

#pragma once
#include "diversion.hpp"
#include <boost/iterator/iterator_facade.hpp>
#include <memory>
#include <type_traits>
#include <cassert>

namespace odbcx { inline namespace v1 { namespace details {

template<typename Cursor, typename CategoryOrTraversal>
struct IteratorBase
{
    struct CachedValue
    {
        using reference_type = typename std::add_lvalue_reference<
                                                typename std::add_const<typename Cursor::value_type>::type
                                                >::type;

        void invalidate() { value_ = diversion::nullopt; }

        template<typename T>
        reference_type dereference(T& owner) const
        {
            if(!value_)
                value_ = owner.value();
            return *value_;
        }
    private:
        mutable diversion::optional<typename Cursor::value_type> value_;
    };

    struct Value
    {
        using reference_type = typename Cursor::value_type const&;
        void invalidate() {}
        template<typename T>
        reference_type dereference(T& owner) const
        {
            return owner.value();
        }

    };
    using type = typename std::conditional<std::is_lvalue_reference<typename Cursor::reference_type>::value, Value, CachedValue>::type;
};

template<typename Cursor, typename CategoryOrTraversal>
class ForwardIterator
	: public boost::iterator_facade<ForwardIterator<Cursor, CategoryOrTraversal>,
	                                    typename Cursor::value_type,
                                        CategoryOrTraversal,
                                        typename std::add_lvalue_reference<
                                                typename std::add_const<typename Cursor::value_type>::type
                                                >::type
                                    >
    , IteratorBase<Cursor, CategoryOrTraversal>::type
{
    using Base = typename IteratorBase<Cursor, CategoryOrTraversal>::type;
    using Buffer = typename Cursor::Buffer;
public:
    ForwardIterator() {}
    ForwardIterator(std::size_t i, std::shared_ptr<Buffer>&& buffer) : i_{ i - buffer->low() }, buffer_{ std::forward<std::shared_ptr<Buffer>>(buffer) } {}
	void increment() 
	{
		assert(!buffer_ == false);
        assert(i_ < buffer_->size());
        Base::invalidate();
		if (++i_ == buffer_->size())
		{
            buffer_ = buffer_->next();
            i_ = 0;
		}
	}

    typename Cursor::reference_type value() const
    {
        assert(!buffer_ == false);
        return buffer_->operator[](i_);
    }

    typename Base::reference_type dereference() const
	{
		return Base::dereference(*this);
	}

	bool equal(ForwardIterator const & other) const
	{
		return i_ == other.i_ && buffer_ == other.buffer_;
	}
private:
    friend Cursor;
    auto bookmark() const -> decltype(std::declval<Buffer>().bookmark(0)) 
    { 
        assert(!buffer_ == false);
        return buffer_->bookmark(i_); 
    }
private:
    std::size_t i_ = 0;
	std::shared_ptr<Buffer> buffer_;
};



} /*namespace details*/} /*inline namespace v1*/} //namespace odbcx

