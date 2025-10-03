cmake_minimum_required(VERSION 3.16)
project(sbert_search)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS ../src/*.cpp)

add_executable(sbert_search ${SOURCES})

target_link_libraries(sbert_search PRIVATE bert ggml "-framework Accelerate")
