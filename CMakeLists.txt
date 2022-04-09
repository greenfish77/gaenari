######################
# gaenari build & test
######################
# mkdir build
# cd build
# cmake ..
# cmake --build . --config release
# cmake --install . --prefix install
# ctest --verbose
# (find executables in build/tests/* and just execute it, display in color.)

####################
# build with gaenari
####################
# -------------------
# wrapper/wrapper.cpp
# -------------------
# #include "gaenari/gaenari.hpp"
# int main(void) {
#    gaenari::logger::init1("/temp/_log.txt");
#    using supul_t = supul::supul::supul_t;
#    supul_t::api::project::create("/temp/supul_dir");
#    supul_t::api::project::add_field("/temp/supul_dir", "x1", "REAL");
#    supul_t::api::project::add_field("/temp/supul_dir", "x2", "INTEGER");
#    supul_t::api::project::add_field("/temp/supul_dir", "x3", "TEXT_ID");
#    supul_t::api::project::add_field("/temp/supul_dir", "y0", "TEXT_ID");
#    supul_t::api::project::x("/temp/supul_dir", {"x1", "x2", "x3"});
#    supul_t::api::project::y("/temp/supul_dir", "y0");
#    supul_t::api::project::set_property("/temp/supul_dir", "db.type", "sqlite");
#    supul_t supul;
#    supul.api.lifetime.open("/temp/supul_dir");
#    supul.api.model.insert_chunk_csv("/temp/dataset.csv");
#    supul.api.model.update();
#    supul.api.model.deinit();
#    // ...
#    supul.api.model.rebuild();
#    supul.api.lifetime.close();
#    return 0;
# }
#
# ----------------------
# wrapper/CMakeLists.txt
# ----------------------
# cmake_minimum_required(VERSION 3.6)
# project(wrapper)
#
# # call order is important.
#
# add_subdirectory(</path/to/gaenari>)
# check_cpp17_gaenari()
#
# add_executable(wrapper wrapper.cpp)
# add_gaenari(wrapper)
#
# -----
# build
# -----
# wrapper/build$ cmake ..
# wrapper/build$ cmake --build . --config release

cmake_minimum_required(VERSION 3.18)

##########
# settings
##########
# project.
project(gaenari VERSION 1.2)

#################################
# auto git ignore build directory
#################################
if(NOT EXISTS ${PROJECT_BINARY_DIR}/.gitignore)
    file(WRITE ${PROJECT_BINARY_DIR}/.gitignore "*")
endif()

#########
# include
#########
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/gaenari_util.cmake")

#########
# options
#########
set(_SQLITE_SHA256_     "999826FE4C871F18919FDB8ED7EC9DD8217180854DD1FE21EEA96AED36186729")
set(SQLITE_VERSION      "3360000"           CACHE STRING "sqlite version to build. minimum(3350400(=3.35.5) for supporting `returning`)")
set(SQLITE_RELEASE_YEAR "2021"              CACHE STRING "release year of SQLITE_VERSION.")
set(SQLITE_SHA256       ${_SQLITE_SHA256_}  CACHE STRING "sha256 value of sqlite.zip.")
set(BUILD_WEB           true                CACHE BOOL   "build web-console.")   

##################
# global variables
##################
# download directory.
set(DOWNLOAD_DIR            "${CMAKE_CURRENT_BINARY_DIR}/download")

# gaenari include directory.
set(GAENARI_INCLUDE_DIR     "${CMAKE_CURRENT_SOURCE_DIR}/include/")

# sqlite include directory.
set(SQLITE_INCLUDE_DIR      "${CMAKE_CURRENT_SOURCE_DIR}/extern/sqlite")

# cpp-httplib(0.10.4) for web console.
set(CPP_HTTPLIB_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/extern/cpp-httplib-0.10.4")

# gaenari hpp files.
# gaenari has only header files.
# this list is used to build source_group for IDE tools, not compilation.
file(GLOB_RECURSE GAENARI_HPP_FILES
    "${GAENARI_INCLUDE_DIR}/*.hpp"
    "${GAENARI_INCLUDE_DIR}/*.h"
)

#######
# c++17
#######
check_cpp17_gaenari()

###########
# configure
###########
configure_file("${GAENARI_INCLUDE_DIR}/gaenari/gaenari-config.h.in" "${GAENARI_INCLUDE_DIR}/gaenari/gaenari-config.h")

############
# definition
############
# compile definitions.
set(GAENARI_DEFINITIONS "")
add_compile_definitions(${GAENARI_DEFINITIONS})

#########
# targets
#########
# add sqlite target to build.
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/extern/sqlite")

# add tests target to build.
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/tests")

# add web console target to build.
if(BUILD_WEB)
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/gaenari-web")
endif()

# add files target to edit some files in IDE.
add_custom_target(files SOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/README.md"
    "${CMAKE_CURRENT_SOURCE_DIR}/TODO.md"
)

#########################
# for add_gaenari() macro
#########################
set(GAENARI_INCLUDE_DIR_FOR_BUILD ${GAENARI_INCLUDE_DIR} ${SQLITE_INCLUDE_DIR} CACHE INTERNAL "_GAENARI_INCLUDE_DIR_FOR_BUILD")
set(GAEANRI_DEFINITION_FOR_BUILD  ${GAENARI_DEFINITIONS}                       CACHE INTERNAL "_GAEANRI_DEFINITION_FOR_BUILD")
set(GAENARI_LINK_FOR_BUILD        sqlite3                                      CACHE INTERNAL "_GAENARI_LINK_FOR_BUILD")

######
# test
######
# call tests from tests sub-directory.
enable_testing()

#########
# install
#########
# add gaenari header files to install directory.
install(DIRECTORY "${GAENARI_INCLUDE_DIR}" DESTINATION include)
if(BUILD_WEB)
    install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/gaenari-web/web-root" DESTINATION bin)
endif()
