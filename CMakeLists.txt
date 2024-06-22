
cmake_minimum_required(VERSION 3.13)

project(ProgLang VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# add_compile_options(-fsanitize=address)

include(FetchContent)

FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz)
FetchContent_MakeAvailable(json)

add_subdirectory(Lang)

add_subdirectory(Test)

add_subdirectory(Cmd)