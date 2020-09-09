# https://cliutils.gitlab.io/modern-cmake/

cmake_minimum_required(VERSION 3.7...3.18)

if(${CMAKE_VERSION} VERSION_LESS 3.12)
	cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
endif()

project(RoguePC
	VERSION 0.1
	DESCRIPTION "Port of original PC-DOS Epyx Rogue to modern platforms"
	LANGUAGES C
)

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

add_executable(splash splash.c)

target_link_libraries(splash ${SDL2_LIBRARIES})
