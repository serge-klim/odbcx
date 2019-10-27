cmake_minimum_required(VERSION 3.1)

find_package(ODBC)
find_package(Boost REQUIRED )

add_library (odbcx
		iterator.hpp 
		odbcx.hpp 
		query.hpp 
		transaction.hpp 
		utility.hpp
		details/diversion.hpp
		details/statement.hpp
		bindings/in.hpp
		bindings/out.hpp
		bindings/ttraits.hpp
		utility.cpp
)

target_link_libraries(odbcx
		PUBLIC ODBC::ODBC
        PRIVATE Boost::boost
)

target_include_directories (odbcx PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
