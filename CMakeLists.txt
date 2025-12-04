cmake_minimum_required(VERSION 3.31.6)
project(Jeu_de_la_vie_de_Conway C)

set(CMAKE_C_STANDARD 11)

set(RAYLIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib/raylib)

include_directories(${RAYLIB_DIR}/include)
link_directories(${RAYLIB_DIR}/lib)

add_executable(Jeu_de_la_vie_de_Conway main.c
        patterns.h
)

target_link_libraries(Jeu_de_la_vie_de_Conway
        raylib
        opengl32
        gdi32
        winmm
        m
)