set(TARGET snake)
set(SRC main.cpp)

add_executable(${TARGET} ${SRC})
target_include_directories(${TARGET} PRIVATE 
    ${CMAKE_SOURCE_DIR}/source
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(${TARGET} PRIVATE
    Nes
    raylib
)

