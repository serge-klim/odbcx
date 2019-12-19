// Copyright (c) 2018-2019 Serge Klimov serge.klim@outlook.com

#pragma once
#include "odbcx/handle.hpp"

namespace odbcx { inline namespace v1 { namespace details{

class alignas(SQLLEN) VarBookmark
{
public:
    constexpr bool valid() const noexcept { return indicator_ > 0; }
    constexpr bool operator!() const noexcept { return !valid(); }
    static std::size_t bind(handle::adapter::Stmt const& stmt, VarBookmark* bookmark)
    {
        return bind(stmt, reinterpret_cast<std::size_t>(bookmark));
    }
    static std::size_t bind(handle::adapter::Stmt const& stmt, std::size_t offset)
    {
        static_assert(std::is_standard_layout<VarBookmark>::value, "Oops! below is not going to work:(");
        static_assert(alignof(VarBookmark) == alignof(SQLLEN), "Oops! it's broken:(");
        auto value_offset = offset + offsetof(VarBookmark, value_);
        call(&SQLBindCol, stmt, 0, SQL_C_VARBOOKMARK, reinterpret_cast<SQLPOINTER>(value_offset), sizeof(VarBookmark::value_), reinterpret_cast<SQLLEN*>(offset));
        return value_offset + sizeof(VarBookmark::value_);
    }
private:
    SQLLEN indicator_;
    char value_[10];
};

} /*namespace details*/ }/*inline namespace v1*/} //namespace odbcx
