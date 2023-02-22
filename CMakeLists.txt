cmake_minimum_required(VERSION 3.16)
project(thtn)

set(CMAKE_CXX_STANDARD 17)
add_compile_options(-Wall -Wextra -g)

FIND_PACKAGE(FLEX 2.6.4 REQUIRED)
FIND_PACKAGE(BISON 3.8.2 REQUIRED)
FIND_PACKAGE(nlohmann_json 3.2.0 REQUIRED)
FIND_PACKAGE(plog)

if (UNIX AND NOT APPLE)
    set(LINUX TRUE)
endif(UNIX AND NOT APPLE)

IF (BISON_FOUND AND FLEX_FOUND)
    MESSAGE(STATUS "[OK] Bison and Flex were found. Parser will be built shortly!")
ELSE (BISON_FOUND AND FLEX_FOUND)
    MESSAGE(STATUS "[ERROR] Could not find Bison or Flex. Parser could not be built!")
ENDIF(BISON_FOUND AND FLEX_FOUND)

BISON_TARGET(thtnParser ${CMAKE_CURRENT_SOURCE_DIR}/parser/hddl.y ${CMAKE_CURRENT_BINARY_DIR}/hddl.cpp
    DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/hddl_header.hpp)
FLEX_TARGET(thtnScanner ${CMAKE_CURRENT_SOURCE_DIR}/parser/hddl-token.l ${CMAKE_CURRENT_BINARY_DIR}/hddl-token.cpp)
ADD_FLEX_BISON_DEPENDENCY(thtnScanner thtnParser)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/parser/include
    ${CMAKE_CURRENT_SOURCE_DIR}/planner/include
    ${CMAKE_CURRENT_BINARY_DIR})

file(GLOB SOURCES "planner/src/*.cpp" "parser/src/*.cpp")
list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/parser/src/main.cpp)

add_executable(planner
    ${SOURCES}
    ${BISON_thtnParser_OUTPUTS}
    ${FLEX_thtnScanner_OUTPUTS}
    )
if (LINUX)
    target_link_libraries(planner ${FLEX_LIBRARIES} nlohmann_json::nlohmann_json plog::plog)
else(NOT LINUX)
    target_link_libraries(planner nlohmann_json::nlohmann_json plog::plog)
endif()
