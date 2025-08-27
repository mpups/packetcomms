cmake_minimum_required(VERSION 3.16)

project(packetcomms)

find_package(Boost COMPONENTS unit_test_framework REQUIRED)

# Cereal (header only library):
include_directories(external/cereal/include)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
if (MSVC)
    add_compile_options(/Wall)
else()
    add_compile_options(-Wall -Wextra -Werror)
    set(CMAKE_CXX_FLAGS_DEBUG "-g")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3")
endif()


# Set platform-specific networking libraries
if(WIN32)
    set(NETWORKING_LIBS ws2_32 wsock32)
else()
    set(NETWORKING_LIBS pthread)
endif()

file(GLOB_RECURSE SRC ${PROJECT_SOURCE_DIR}/src/*.hpp ${PROJECT_SOURCE_DIR}/src/*.cpp)
add_library(packetcomms STATIC ${SRC})
target_link_libraries(packetcomms ${NETWORKING_LIBS})

set(PACKETCOMMS_LIBRARIES packetcomms PARENT_SCOPE)
set(PACKETCOMMS_INCLUDES ${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR}/external/cereal/include PARENT_SCOPE)

add_executable(test tests/test.cpp)
target_link_libraries(test packetcomms ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} ${NETWORKING_LIBS})
add_test(test_packetcomms test)
