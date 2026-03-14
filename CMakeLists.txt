cmake_minimum_required(VERSION 3.10.0)
project(gravity-sim VERSION 0.1.0 LANGUAGES C CXX)

add_executable(gravity-sim main.cpp)

find_package(SFML COMPONENTS graphics window system REQUIRED)
target_link_libraries(gravity-sim PRIVATE sfml-graphics sfml-window sfml-system)