set(TARGET nes-tests)
set(SRC memory.cpp)

add_executable(${TARGET} ${SRC})
target_include_directories(${TARGET} PRIVATE 
    ${CMAKE_SOURCE_DIR}/source
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(${TARGET} PRIVATE
    Nes
    gtest_main
    gtest
)

add_test(NAME ${TARGET} 
    COMMAND ${TARGET}
)

if(ENABLE_COVERAGE)
    target_compile_options(${TARGET} PRIVATE -O0 -coverage -fno-exceptions -fno-inline)
    target_link_options(${TARGET} PRIVATE -coverage)
endif()