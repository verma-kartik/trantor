# Test configuration
file(GLOB_RECURSE TEST_SOURCES "*.cpp")

# Create test executable
add_executable(ormcpp_tests ${TEST_SOURCES})

# Link with Google Test and our library
target_link_libraries(ormcpp_tests 
    ormcpp_lib
    gtest
    gtest_main
    pthread
)

# Set C++20 standard for tests
target_compile_features(ormcpp_tests PUBLIC cxx_std_20)

# Include directories for tests
target_include_directories(ormcpp_tests PRIVATE 
    ${CMAKE_SOURCE_DIR}/include
)

# Add tests to CTest
include(GoogleTest)
gtest_discover_tests(ormcpp_tests)

# Custom test target for convenience
add_custom_target(run_tests
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/ormcpp_tests
    DEPENDS ormcpp_tests
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
