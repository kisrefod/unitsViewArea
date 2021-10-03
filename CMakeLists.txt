cmake_minimum_required(VERSION 3.20)
project(untitled)

set(CMAKE_CXX_STANDARD 14)
if(MSVC)
    set(CMAKE_CXX_FLAGS_RELEASE "/o2")
else()
    set(CMAKE_CXX_FLAGS_DEBUG "-g")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3")
endif()
find_package(OpenMP REQUIRED)

set(SFML_DIR "include/SFML/lib/cmake/SFML")
set(SFML_STATIC_LIBRARIES TRUE)
find_package(SFML 2.5.1 COMPONENTS graphics REQUIRED)

add_executable(untitled main.cpp objects.h)

target_link_libraries(untitled PRIVATE OpenMP::OpenMP_CXX sfml-graphics sfml-audio)