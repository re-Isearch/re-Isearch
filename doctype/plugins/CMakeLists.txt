# 1. Collect all .cxx files in this directory
file(GLOB PLUGIN_SRCS "*.cxx")

foreach(SOURCE_FILE ${PLUGIN_SRCS})
    # Get the base name (e.g., /path/to/odt.cxx -> odt)
    get_filename_component(PLUGIN_NAME ${SOURCE_FILE} NAME_WE)

    # 2. Handle Special Case: autoremote (requires OpenSSL)
    if(PLUGIN_NAME STREQUAL "autoremote")
        if(NOT (SSL_LIB AND CRYPTO_LIB AND OPENSSL_INCLUDE))
            message(STATUS "Skipping plugin: autoremote (OpenSSL missing)")
            continue()
        endif()
    endif()

    # 3. Create the plugin target
    add_library(${PLUGIN_NAME} MODULE ${SOURCE_FILE})

    # 4. Apply Common Logic
    ib_common_includes(${PLUGIN_NAME})
    
    # Link OpenSSL only for autoremote, others get standard libs
    if(PLUGIN_NAME STREQUAL "autoremote")
        target_include_directories(autoremote PRIVATE ${OPENSSL_INCLUDE})
        target_link_libraries(autoremote PRIVATE ${COMMON_LINK_LIBS} ${SSL_LIB} ${CRYPTO_LIB})
    else()
        target_link_libraries(${PLUGIN_NAME} PRIVATE ${COMMON_LINK_LIBS})
    endif()

    # 5. Force the .sob extension and output path
    set_target_properties(${PLUGIN_NAME} PROPERTIES
        PREFIX ""
        SUFFIX ".sob"
        LIBRARY_OUTPUT_DIRECTORY "${PROJECT_ROOT}/lib/plugins"
    )

    message(STATUS "Configured plugin: ${PLUGIN_NAME}.sob")
endforeach()
