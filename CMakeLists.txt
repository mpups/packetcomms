cmake_minimum_required(VERSION 3.20)

project(packetcomms)

find_package(Boost COMPONENTS unit_test_framework REQUIRED)

# Cereal (header only library):
include_directories(external/cereal/include)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
add_compile_options(-Wall -Wextra -Werror)
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

file(GLOB_RECURSE SRC ${PROJECT_SOURCE_DIR}/src/*.hpp ${PROJECT_SOURCE_DIR}/src/*.cpp)
add_library(packetcomms ${SRC})
target_link_libraries(packetcomms pthread)

add_executable(test tests/test.cpp)
target_link_libraries(test packetcomms ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})
add_test(test_packetcomms test)
