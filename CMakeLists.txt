cmake_minimum_required(VERSION 4.0)
project(Jeu_de_la_vie_de_Conway C)

set(CMAKE_C_STANDARD 11)

set(RAYLIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vendor/raylib)

include_directories(${RAYLIB_DIR}/include)
link_directories(${RAYLIB_DIR}/lib)

add_executable(Jeu_de_la_vie_de_Conway main.c)

target_link_libraries(Jeu_de_la_vie_de_Conway
        raylib
        opengl32
        gdi32
        winmm
        m
)