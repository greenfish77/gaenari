cmake_minimum_required(VERSION 3.6)

# set the project.
PROJECT(tests)

# sqlite uses threads.
find_package(Threads REQUIRED)

# *.hpp files in tests.
file(GLOB_RECURSE TESTS_HPP_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp")

# only one tests.cpp files.
# *.hpp files in tests and gaenari are added for IDE tools, not compilation.
set(TESTS_SOURCES tests.cpp ${TESTS_HPP_FILES} ${GAENARI_HPP_FILES})

# in IDE tools, the gaenari header files are represented as a directory structure.
source_group(TREE ${GAENARI_INCLUDE_DIR} FILES ${GAENARI_HPP_FILES})

# add the executable.
add_executable(tests ${TESTS_SOURCES})

# set big obj.
if(MSVC)
    add_definitions(/bigobj)
endif()

# include.
target_include_directories(tests PRIVATE 
    ${GAENARI_INCLUDE_DIR}
    ${SQLITE_INCLUDE_DIR}
)

# link.
# sqlite.lib + thread + dl
target_link_libraries(tests PRIVATE sqlite3 ${CMAKE_THREAD_LIBS_INIT} ${CMAKE_DL_LIBS})

# test.
enable_testing()
add_test(default tests default)
# add_test(large tests large) # too long.

# install.
install(TARGETS tests DESTINATION bin)
