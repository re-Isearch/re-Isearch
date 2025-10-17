cmake_minimum_required(VERSION 3.16)
project(sbert_search)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


option(USE_THREADPOOL "Use cached thread pool for parallel_search" OFF)
if(USE_THREADPOOL)
    add_definitions(-DUSE_THREADPOOL)
endif()

# Find libraries with error checking
find_library(BERT NAMES bert PATHS ../lib lib)
find_library(GGML NAMES ggml PATHS ../lib lib)

if(NOT BERT)
    message(FATAL_ERROR "libbert not found in ../lib or lib/")
endif()
if(NOT GGML)
    message(FATAL_ERROR "libggml not found in ../lib or lib/")
endif()

# Collect sources (fix path)
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)

# Main executable
add_executable(sbert_search ${SOURCES})
target_include_directories(sbert_search PRIVATE include)
target_link_libraries(sbert_search PRIVATE ${BERT} ${GGML} "-framework Accelerate")

add_executable(config_editor
    utils/config_editor.cpp
    src/HnswConfig.cpp
    src/Logger.cpp
    src/Util.cpp
)
target_include_directories(config_editor PRIVATE include)


# Test executable
add_executable(test_reconstruct 
    tests/test_reconstruct.cpp
    src/BertIndex.cpp
    src/ShardedIndex.cpp
    src/BertIndexManager.cpp
    src/Util.cpp
    src/OffsetFile.cpp
    src/Logger.cpp
    src/SBertGGML.cpp)
target_include_directories(test_reconstruct PRIVATE include)
target_link_libraries(test_reconstruct PRIVATE ${BERT} ${GGML} "-framework Accelerate")


