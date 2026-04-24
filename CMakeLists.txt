cmake_minimum_required(VERSION 3.20)
project(reIsearch VERSION 4.7 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ─────────────────────────────────────────────
# Project root — one level above this file.
# Must be defined first; everything else uses it.
# ─────────────────────────────────────────────
get_filename_component(PROJECT_ROOT "${CMAKE_SOURCE_DIR}/" ABSOLUTE)
set(SRC_DIR     "${PROJECT_ROOT}/src")
set(DOCTYPE_DIR "${PROJECT_ROOT}/doctype")

message(STATUS "Project root : ${PROJECT_ROOT}")
message(STATUS "Source dir   : ${SRC_DIR}")
message(STATUS "Doctype dir  : ${DOCTYPE_DIR}")

# ─────────────────────────────────────────────
# Platform detection
# ─────────────────────────────────────────────
if(APPLE)
    message(STATUS "Platform: macOS")
    set(IS_MACOS TRUE)
elseif(UNIX)
    message(STATUS "Platform: Linux")
    set(IS_LINUX TRUE)
else()
    message(FATAL_ERROR "Unsupported platform (macOS and Linux only)")
endif()

# ─────────────────────────────────────────────
# Options
# ─────────────────────────────────────────────
option(BUILD_VECTOR_INDEX "Build with HNSW/vector index support"  ON)
option(BUILD_SHARED_LIBS  "Build shared libraries"                ON)
option(USE_LIBMAGIC       "Use libmagic for file type detection"   ON)
option(USE_JEMALLOC       "Use jemalloc allocator"                ON)
option(USE_OPENMP         "Use OpenMP"                            ON)
option(USE_BERKELEY_DB    "Use Berkeley DB"                       ON)

# ─────────────────────────────────────────────
# Platform-specific default search paths
# ─────────────────────────────────────────────
if(IS_MACOS)
    set(_DEFAULT_BDB_ROOT     "/opt/homebrew/Cellar/berkeley-db/18.1.40_2")
    set(_DEFAULT_MAGIC_ROOT   "/opt/homebrew/Cellar/libmagic/5.45")
    set(_DEFAULT_OPENSSL_ROOT "/opt/homebrew/Cellar/openssl@3/3.5.2")
    set(_DEFAULT_EXTRA_INC    "/opt/homebrew/include")
    set(_DEFAULT_EXTRA_LIB    "/opt/homebrew/lib")
else()
    # Linux: standard system paths — override with -D for non-system installs
    set(_DEFAULT_BDB_ROOT     "/usr")
    set(_DEFAULT_MAGIC_ROOT   "/usr")
    set(_DEFAULT_OPENSSL_ROOT "/usr")
    set(_DEFAULT_EXTRA_INC    "")
    set(_DEFAULT_EXTRA_LIB    "")
endif()

# ─────────────────────────────────────────────
# Configurable paths (override on cmake line with -D)
# ─────────────────────────────────────────────
set(BERKELEY_DB_ROOT    "${_DEFAULT_BDB_ROOT}"
    CACHE PATH "Berkeley DB root")
set(LIBMAGIC_ROOT       "${_DEFAULT_MAGIC_ROOT}"
    CACHE PATH "libmagic root")
set(OPENSSL_ROOT        "${_DEFAULT_OPENSSL_ROOT}"
    CACHE PATH "OpenSSL root (for autoremote plugin)")
set(SCHMATE_INCLUDE_DIR "${PROJECT_ROOT}/Schmate/include"
    CACHE PATH "Schmate/HNSW include dir")
set(SCHMATE_LIB_DIR     "${PROJECT_ROOT}/Schmate/lib"
    CACHE PATH "Schmate/HNSW library dir")

set(INSTALL_BIN_DIR  "/usr/local/bin"    CACHE PATH "Executable install dir")
set(INSTALL_LIB_DIR  "/usr/local/lib/ib" CACHE PATH "Library install dir")

# ─────────────────────────────────────────────
# Compiler flags
# ─────────────────────────────────────────────
add_compile_options(-O3 -ffast-math -ftree-vectorize -fpic -m64)

if(IS_MACOS)
    # clang: needs explicit -fvectorize; allow unresolved symbols in dylibs
    add_compile_options(-fvectorize)
    add_link_options(-undefined dynamic_lookup)
else()
    # GCC: -ftree-vectorize covers it; export symbols for plugin resolution
    add_link_options(-Wl,--export-dynamic)
endif()

# ─────────────────────────────────────────────
# Global preprocessor defines
# ─────────────────────────────────────────────
add_compile_definitions(
    SVR4
    WWW_SUPPORT
    __CSTRING__
    BSDN_EXTENSIONS=2
    DO_BUILD_IB64=1
)
if(IS_MACOS)
    add_compile_definitions(BSD)
endif()
if(USE_LIBMAGIC)
    add_compile_definitions(USE_LIBMAGIC)
endif()
if(BUILD_VECTOR_INDEX)
    add_compile_definitions(VECTOR_INDEX=1)
endif()

# ─────────────────────────────────────────────
# Threading
# ─────────────────────────────────────────────
find_package(Threads REQUIRED)
add_compile_definitions(pthread)   # mirrors -pthread in Makefile DEFINES

# ─────────────────────────────────────────────
# OpenMP
# ─────────────────────────────────────────────
if(USE_OPENMP)
    find_package(OpenMP REQUIRED)
    if(IS_MACOS)
        # Apple clang doesn't bundle libomp — needs Homebrew llvm's libomp
        find_library(OMP_LIB omp HINTS "${_DEFAULT_EXTRA_LIB}" REQUIRED)
        message(STATUS "OpenMP libomp  : ${OMP_LIB}")
    endif()
endif()

# ─────────────────────────────────────────────
# Berkeley DB
# ─────────────────────────────────────────────
if(USE_BERKELEY_DB)
    find_library(BDB_LIB db
        HINTS "${BERKELEY_DB_ROOT}/lib"
        REQUIRED)
    find_path(BDB_INCLUDE db.h
        HINTS "${BERKELEY_DB_ROOT}/include"
        REQUIRED)
    message(STATUS "Berkeley DB    : ${BDB_LIB}")
endif()

# ─────────────────────────────────────────────
# libmagic
# ─────────────────────────────────────────────
if(USE_LIBMAGIC)
    find_library(MAGIC_LIB magic
        HINTS "${LIBMAGIC_ROOT}/lib"
        REQUIRED)
    find_path(MAGIC_INCLUDE magic.h
        HINTS "${LIBMAGIC_ROOT}/include"
        REQUIRED)
    message(STATUS "libmagic       : ${MAGIC_LIB}")
endif()

# ─────────────────────────────────────────────
# jemalloc
# ─────────────────────────────────────────────
if(USE_JEMALLOC)
    find_library(JEMALLOC_LIB jemalloc
        HINTS "${_DEFAULT_EXTRA_LIB}" /usr/lib /usr/local/lib
        REQUIRED)
    message(STATUS "jemalloc       : ${JEMALLOC_LIB}")
endif()

# ─────────────────────────────────────────────
# OpenSSL (optional — autoremote plugin only)
# ─────────────────────────────────────────────
find_library(SSL_LIB ssl
    HINTS "${OPENSSL_ROOT}/lib" /usr/lib /usr/local/lib)
find_library(CRYPTO_LIB crypto
    HINTS "${OPENSSL_ROOT}/lib" /usr/lib /usr/local/lib)
find_path(OPENSSL_INCLUDE openssl/ssl.h
    HINTS "${OPENSSL_ROOT}/include" /usr/include /usr/local/include)

# ─────────────────────────────────────────────
# schmate / HNSW vector library
# ─────────────────────────────────────────────
if(BUILD_VECTOR_INDEX)
    find_library(SCHMATE_LIB schmate
        HINTS "${SCHMATE_LIB_DIR}" "${PROJECT_ROOT}/lib"
        REQUIRED)
    message(STATUS "schmate        : ${SCHMATE_LIB}")
endif()

# ─────────────────────────────────────────────
# Helper: standard include paths for ib targets
# ─────────────────────────────────────────────
function(ib_common_includes tgt)
    target_include_directories(${tgt} PUBLIC ${SRC_DIR} ${DOCTYPE_DIR})
    if(_DEFAULT_EXTRA_INC)
        target_include_directories(${tgt} PUBLIC ${_DEFAULT_EXTRA_INC})
    endif()
endfunction()

# ─────────────────────────────────────────────
# ibUtils — base/utility library
# ─────────────────────────────────────────────
add_library(ibUtils
    ${SRC_DIR}/string.cxx       ${SRC_DIR}/fuzzy.cxx
    ${SRC_DIR}/qsort.cxx        ${SRC_DIR}/vlist.cxx
    ${SRC_DIR}/plist.cxx        ${SRC_DIR}/common.cxx
    ${SRC_DIR}/error.cxx        ${SRC_DIR}/glob.cxx
    ${SRC_DIR}/fieldmatch.cxx   ${SRC_DIR}/pathname.cxx
    ${SRC_DIR}/inode.cxx        ${SRC_DIR}/numbers.cxx
    ${SRC_DIR}/snefru.cxx       ${SRC_DIR}/directory.cxx
    ${SRC_DIR}/logger.cxx       ${SRC_DIR}/mmap.cxx
    ${SRC_DIR}/object.cxx       ${SRC_DIR}/dictionary.cxx
    ${SRC_DIR}/strlist.cxx      ${SRC_DIR}/strstack.cxx
    ${SRC_DIR}/lang-codes.cxx   ${SRC_DIR}/unicode.cxx
    ${SRC_DIR}/htmlcat.cxx      ${SRC_DIR}/date.cxx
    ${SRC_DIR}/listof.cxx       ${SRC_DIR}/hash.cxx
    ${SRC_DIR}/hashof.cxx       ${SRC_DIR}/buffer.cxx
    ${SRC_DIR}/glist.cxx        ${SRC_DIR}/gstack.cxx
    ${SRC_DIR}/registry.cxx     ${SRC_DIR}/fprec.cxx
    ${SRC_DIR}/fpt.cxx          ${SRC_DIR}/crc64.cxx
    ${SRC_DIR}/utf8.cxx         ${SRC_DIR}/eudex.cxx
    ${SRC_DIR}/metaphone.cxx
)
ib_common_includes(ibUtils)

# ─────────────────────────────────────────────
# ibIO — network/IO library
# ─────────────────────────────────────────────
set(IO_SRCS
    ${SRC_DIR}/io.cxx         ${SRC_DIR}/protman.cxx
    ${SRC_DIR}/netbox.cxx     ${SRC_DIR}/strbox.cxx
    ${SRC_DIR}/treebox.cxx
)
if(USE_BERKELEY_DB)
    list(APPEND IO_SRCS ${SRC_DIR}/rldcache.cxx)
endif()
add_library(ibIO ${IO_SRCS})
ib_common_includes(ibIO)
target_link_libraries(ibIO PUBLIC ibUtils)
if(USE_BERKELEY_DB)
    target_include_directories(ibIO PUBLIC ${BDB_INCLUDE})
    target_link_libraries(ibIO PUBLIC ${BDB_LIB})
endif()

# ─────────────────────────────────────────────
# ibProcess
# ─────────────────────────────────────────────
add_library(ibProcess
    ${SRC_DIR}/popen.cxx
    ${SRC_DIR}/process.cxx
)
ib_common_includes(ibProcess)
target_link_libraries(ibProcess PUBLIC ibUtils)

# ─────────────────────────────────────────────
# ibLocal — ctype/ranking (C sources)
# ─────────────────────────────────────────────
add_library(ibLocal
    ${SRC_DIR}/ctype.c
    ${SRC_DIR}/ranking.c
    ${SRC_DIR}/extras.c
)
target_include_directories(ibLocal PUBLIC ${SRC_DIR})

# ─────────────────────────────────────────────
# ibSearch — search engine library
# ─────────────────────────────────────────────
add_library(ibSearch
    ${SRC_DIR}/attr.cxx         ${SRC_DIR}/attrlist.cxx
    ${SRC_DIR}/df.cxx           ${SRC_DIR}/dfd.cxx
    ${SRC_DIR}/dfdt.cxx         ${SRC_DIR}/dft.cxx
    ${SRC_DIR}/fc.cxx           ${SRC_DIR}/fct.cxx
    ${SRC_DIR}/nodetree.cxx     ${SRC_DIR}/idbobj.cxx
    ${SRC_DIR}/idb.cxx          ${SRC_DIR}/metadata.cxx
    ${SRC_DIR}/scan.cxx         ${SRC_DIR}/vidb.cxx
    ${SRC_DIR}/index.cxx        ${SRC_DIR}/db_string.cxx
    ${SRC_DIR}/numsearch.cxx    ${SRC_DIR}/datesearch.cxx
    ${SRC_DIR}/dsearch.cxx      ${SRC_DIR}/geosearch.cxx
    ${SRC_DIR}/nlist.cxx        ${SRC_DIR}/dlist.cxx
    ${SRC_DIR}/nfldmgr.cxx      ${SRC_DIR}/intlist.cxx
    ${SRC_DIR}/intfield.cxx     ${SRC_DIR}/gpolylist.cxx
    ${SRC_DIR}/gpolyfield.cxx   ${SRC_DIR}/bboxfield.cxx
    ${SRC_DIR}/bboxlist.cxx     ${SRC_DIR}/nfield.cxx
    ${SRC_DIR}/dfield.cxx       ${SRC_DIR}/lock.cxx
    ${SRC_DIR}/iresult.cxx      ${SRC_DIR}/resource.cxx
    ${SRC_DIR}/irset.cxx        ${SRC_DIR}/mdt.cxx
    ${SRC_DIR}/mdthashtable.cxx ${SRC_DIR}/mdtrec.cxx
    ${SRC_DIR}/opobj.cxx        ${SRC_DIR}/operand.cxx
    ${SRC_DIR}/operator.cxx     ${SRC_DIR}/opstack.cxx
    ${SRC_DIR}/reclist.cxx      ${SRC_DIR}/record.cxx
    ${SRC_DIR}/recordsyntax.cxx ${SRC_DIR}/result.cxx
    ${SRC_DIR}/rset.cxx         ${SRC_DIR}/soundex.cxx
    ${SRC_DIR}/squery.cxx       ${SRC_DIR}/thesaurus.cxx
    ${SRC_DIR}/sterm.cxx        ${SRC_DIR}/termobj.cxx
    ${SRC_DIR}/rcache.cxx       ${SRC_DIR}/fcache.cxx
    ${SRC_DIR}/filemap.cxx      ${SRC_DIR}/mergeunit.cxx
    ${SRC_DIR}/infix2rpn.cxx    ${SRC_DIR}/tokengen.cxx
    ${SRC_DIR}/merge.cxx        ${SRC_DIR}/stoplist.cxx
    ${SRC_DIR}/support.cxx      ${SRC_DIR}/defs.cxx
)
ib_common_includes(ibSearch)
target_link_libraries(ibSearch PUBLIC ibUtils ibLocal Threads::Threads)
if(USE_OPENMP)
    target_link_libraries(ibSearch PUBLIC OpenMP::OpenMP_CXX)
    if(IS_MACOS AND OMP_LIB)
        target_link_libraries(ibSearch PUBLIC ${OMP_LIB})
    endif()
endif()
if(USE_LIBMAGIC)
    target_include_directories(ibSearch PUBLIC ${MAGIC_INCLUDE})
    target_link_libraries(ibSearch PUBLIC ${MAGIC_LIB})
endif()
if(BUILD_VECTOR_INDEX)
    target_include_directories(ibSearch PUBLIC ${SCHMATE_INCLUDE_DIR})
    target_link_libraries(ibSearch PUBLIC ${SCHMATE_LIB})
endif()

# ─────────────────────────────────────────────
# ibDoc — document type handlers
# ─────────────────────────────────────────────
add_library(ibDoc
    ${SRC_DIR}/dtreg.cxx
    ${DOCTYPE_DIR}/doctype.cxx      ${DOCTYPE_DIR}/autodetect.cxx
    ${DOCTYPE_DIR}/resourcedoc.cxx  ${DOCTYPE_DIR}/image.cxx
    ${DOCTYPE_DIR}/plaintext.cxx    ${DOCTYPE_DIR}/simple.cxx
    ${DOCTYPE_DIR}/para.cxx         ${DOCTYPE_DIR}/ptext.cxx
    ${DOCTYPE_DIR}/binary.cxx       ${DOCTYPE_DIR}/filter.cxx
    ${DOCTYPE_DIR}/xfilter.cxx      ${DOCTYPE_DIR}/ftp.cxx
    ${DOCTYPE_DIR}/bibtex.cxx       ${DOCTYPE_DIR}/firstline.cxx
    ${DOCTYPE_DIR}/colondoc.cxx     ${DOCTYPE_DIR}/tsldoc.cxx
    ${DOCTYPE_DIR}/csvdoc.cxx       ${DOCTYPE_DIR}/ozsearch.cxx
    ${DOCTYPE_DIR}/psdoc.cxx        ${DOCTYPE_DIR}/iknowdoc.cxx
    ${DOCTYPE_DIR}/roadsdoc.cxx     ${DOCTYPE_DIR}/colongrp.cxx
    ${DOCTYPE_DIR}/euromedia.cxx    ${DOCTYPE_DIR}/mismedia.cxx
    ${DOCTYPE_DIR}/bibcolon.cxx     ${DOCTYPE_DIR}/dif.cxx
    ${DOCTYPE_DIR}/iafadoc.cxx      ${DOCTYPE_DIR}/mailfolder.cxx
    ${DOCTYPE_DIR}/newsfolder.cxx   ${DOCTYPE_DIR}/referbib.cxx
    ${DOCTYPE_DIR}/irlist.cxx       ${DOCTYPE_DIR}/mailman.cxx
    ${DOCTYPE_DIR}/listdigest.cxx   ${DOCTYPE_DIR}/maildigest.cxx
    ${DOCTYPE_DIR}/yahoolist.cxx    ${DOCTYPE_DIR}/digesttoc.cxx
    ${DOCTYPE_DIR}/medline.cxx      ${DOCTYPE_DIR}/filmline.cxx
    ${DOCTYPE_DIR}/oneline.cxx      ${DOCTYPE_DIR}/dvbline.cxx
    ${DOCTYPE_DIR}/memodoc.cxx      ${DOCTYPE_DIR}/soif.cxx
    ${DOCTYPE_DIR}/harvest.cxx      ${DOCTYPE_DIR}/sgmltag.cxx
    ${DOCTYPE_DIR}/gils.cxx         ${DOCTYPE_DIR}/sgmlnorm.cxx
    ${DOCTYPE_DIR}/sgml.cxx         ${DOCTYPE_DIR}/xml.cxx
    ${DOCTYPE_DIR}/gilsxml.cxx      ${DOCTYPE_DIR}/gils_isoteia.cxx
    ${DOCTYPE_DIR}/rss2.cxx         ${DOCTYPE_DIR}/html.cxx
    ${DOCTYPE_DIR}/HTMLEntities.cxx ${DOCTYPE_DIR}/htmlmeta.cxx
    ${DOCTYPE_DIR}/htmlhead.cxx     ${DOCTYPE_DIR}/htmlremote.cxx
    ${DOCTYPE_DIR}/antihtml.cxx     ${DOCTYPE_DIR}/htmlcache.cxx
    ${DOCTYPE_DIR}/filter2.cxx      ${DOCTYPE_DIR}/pandoc.cxx
    ${DOCTYPE_DIR}/pdf.cxx
)
ib_common_includes(ibDoc)
target_link_libraries(ibDoc PUBLIC ibUtils ibLocal)

# ─────────────────────────────────────────────
# ibApps — exports _Iindex_main / _Isearch_main / _Iutil_main
# ─────────────────────────────────────────────
add_library(ibApps
    ${SRC_DIR}/iindex_main.cxx
    ${SRC_DIR}/isearch_main.cxx
    ${SRC_DIR}/iutil_main.cxx
)
ib_common_includes(ibApps)
target_link_libraries(ibApps PUBLIC ibUtils ibSearch ibDoc ibProcess)

# ─────────────────────────────────────────────
# Common link set used by all executables
# ─────────────────────────────────────────────
set(COMMON_LINK_LIBS
    ibLocal ibUtils ibSearch ibDoc ibProcess ibIO ibApps
    m dl
)
if(USE_LIBMAGIC)
    list(APPEND COMMON_LINK_LIBS ${MAGIC_LIB})
endif()
if(USE_BERKELEY_DB)
    list(APPEND COMMON_LINK_LIBS ${BDB_LIB})
endif()
if(USE_JEMALLOC)
    list(APPEND COMMON_LINK_LIBS ${JEMALLOC_LIB})
endif()
if(USE_OPENMP)
    list(APPEND COMMON_LINK_LIBS OpenMP::OpenMP_CXX)
    if(IS_MACOS AND OMP_LIB)
        list(APPEND COMMON_LINK_LIBS ${OMP_LIB})
    endif()
endif()
if(BUILD_VECTOR_INDEX)
    list(APPEND COMMON_LINK_LIBS ${SCHMATE_LIB})
endif()
if(IS_LINUX)
    list(APPEND COMMON_LINK_LIBS Threads::Threads)
endif()

# ─────────────────────────────────────────────
# Main executables — thin C wrappers calling into ibApps
# ─────────────────────────────────────────────
foreach(exe Iindex Isearch Iutil)
    add_executable(${exe} ${SRC_DIR}/${exe}.c)
    target_compile_options(${exe} PRIVATE -m64)
    target_link_libraries(${exe} PRIVATE ${COMMON_LINK_LIBS})
    set_target_properties(${exe} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${PROJECT_ROOT}/bin"
    )
endforeach()

# Idelete — self-contained with its own main()
add_executable(Idelete ${SRC_DIR}/Idelete.cxx)
ib_common_includes(Idelete)
target_link_libraries(Idelete PRIVATE ${COMMON_LINK_LIBS})
set_target_properties(Idelete PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${PROJECT_ROOT}/bin"
)

# ─────────────────────────────────────────────
# Utility executables
# ─────────────────────────────────────────────
foreach(util siscompress sisdump sisdump32)
    add_executable(${util} ${SRC_DIR}/${util}.c)
    set_target_properties(${util} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${PROJECT_ROOT}/bin"
    )
endforeach()

add_executable(sischeck ${SRC_DIR}/sischeck.cxx)
target_link_libraries(sischeck PRIVATE ${COMMON_LINK_LIBS})
set_target_properties(sischeck PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${PROJECT_ROOT}/bin"
)

add_executable(Iwatch ${SRC_DIR}/Iwatch.c)
set_target_properties(Iwatch PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${PROJECT_ROOT}/bin"
)

add_executable(zipper ${SRC_DIR}/zipper.cxx)
set_target_properties(zipper PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${PROJECT_ROOT}/bin"
)

foreach(exe zpresent zsearch)
    add_executable(${exe} ${SRC_DIR}/${exe}.cxx)
    ib_common_includes(${exe})
    target_link_libraries(${exe} PRIVATE ${COMMON_LINK_LIBS})
    set_target_properties(${exe} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${PROJECT_ROOT}/bin"
    )
endforeach()

# ─────────────────────────────────────────────
# autoremote plugin (optional — needs OpenSSL)
# ─────────────────────────────────────────────
if(SSL_LIB AND CRYPTO_LIB AND OPENSSL_INCLUDE)
    add_library(autoremote MODULE ${DOCTYPE_DIR}/autoremote.cxx)
    ib_common_includes(autoremote)
    target_include_directories(autoremote PRIVATE ${OPENSSL_INCLUDE})
    target_link_libraries(autoremote PRIVATE
        ${COMMON_LINK_LIBS} ${SSL_LIB} ${CRYPTO_LIB}
    )
    set_target_properties(autoremote PROPERTIES
        PREFIX ""
        # CMake sets .dylib on macOS, .so on Linux for MODULE targets automatically
        LIBRARY_OUTPUT_DIRECTORY "${PROJECT_ROOT}/lib/plugins"
    )
    message(STATUS "autoremote plugin: enabled")
else()
    message(STATUS "autoremote plugin: disabled (OpenSSL not found)")
endif()

# ─────────────────────────────────────────────
# Install
# ─────────────────────────────────────────────
install(TARGETS Iindex Isearch Iutil Idelete zpresent zsearch Iwatch zipper
    RUNTIME DESTINATION "${INSTALL_BIN_DIR}"
)
install(TARGETS ibUtils ibSearch ibDoc ibProcess ibLocal ibIO ibApps
    LIBRARY DESTINATION "${INSTALL_LIB_DIR}"
)

# ─────────────────────────────────────────────
# User plugin directory
# ─────────────────────────────────────────────
file(MAKE_DIRECTORY "$ENV{HOME}/.ib/lib/plugins")
