cmake_minimum_required(VERSION 3.10)

project(pairup VERSION 1.0)

set(CMAKE_C_STANDARD 99)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -g")

set(LIB_DIR ${CMAKE_SOURCE_DIR}/lib)
set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)
set(BUILD_DIR ${CMAKE_BINARY_DIR}/build)

include_directories(${LIB_DIR} ${SRC_DIR})

add_library(librwcsv STATIC ${LIB_DIR}/rw-csv.c)
add_library(libcjson STATIC ${LIB_DIR}/cJSON.c)

set(PAIRUP_SOURCES
    ${SRC_DIR}/pairup/pairup-algorithm.c
    ${SRC_DIR}/pairup/pairup-formatter.c
    ${SRC_DIR}/pairup/pairup-types.c
)

add_library(pairup STATIC ${PAIRUP_SOURCES})

set(MAIN_SOURCE ${SRC_DIR}/main.c)

set(VERSION_FILE ${SRC_DIR}/version.h)

# Call git to get the latest commit hash
execute_process(
    COMMAND git log --oneline --pretty=format:"%h" -n 1
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Call git to get the author name of the latest commit
execute_process(
    COMMAND git log --oneline --pretty=format:"%an" -n 1
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_AUTHOR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Set the version string with the retrieved Git commit hash
set(version_string ${GIT_HASH})

# Set the author name
set(author_name ${GIT_AUTHOR})

# Ensure the version string is passed as a quoted string to the C preprocessor
configure_file(
    ${CMAKE_SOURCE_DIR}/src/version.h.in  # version.h template
    ${VERSION_FILE}
    @ONLY
)

add_executable(main ${MAIN_SOURCE} ${VERSION_FILE})
target_link_libraries(main librwcsv libcjson pairup)
target_include_directories(main PUBLIC ${SRC_DIR})

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_custom_command(
    TARGET main POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:main> ${CMAKE_SOURCE_DIR}/pairup
    COMMENT "Copying executable to project root as 'pairup'"
)

add_custom_target(clean-all
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}
    COMMAND ${CMAKE_COMMAND} -E remove ${CMAKE_SOURCE_DIR}/pairup
    COMMENT "Cleaning build directory and root-level executable"
)

