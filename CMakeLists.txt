cmake_minimum_required(VERSION "3.11" FATAL_ERROR)
project(NeHe-SDL_GPU LANGUAGES C)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules")

find_package(SDL3 REQUIRED CONFIG)

add_subdirectory(src/c)
