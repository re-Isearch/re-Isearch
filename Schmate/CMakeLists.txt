cmake_minimum_required(VERSION 3.15)
project(SBERT_HNSW_Search CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BERTCPP_DIR "Path to bert.cpp root directory" "../bert.cpp")
option(HNSWLIB_DIR "Path to hnswlib root directory" "../hnswlib")
option(GGML_LIB_DIR "Directory containing built libraries (libbert, libggml)" "${BERTCPP_DIR}/build")

include_directories(${BERTCPP_DIR}/include)
include_directories(${HNSWLIB_DIR})

link_directories(${GGML_LIB_DIR})

add_executable(sbert_search main.cpp)

find_package(Threads REQUIRED)
target_link_libraries(sbert_search PRIVATE Threads::Threads)

# Link bert + ggml
target_link_libraries(sbert_search PRIVATE bert ggml)

# macOS Accelerate
if(APPLE)
    find_library(ACCELERATE_FRAMEWORK Accelerate)
    if(ACCELERATE_FRAMEWORK)
        target_link_libraries(sbert_search PRIVATE ${ACCELERATE_FRAMEWORK})
    endif()
endif()

set_target_properties(sbert_search PROPERTIES MACOSX_RPATH ON)


