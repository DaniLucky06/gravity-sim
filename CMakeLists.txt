cmake_minimum_required(VERSION 3.10.0)
project(gravity-sim VERSION 0.1.0 LANGUAGES C CXX)

set( TARGET ${PROJECT_NAME} )
set( SOURCE_PATH "${PROJECT_SOURCE_DIR}/src" )
set( INCLUDE_PATH "${PROJECT_SOURCE_DIR}/include" )
set(SFML_DIR "C:/Users/musne/Programming/cpp/.Libraries/SFML-2.6.2/lib/cmake/SFML") # FOR WINDOWS

file( GLOB_RECURSE HPPS "${INCLUDE_PATH}/*.hpp" )
file( GLOB_RECURSE CPPS "${SOURCE_PATH}/*.cpp" )

add_executable(${TARGET} ${CPPS} ${HPPS})

find_package(SFML COMPONENTS graphics window system REQUIRED)
target_link_libraries(${TARGET} PRIVATE sfml-graphics sfml-window sfml-system)
target_include_directories(${TARGET} PUBLIC ${INCLUDE_PATH})

install(
    TARGETS ${TARGET}
)