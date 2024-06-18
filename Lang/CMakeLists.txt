

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
)

add_library(ProgLang ${SOURCES})

target_include_directories(ProgLang PUBLIC "/")