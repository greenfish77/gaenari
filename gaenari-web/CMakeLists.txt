cmake_minimum_required(VERSION 3.6)

# set the project.
PROJECT(gaenari-web VERSION 1.0)

# sqlite uses threads.
find_package(Threads REQUIRED)

# download AdminLTE.
set(ADMINLTE_URL "https://github.com/ColorlibHQ/AdminLTE/archive/refs/tags/v${ADMINLTE_VERSION}.zip")
message(STATUS "[info] download sqlite. wait...")
message(STATUS "[info]    * url: ${ADMINLTE_URL}")
message(STATUS "[info]    * to:  ${DOWNLOAD_DIR}/AdminLTE-${ADMINLTE_VERSION}.zip")
message(STATUS "[info] if the network is blocked, copy the downloaded file to the <to> directory. of course, the SHA256 value must match.")
file(DOWNLOAD
    "${ADMINLTE_URL}"
    "${DOWNLOAD_DIR}/AdminLTE-${ADMINLTE_VERSION}.zip"
    # SHOW_PROGRESS
    EXPECTED_HASH SHA256=${ADMINLTE_SHA256}
)
file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/web-root/www/extern")
message(STATUS "[info] download completed, and extract.")
execute_process(COMMAND "${CMAKE_COMMAND}" -E tar xzf "${DOWNLOAD_DIR}/AdminLTE-${ADMINLTE_VERSION}.zip"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/web-root/www/extern"
)

# configure.
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/gaenari-web-config.h.in" "${CMAKE_CURRENT_SOURCE_DIR}/gaenari-web-config.h")

# only one gaenari-web.cpp files.
# *.hpp files in gaenari and web-root are added for IDE tools, not compilation.
file(GLOB_RECURSE GAENARI_WEB_HPP_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp" "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
file(GLOB_RECURSE WEB_ROOT_FILES "${CMAKE_CURRENT_SOURCE_DIR}/web-root/*.*")
list(FILTER WEB_ROOT_FILES EXCLUDE REGEX "/www/extern/")
set(GAENARI_WEB_SOURCES gaenari-web.cpp ${GAENARI_WEB_HPP_FILES} ${GAENARI_HPP_FILES} ${WEB_ROOT_FILES})

# in IDE tools, some files are represented as a directory structure.
source_group(TREE ${GAENARI_INCLUDE_DIR} FILES ${GAENARI_HPP_FILES})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${WEB_ROOT_FILES})

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
