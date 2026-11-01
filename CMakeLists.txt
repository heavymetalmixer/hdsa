cmake_minimum_required(VERSION 3.27)

# Select the C standard and compiler
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED True)
set(CMAKE_C_COMPILER gcc) # gcc, clang or cl. For clang use -fuse-ld=lld as well
set(CMAKE_C_EXTENSIONS OFF)

# The option -DNDEBUG disables all standard C asserts, and -DHDSA_DISABLE_ASSERTS disables the HDSA asserts
set(CMAKE_C_FLAGS_DEBUG "-v -ftime-report -fdiagnostics-color=always -m64 -O0 -g3 -Wall -Wextra -Wstrict-prototypes -Wconversion -Warith-conversion -Wsign-conversion -Wsign-compare -Wuninitialized -Wunused -Wunused-parameter -Wshadow -Wpedantic -Winline -std=c${CMAKE_C_STANDARD}" CACHE STRING "" FORCE)
set(CMAKE_C_FLAGS_RELWITHDEBINFO "-v -ftime-report -fdiagnostics-color=always -m64 -Wall -O1 -g3 -Wextra -Wstrict-prototypes -Wconversion -Warith-conversion -Wsign-conversion -Wsign-compare -Wuninitialized -Wunused -Wunused-parameter -Wshadow -Wpedantic -Winline -std=c${CMAKE_C_STANDARD}" CACHE STRING "" FORCE)
set(CMAKE_C_FLAGS_RELEASE "-v -ftime-report -fdiagnostics-color=always -m64 -DHDSA_DISABLE_ASSERTS -O3 -g0 -Wall -Wextra -Wstrict-prototypes -Wconversion -Warith-conversion -Wsign-conversion -Wsign-compare -Wuninitialized -Wunused -Wunused-parameter -Wshadow -Wpedantic -Winline -std=c${CMAKE_C_STANDARD}" CACHE STRING "" FORCE)

# Select the C++ standard and compiler
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_COMPILER g++) # g++, clang++ or cl
set(CMAKE_CXX_EXTENSIONS OFF)

# set(CMAKE_CXX_FLAGS_DEBUG "-v -fdiagnostics-color=always -m64 -O0 -g3 -Wall -Wextra -Wconversion -Warith-conversion -Wsign-conversion -Wsign-compare -Wuninitialized -Wunused -Wunused-parameter -Wshadow -Wpedantic -lstdc++exp -std=c++${CMAKE_CXX_STANDARD}" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS_DEBUG "-ftime-report -fdiagnostics-color=always -m64 -O0 -g3 -Wall -Wextra -Wstrict-prototypes -Wconversion -Warith-conversion -Wsign-conversion -Wsign-compare -Wuninitialized -Wunused -Wunused-parameter -Wshadow -Wpedantic -Winline -std=c++${CMAKE_CXX_STANDARD}" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-ftime-report -fdiagnostics-color=always -m64 -Wall -O1 -g3 -Wextra -Wstrict-prototypes -Wconversion -Warith-conversion -Wsign-conversion -Wsign-compare -Wuninitialized -Wunused -Wunused-parameter -Wshadow -Wpedantic -Winline -std=c++${CMAKE_CXX_STANDARD}" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS_RELEASE "-ftime-report -fdiagnostics-color=always -m64 -DHDSA_DISABLE_ASSERTS -O3 -g0 -Wall -Wextra -Wstrict-prototypes -Wconversion -Warith-conversion -Wsign-conversion -Wsign-compare -Wuninitialized -Wunused -Wunused-parameter -Wshadow -Wpedantic -Winline -std=c++${CMAKE_CXX_STANDARD}" CACHE STRING "" FORCE)

project(HDSA VERSION 0.1.0 LANGUAGES CXX C)

# If the user doesn't provide a build type, choose Debug by default
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
endif()

if(CMAKE_BUILD_TYPE)
    message("\nUsing configuraton generation with the ${CMAKE_BUILD_TYPE} configuration type!\n")
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY VALUE "${CMAKE_BUILD_TYPE}")
else()
    message("Defaulting to Debug build.\n")
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY VALUE "Debug")
endif()

option(BUILD_SHARED_LIBS "Enable compilation of shared/dynamic libraries" OFF)
option(ENABLE_TESTING "Enable testing builds" OFF)

# C++ files to link
# set(SOURCE_FILES
#     src/hdsa.cpp)

# C files to link
set(SOURCE_FILES
    src/c/common.c
    src/c/virtual_alloc.c
    src/c/arena_alloc.c
    src/c/stack_alloc.c
    src/c/dynamic_array.c
    src/hdsa.c)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

# Necessary for some C++ 23 and 26 features
target_link_libraries(${PROJECT_NAME} PUBLIC stdc++exp)

# set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_SOURCE_DIR})

set_target_properties(
    ${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin/debug/x64
    ${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/bin/relwithdebinfo/x64
    ${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin/release/x64
)
