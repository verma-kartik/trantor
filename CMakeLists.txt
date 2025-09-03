cmake_minimum_required(VERSION 3.20)
project(ormcpp VERSION 1.0.0 LANGUAGES CXX)

# Set C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set build type to Debug by default
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# Compiler-specific options
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    add_compile_options(-Wall -Wextra -Wpedantic)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Find SQLite3
find_package(PkgConfig REQUIRED)
pkg_check_modules(SQLITE3 REQUIRED sqlite3)

# Include directories
include_directories(include)
include_directories(${SQLITE3_INCLUDE_DIRS})

# Library source files (excluding trantor.cpp)
file(GLOB_RECURSE LIB_SOURCES "src/*.cpp")
list(REMOVE_ITEM LIB_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/trantor.cpp")

# Create the main library (only if there are library sources)
if(LIB_SOURCES)
    add_library(ormcpp_lib ${LIB_SOURCES})
    target_include_directories(ormcpp_lib PUBLIC include)
    target_compile_features(ormcpp_lib PUBLIC cxx_std_20)
    target_link_libraries(ormcpp_lib ${SQLITE3_LIBRARIES})
    target_compile_options(ormcpp_lib PRIVATE ${SQLITE3_CFLAGS_OTHER})
else()
    # Create a header-only interface library for now
    add_library(ormcpp_lib INTERFACE)
    target_include_directories(ormcpp_lib INTERFACE include)
    target_compile_features(ormcpp_lib INTERFACE cxx_std_20)
    target_link_libraries(ormcpp_lib INTERFACE ${SQLITE3_LIBRARIES})
endif()

# Create the main executable
add_executable(ormcpp src/trantor.cpp)
target_link_libraries(ormcpp ormcpp_lib ${SQLITE3_LIBRARIES})

# Enable testing
enable_testing()

# Fetch Google Test
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Add test subdirectory
add_subdirectory(tests)

# Print configuration summary
message(STATUS "ormcpp Configuration Summary:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Compiler: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "  SQLite3 version: ${SQLITE3_VERSION}")
message(STATUS "  SQLite3 libraries: ${SQLITE3_LIBRARIES}")
