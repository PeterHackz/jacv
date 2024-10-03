cmake_minimum_required(VERSION 3.27)
project(random_proj C)

set(CMAKE_C_STANDARD 11)

include_directories(include)

add_executable(random_proj
        include/ast.h
        include/gc.h
        include/internals.h
        include/io_utils.h
        include/jacv.h
        include/jmemory.h
        include/jutils.h
        include/keywords.h
        include/lexer.h
        include/list.h
        include/tokens.h
        ast.c
        io_utils.c
        jacv.c
        lexer.c
        list.c
        jutils.c
)
