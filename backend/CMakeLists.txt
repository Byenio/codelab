file(GLOB_RECURSE SRC_FILES "src/*.cc")

add_executable(codelab_server ${SRC_FILES})

target_include_directories(codelab_server PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(codelab_server PRIVATE
        Crow::Crow
        nlohmann_json::nlohmann_json
        unofficial::sqlite3::sqlite3
        libgit2::libgit2package
)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/db/schema.sql
        DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/db)