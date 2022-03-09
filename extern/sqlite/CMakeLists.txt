PROJECT(sqlite3)
cmake_minimum_required(VERSION 3.6)

# download sqlite source.
set(SQLITE_URL "https://www.sqlite.org/${SQLITE_RELEASE_YEAR}/sqlite-amalgamation-${SQLITE_VERSION}.zip")
message(STATUS "[info] download sqlite. wait...")
message(STATUS "[info]    * url: ${SQLITE_URL}")
message(STATUS "[info]    * to:  ${DOWNLOAD_DIR}/sqlite-amalgamation-${SQLITE_VERSION}.zip")
message(STATUS "[info] if the network is blocked, copy the downloaded file to the <to> directory. of course, the SHA256 value must match.")
file(DOWNLOAD
    "${SQLITE_URL}"
    "${DOWNLOAD_DIR}/sqlite-amalgamation-${SQLITE_VERSION}.zip"
    # SHOW_PROGRESS
    EXPECTED_HASH SHA256=${SQLITE_SHA256}
)
message(STATUS "[info] download completed, and extract.")

# `file(ARCHIVE_EXTRACT ...` require cmake 3.18.
# use the old way.
# file(ARCHIVE_EXTRACT
#    INPUT ${DOWNLOAD_DIR}/sqlite-amalgamation-${SQLITE_VERSION}.zip
#    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/extract
#    VERBOSE
# )
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/extract")
execute_process(COMMAND "${CMAKE_COMMAND}" -E tar xzf "${DOWNLOAD_DIR}/sqlite-amalgamation-${SQLITE_VERSION}.zip"
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/extract"
)

# compare (it doesn't compare all files in the directory, only the sqlite3.h file.)
execute_process(COMMAND "${CMAKE_COMMAND}" -E compare_files
    "${CMAKE_CURRENT_BINARY_DIR}/extract/sqlite-amalgamation-${SQLITE_VERSION}/sqlite3.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/sqlite/sqlite3.h"
    RESULT_VARIABLE COMPARE_RESULT
)
if(NOT COMPARE_RESULT EQUAL 0)
    message(STATUS "[info] sqlite source compare: diff -> copy.")
    file(COPY "${CMAKE_CURRENT_BINARY_DIR}/extract/sqlite-amalgamation-${SQLITE_VERSION}/sqlite3.h"    DESTINATION "${CMAKE_CURRENT_SOURCE_DIR}/sqlite/")
    file(COPY "${CMAKE_CURRENT_BINARY_DIR}/extract/sqlite-amalgamation-${SQLITE_VERSION}/sqlite3.c"    DESTINATION "${CMAKE_CURRENT_SOURCE_DIR}/sqlite/")
    file(COPY "${CMAKE_CURRENT_BINARY_DIR}/extract/sqlite-amalgamation-${SQLITE_VERSION}/sqlite3ext.h" DESTINATION "${CMAKE_CURRENT_SOURCE_DIR}/sqlite/")
else()
    message(STATUS "[info] sqlite source compare: same.")
endif()
file(REMOVE_RECURSE "${CMAKE_CURRENT_BINARY_DIR}/extract")

# build static sqlite3.lib.
add_library(sqlite3 STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/sqlite/sqlite3.c
    ${CMAKE_CURRENT_SOURCE_DIR}/sqlite/sqlite3.h
    ${CMAKE_CURRENT_SOURCE_DIR}/sqlite/sqlite3ext.h
)

# compile option.
# check it.
#     https://www.sqlite.org/compile.html
#     2. Recommended Compile-time Options
add_definitions(-DSQLITE_DQS=0)
add_definitions(-DSQLITE_DEFAULT_MEMSTATUS=0)
add_definitions(-DSQLITE_MAX_EXPR_DEPTH=0)
add_definitions(-DSQLITE_OMIT_DECLTYPE)
add_definitions(-DSQLITE_OMIT_DEPRECATED)
add_definitions(-DSQLITE_OMIT_PROGRESS_CALLBACK)
add_definitions(-DSQLITE_OMIT_SHARED_CACHE)
add_definitions(-DSQLITE_USE_ALLOCA)

# disable compile warning.
if(MSVC)
    target_compile_options(sqlite3 PRIVATE /W1)
else()
    target_compile_options(sqlite3 PRIVATE -Wall -Wformat=0)
endif()

# install.
install(FILES 
    "${SQLITE_INCLUDE_DIR}/sqlite/sqlite3.h"
    "${SQLITE_INCLUDE_DIR}/sqlite/sqlite3ext.h"
    DESTINATION include/sqlite
)
install(TARGETS sqlite3 LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)
