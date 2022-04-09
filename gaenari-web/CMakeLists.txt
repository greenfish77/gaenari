cmake_minimum_required(VERSION 3.6)

# set the project.
PROJECT(gaenari-web)

# sqlite uses threads.
find_package(Threads REQUIRED)

# *.hpp files in gaenari-web.
file(GLOB_RECURSE GAENARI_WEB_HPP_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp")

# only one gaenari-web.cpp files.
# *.hpp files in gaenari-web and gaenari are added for IDE tools, not compilation.
set(GAENARI_WEB_SOURCES gaenari-web.cpp ${GAENARI_WEB_HPP_FILES} ${GAENARI_HPP_FILES})

# in IDE tools, the gaenari header files are represented as a directory structure.
source_group(TREE ${GAENARI_INCLUDE_DIR} FILES ${GAENARI_HPP_FILES})

# add the executable.
add_executable(gaenari-web ${GAENARI_WEB_SOURCES})

# set big obj.
if(MSVC)
    add_definitions(/bigobj)
endif()

# include.
target_include_directories(gaenari-web PRIVATE 
    ${GAENARI_INCLUDE_DIR}
    ${SQLITE_INCLUDE_DIR}
    ${CPP_HTTPLIB_INCLUDE_DIR}
)

# link.
# sqlite.lib + thread + dl
target_link_libraries(gaenari-web PRIVATE sqlite3 ${CMAKE_THREAD_LIBS_INIT} ${CMAKE_DL_LIBS})

# install.
install(TARGETS gaenari-web DESTINATION bin)
