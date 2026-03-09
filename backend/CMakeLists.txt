cmake_minimum_required(VERSION 3.24)

# --- CORE LOGIC LIBRARY ---
file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc")

list(FILTER SRC_FILES EXCLUDE REGEX "src/main.cc")

add_library(codelab_lib STATIC ${SRC_FILES})

target_include_directories(codelab_lib PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(codelab_lib PUBLIC
        Crow::Crow
        nlohmann_json::nlohmann_json
        unofficial::sqlite3::sqlite3
        libgit2::libgit2package
        jwt-cpp::jwt-cpp
        unofficial-sodium::sodium
        reproc++
)

# --- SERVER EXECUTABLE ---
add_executable(codelab_server src/main.cc)
target_link_libraries(codelab_server PRIVATE codelab_lib)

# --- TESTS ---
enable_testing()
find_package(GTest CONFIG REQUIRED)

add_executable(codelab_tests
        tests/test_main.cc
        tests/dao_tests.cc
        tests/git_tests.cc
        tests/service_tests.cc
        tests/auth_tests.cc
        tests/api_tests.cc
)

target_include_directories(codelab_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(codelab_tests PRIVATE codelab_lib GTest::gtest cpr::cpr)

include(GoogleTest)
gtest_discover_tests(codelab_tests)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/db/schema.sql
        DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/db)