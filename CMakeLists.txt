cmake_minimum_required(VERSION 3.13)
project(SubnetCalculator)

set(CMAKE_CXX_STANDARD 17)

find_package(Boost REQUIRED)

add_executable(SubnetCalculator main.cpp)

target_include_directories(SubnetCalculator PUBLIC ${BOOST_SOURCE_DIRS})
link_libraries(SubnetCalculator ${BOOST_INCLUDE_DIRS})
