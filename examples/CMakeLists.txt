cmake_minimum_required(VERSION 3.1)

set(Boost_USE_STATIC_LIBS ON)
find_package( Boost COMPONENTS program_options unit_test_framework REQUIRED )

add_executable (select
		select.cpp
)

target_link_libraries(select
        odbcx
        Boost::boost
)

