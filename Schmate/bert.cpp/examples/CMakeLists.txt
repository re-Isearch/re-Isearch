include_directories(${CMAKE_SOURCE_DIR}/)

add_executable(main main.cpp)
target_link_libraries(main PRIVATE bert ggml)

#add_executable(basic basic.cpp)
#target_link_libraries(basic PRIVATE bert ggml)
