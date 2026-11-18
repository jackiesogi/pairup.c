cmake_minimum_required(VERSION 3.15)

# CXX is needed only when HiGHS (a C++ library) is built from source
project(pairup VERSION 1.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 99)
if(NOT MSVC)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -g")
endif()

set(LIB_DIR ${CMAKE_SOURCE_DIR}/lib)
set(API_LIB_DIR ${CMAKE_SOURCE_DIR}/src/api)
set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)
set(BUILD_DIR ${CMAKE_BINARY_DIR}/build)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ---------------------------------------------------------------------------
# HiGHS (ILP solver)
#   1. Use an installed copy (e.g. `brew install highs`, or set
#      CMAKE_PREFIX_PATH / highs_DIR to its install prefix).
#   2. Otherwise download and build it (needs a C++ compiler and network).
# ---------------------------------------------------------------------------
option(PAIRUP_FETCH_HIGHS "Download and build HiGHS when it is not installed" ON)
set(PAIRUP_HIGHS_VERSION "1.16.0")

find_package(highs CONFIG QUIET)
if(highs_FOUND)
    message(STATUS "Using installed HiGHS ${highs_VERSION}")
elseif(PAIRUP_FETCH_HIGHS)
    message(STATUS "HiGHS not found, building v${PAIRUP_HIGHS_VERSION} from source")
    include(FetchContent)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(highs
        GIT_REPOSITORY https://github.com/ERGO-Code/HiGHS.git
        GIT_TAG        v${PAIRUP_HIGHS_VERSION}
        GIT_SHALLOW    TRUE)
    FetchContent_MakeAvailable(highs)
    if(NOT TARGET highs::highs)
        add_library(highs::highs ALIAS highs)
    endif()
else()
    message(FATAL_ERROR "HiGHS not found. Install it or configure with -DPAIRUP_FETCH_HIGHS=ON")
endif()

include_directories(${LIB_DIR} ${SRC_DIR} ${API_LIB_DIR})

add_library(librwcsv STATIC ${LIB_DIR}/rw-csv.c)
add_library(libcjson STATIC ${LIB_DIR}/cJSON.c)
add_library(libgetopt STATIC ${LIB_DIR}/getopt.c)

set(PAIRUP_SOURCES
    # Internal
    ${SRC_DIR}/pairup/pairup.c
    ${SRC_DIR}/pairup/pairup-problem.c
    ${SRC_DIR}/pairup/pairup-model.c
    ${SRC_DIR}/pairup/pairup-solver.c
    ${SRC_DIR}/pairup/pairup-formatter.c
    ${SRC_DIR}/pairup/pairup-types.c
    # API
    ${SRC_DIR}/api/libpairup.c
)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
add_library(libpairup STATIC ${PAIRUP_SOURCES})
target_link_libraries(libpairup PUBLIC highs::highs librwcsv libcjson)
# A static HiGHS pulls in the C++ runtime, so link with the C++ driver
set_target_properties(libpairup PROPERTIES LINKER_LANGUAGE CXX)

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
target_link_libraries(main libpairup libgetopt)
target_include_directories(main PUBLIC ${SRC_DIR})
set_target_properties(main PROPERTIES LINKER_LANGUAGE CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if(WIN32)
    set(PAIRUP_EXE pairup.exe)
else()
    set(PAIRUP_EXE pairup)
endif()

add_custom_command(
    TARGET main POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            $<TARGET_FILE:main>
            ${CMAKE_SOURCE_DIR}/${PAIRUP_EXE}
    COMMENT "Copying executable to project root as ${PAIRUP_EXE}"
)

add_custom_target(clean-all
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}
    COMMAND ${CMAKE_COMMAND} -E remove ${CMAKE_SOURCE_DIR}/${PAIRUP_EXE}
    COMMENT "Cleaning build directory and root-level executable"
)

# ---------------------------------------------------------------------------
# Tests: `ctest --test-dir build`
# ---------------------------------------------------------------------------
option(PAIRUP_BUILD_TESTS "Build the solver tests" ON)
if(PAIRUP_BUILD_TESTS)
    enable_testing()
    add_executable(test_solver ${CMAKE_SOURCE_DIR}/tests/test_solver.c)
    target_link_libraries(test_solver libpairup)
    set_target_properties(test_solver PROPERTIES LINKER_LANGUAGE CXX)
    add_test(NAME solver_matches_brute_force COMMAND test_solver)
endif()
