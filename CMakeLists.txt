
cmake_minimum_required(VERSION 3.13)

project(ProgLang VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_subdirectory(Lang)

add_subdirectory(Test)

add_subdirectory(Cmd)