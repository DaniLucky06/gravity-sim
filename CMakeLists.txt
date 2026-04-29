cmake_minimum_required(VERSION 3.16.0)
project(gravity-sim VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set( TARGET ${PROJECT_NAME} )
set( SOURCE_PATH "${PROJECT_SOURCE_DIR}/src" )
set( INCLUDE_PATH "${PROJECT_SOURCE_DIR}/include" )

file( GLOB_RECURSE HPPS "${INCLUDE_PATH}/*.hpp" )
file( GLOB_RECURSE CPPS "${SOURCE_PATH}/*.cpp" )

add_executable(${TARGET} ${CPPS} ${HPPS})

# Find SFML 3 with TitleCase components
find_package(SFML 3 COMPONENTS Graphics Window System REQUIRED)

# Link using modern SFML:: namespace targets
target_link_libraries(${TARGET} PRIVATE SFML::Graphics SFML::Window SFML::System)
target_include_directories(${TARGET} PUBLIC ${INCLUDE_PATH})

install(
    TARGETS ${TARGET}
)