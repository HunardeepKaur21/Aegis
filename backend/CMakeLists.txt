cmake_minimum_required(VERSION 3.16)
project(aegis_backend LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

find_package(SQLite3 QUIET)
if(NOT TARGET SQLite::SQLite3)
    message(STATUS "Using bundled SQLite3 amalgamation")
    add_library(sqlite3_bundled STATIC third_party/sqlite3/sqlite3.c)
    target_include_directories(sqlite3_bundled PUBLIC third_party/sqlite3)
    add_library(SQLite::SQLite3 ALIAS sqlite3_bundled)
    set(SQLite3_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/third_party/sqlite3)
endif()

find_package(Threads REQUIRED)

include(FetchContent)
# Standalone Asio headers required by Crow
FetchContent_Declare(asio
    GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
    GIT_TAG asio-1-30-2
    GIT_SHALLOW TRUE
)
FetchContent_GetProperties(asio)
if(NOT asio_POPULATED)
    FetchContent_Populate(asio)
    set(ASIO_INCLUDE_DIR "${asio_SOURCE_DIR}/asio/include" CACHE PATH "Asio include dir" FORCE)
    include_directories("${asio_SOURCE_DIR}/asio/include")
endif()

# Crow (REST framework)
set(CROW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(CROW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(CROW_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_Declare(crow GIT_REPOSITORY https://github.com/CrowCpp/Crow.git GIT_TAG v1.2.0)
FetchContent_MakeAvailable(crow)

# DSA engine and core code (one static library so each module can be unit-tested alone)
file(GLOB_RECURSE AEGIS_SOURCES CONFIGURE_DEPENDS src/core/*.cpp src/graph/*.cpp
     src/rescue/*.cpp src/resource/*.cpp src/shelter/*.cpp src/lab/*.cpp)
add_library(aegis_core STATIC ${AEGIS_SOURCES})
target_include_directories(aegis_core PUBLIC include ${SQLite3_INCLUDE_DIRS} "${asio_SOURCE_DIR}/asio/include")
target_link_libraries(aegis_core PUBLIC SQLite::SQLite3 Crow::Crow)

file(GLOB AEGIS_API_SOURCES CONFIGURE_DEPENDS src/api/*.cpp)
add_executable(aegis_server src/main.cpp ${AEGIS_API_SOURCES})
target_link_libraries(aegis_server PRIVATE aegis_core Crow::Crow Threads::Threads)
if(WIN32)
    target_link_libraries(aegis_server PRIVATE ws2_32 wsock32)
endif()

enable_testing()
# One test executable per module, registered with ctest.
add_executable(test_graph tests/test_graph.cpp)
target_link_libraries(test_graph PRIVATE aegis_core)
add_test(NAME graph COMMAND test_graph)

add_executable(test_rescue tests/test_rescue.cpp)
target_link_libraries(test_rescue PRIVATE aegis_core)
add_test(NAME rescue COMMAND test_rescue)

add_executable(test_resource tests/test_resource.cpp)
target_link_libraries(test_resource PRIVATE aegis_core)
add_test(NAME resource COMMAND test_resource)

add_executable(test_shelter tests/test_shelter.cpp)
target_link_libraries(test_shelter PRIVATE aegis_core)
add_test(NAME shelter COMMAND test_shelter)

add_executable(test_lab tests/test_lab.cpp)
target_link_libraries(test_lab PRIVATE aegis_core)
add_test(NAME lab COMMAND test_lab)



