cmake_minimum_required(VERSION 3.16)
project(sbert_search)

# =========================
# C++ Standard
# =========================
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# =========================
# Options
# =========================
option(USE_THREADPOOL "Use cached thread pool for parallel_search" OFF)

# =========================
# External libraries
# =========================
#set(BERT "${CMAKE_SOURCE_DIR}/lib/libbert.a")
#set(GGML "${CMAKE_SOURCE_DIR}/lib/libggml.a")
set(BERT "${CMAKE_SOURCE_DIR}/lib/libbert.dylib")
set(GGML "${CMAKE_SOURCE_DIR}/lib/libggml.dylib")

# =========================
# Core sources (library only)
# =========================
set(CORE_SOURCES
    src/BertIndex.cpp
    src/HnswConfig.cpp
    src/ShardedIndex.cpp
    src/BertIndexManager.cpp
    src/Util.cpp
    src/OffsetFile.cpp
    src/Logger.cpp
    src/FileLock.cpp
    src/SBertGGML.cpp
    src/hf_mapper.cpp
    src/StringEmbedding.cpp
    src/FileStreamCache.cpp
    src/unified_hnsw.cpp
)

set(EXTENDED_SOURCES
    ${CORE_SOURCES}
    src/EmbeddingIndexer.cpp
)

# =========================
# STATIC LIB
# =========================
add_library(sbert_core STATIC ${CORE_SOURCES})

target_include_directories(sbert_core
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(sbert_core
    PUBLIC
        ${BERT}
        ${GGML}
)

# =========================
# SHARED LIB
# =========================
add_library(sbert_shared SHARED ${CORE_SOURCES})

target_include_directories(sbert_shared
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(sbert_shared
    PUBLIC
        ${BERT}
        ${GGML}
)

set_target_properties(sbert_shared PROPERTIES
    OUTPUT_NAME "sbert"
)

# =========================
# Threadpool
# =========================
if(USE_THREADPOOL)
    target_compile_definitions(sbert_core PUBLIC USE_THREADPOOL)
    target_compile_definitions(sbert_shared PUBLIC USE_THREADPOOL)
endif()

# =========================
# OpenMP
# =========================
find_package(OpenMP)
if(OpenMP_CXX_FOUND)
    target_link_libraries(sbert_core PUBLIC OpenMP::OpenMP_CXX)
    target_link_libraries(sbert_shared PUBLIC OpenMP::OpenMP_CXX)
endif()

# =========================
# Executables (DEFINE FIRST!)
# =========================
add_executable(sbert_search src/main.cpp)
add_executable(config_editor utils/config_editor.cpp)
add_executable(test_reconstruct tests/test_reconstruct.cpp)

# =========================
# Link executables → shared lib
# =========================
target_link_libraries(sbert_search PRIVATE sbert_shared)
target_link_libraries(config_editor PRIVATE sbert_shared)
target_link_libraries(test_reconstruct PRIVATE sbert_shared)

# =========================
# Optimization
# =========================
target_compile_options(sbert_core PRIVATE -O3)
target_compile_options(sbert_shared PRIVATE -O3)

# =========================
# macOS specific fixes
# =========================
if(APPLE)

    # Enable rpath globally
    set(CMAKE_MACOSX_RPATH ON)

    # Shared lib install name
    set_target_properties(sbert_shared PROPERTIES
        INSTALL_NAME_DIR "@rpath"
    )

    # Executables find dylib next to them
    set_target_properties(sbert_search config_editor test_reconstruct PROPERTIES
        BUILD_RPATH "@loader_path"
    )

    # Accelerate framework
    target_link_libraries(sbert_core PUBLIC "-framework Accelerate")
    target_link_libraries(sbert_shared PUBLIC "-framework Accelerate")

endif()

target_link_options(sbert_shared PRIVATE
    "-Wl,-undefined,dynamic_lookup"
)

add_library(sbert_extended SHARED ${EXTENDED_SOURCES})

target_include_directories(sbert_extended
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
	ib/src

)

target_link_libraries(sbert_extended
    PUBLIC
        ${BERT}
        ${GGML}
)

# Allow unresolved symbols (same as your main shared lib)
target_link_options(sbert_extended PRIVATE
    "-Wl,-undefined,dynamic_lookup"
)

set_target_properties(sbert_extended PROPERTIES
    OUTPUT_NAME "schmate"
)
