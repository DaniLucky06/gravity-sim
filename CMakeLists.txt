cmake_minimum_required(VERSION 3.10.0)
project(gravity-sim VERSION 0.1.0 LANGUAGES C CXX)

set(SFML_DIR "C:/Users/musne/Programming/cpp/.Libraries/SFML-2.6.2/lib/cmake/SFML")

find_package(SFML COMPONENTS graphics window system REQUIRED)

add_executable(gravity-sim main.cpp)
target_link_libraries(gravity-sim PRIVATE sfml-graphics sfml-window sfml-system)

# Tell CMake to copy all required DLLs next to the executable after building
add_custom_command(TARGET gravity-sim POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_RUNTIME_DLLS:gravity-sim>
    $<TARGET_FILE_DIR:gravity-sim>
    COMMAND_EXPAND_LISTS
)