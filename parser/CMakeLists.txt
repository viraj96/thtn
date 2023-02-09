cmake_minimum_required(VERSION 3.16)
project(thtn_parser)

set(CMAKE_CXX_STANDARD 17)
add_compile_options(-Wall -Wextra -g)

FIND_PACKAGE(FLEX REQUIRED)
FIND_PACKAGE(BISON REQUIRED)

IF (BISON_FOUND AND FLEX_FOUND)
    MESSAGE(STATUS "[OK] Bison and Flex were found. Parser will be built shortly!")
ELSE (BISON_FOUND AND FLEX_FOUND)
    MESSAGE(STATUS "[ERROR] Could not find Bison or Flex. Parser could not be built!")
ENDIF(BISON_FOUND AND FLEX_FOUND)

BISON_TARGET(thtnParser hddl.y ${CMAKE_CURRENT_BINARY_DIR}/hddl.cpp
    DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/hddl_header.hpp)
FLEX_TARGET(thtnScanner hddl-token.l ${CMAKE_CURRENT_BINARY_DIR}/hddl-token.cpp)
ADD_FLEX_BISON_DEPENDENCY(thtnScanner thtnParser)

include_directories("include")
include_directories(${CMAKE_CURRENT_BINARY_DIR})

file(GLOB SOURCES "src/*.cpp")
add_executable(miniParser
    ${SOURCES}
    ${BISON_thtnParser_OUTPUTS}
    ${FLEX_thtnScanner_OUTPUTS}
    )
target_link_libraries(miniParser ${FLEX_LIBRARIES})
