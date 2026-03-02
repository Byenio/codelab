enable_testing()

file(GLOB_RECURSE TEST_SOURCES "*.cc")

add_executable(unit_tests ${TEST_SOURCES})

target_link_libraries(unit_tests PRIVATE
        gtest
        gtest_main
        Crow::Crow
        SQLite::SQLite3
        libgit2
)

target_include_directories(unit_tests PRIVATE ../src)

include(GoogleTest)
gtest_discover_tests(unit_tests)