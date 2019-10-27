cmake_minimum_required(VERSION 3.1)

set(Boost_USE_STATIC_LIBS ON)
find_package( Boost COMPONENTS program_options unit_test_framework REQUIRED )

add_executable (test
		dd.hpp
		tests.hpp
		tests.cpp
		bindings_test.cpp
		select_statments_test.cpp
		simple_queries_test.cpp
		transactions_test.cpp
		ttraits_tests.cpp
		iterator_test.cpp
		select_optional_test.cpp
)

target_link_libraries(test
        odbcx
        Boost::boost
        Boost::program_options
		Boost::unit_test_framework
)

