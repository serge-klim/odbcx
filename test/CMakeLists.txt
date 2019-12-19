cmake_minimum_required(VERSION 3.1)

set(Boost_USE_STATIC_LIBS ON)
find_package( Boost COMPONENTS program_options filesystem unit_test_framework REQUIRED )

add_executable (test
        test.hpp
        test.cpp
        connections.hpp
        connections.cpp
        dd.hpp
        blob_test.cpp
        flyweight_test.cpp
        handle_test.cpp
        modifiable_cursor_test.cpp
        select_statments_test.cpp
        simple_queries_test.cpp
        surrogate_test.cpp
        transactions_test.cpp
)

target_link_libraries(test
        odbcx
        Boost::boost
        Boost::filesystem
        Boost::program_options
		Boost::unit_test_framework
)

