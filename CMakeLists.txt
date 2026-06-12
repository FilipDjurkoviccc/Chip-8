cmake_minimum_required(VERSION 2.8...3.13)
project(chip8)
set(CMAKE_CXX_FLAGS_DEBUG "-g -Wall")


set(CMAKE_CXX_STANDARD 14)
find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)
add_compile_options(-g)

add_subdirectory(includes/glad)
add_executable(chip8 src/main.cpp src/Chip8.cpp)

target_link_libraries(chip8 glad glfw OpenGL::GL)
