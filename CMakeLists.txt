cmake_minimum_required(VERSION 3.24)
project(codelab VERSION 0.1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Crow CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(unofficial-sqlite3 CONFIG REQUIRED)
find_package(libgit2 CONFIG REQUIRED)
find_package(jwt-cpp CONFIG REQUIRED)
find_package(cpr CONFIG REQUIRED)
find_package(unofficial-sodium CONFIG REQUIRED)

add_subdirectory(backend)