cmake_minimum_required(VERSION 3.1)

find_package(ODBC)
find_package(Boost REQUIRED )

add_library (odbcx
        adapter.hpp
		attribute.hpp
        cursor.hpp
        handle.hpp
        type_info.hpp
        transaction.hpp
        transaction.cpp
        query.hpp
        utility.hpp
        utility.cpp
        details/attribute.hpp
        details/bookmark.hpp
        details/cast.hpp
        details/cursor.hpp
		details/diversion.hpp
        details/handle.hpp   
        details/iterator.hpp
        details/query.hpp
        bindings/adapter.hpp
        bindings/ttraits.hpp
        bindings/in.hpp
        bindings/columns.hpp
        bindings/surrogate.hpp
        bindings/flyweight.hpp
        bindings/surrogate.hpp
)

target_link_libraries(odbcx
		PUBLIC ODBC::ODBC
        PRIVATE Boost::boost
)

target_include_directories (odbcx PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
