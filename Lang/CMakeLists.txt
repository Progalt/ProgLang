

set (SOURCES 
    "Lang/Chunk.cpp"
    "Lang/Compiler.cpp"
    "Lang/Lexer.cpp"
    "Lang/Memory.cpp"
    "Lang/Object.cpp"
    "Lang/String.cpp"
    "Lang/Value.cpp"
    "Lang/VM.cpp"
    "Lang/Libraries/std.cpp"
    "Lang/Libraries/web.cpp"
    "Lang/EventSystem.cpp"
)

add_library(ProgLang ${SOURCES})

target_include_directories(ProgLang PUBLIC "/")

target_link_libraries(ProgLang PRIVATE nlohmann_json::nlohmann_json)
