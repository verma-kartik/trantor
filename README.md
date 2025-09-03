# OrmCpp - Modern C++20 ORM Library

[![Build and Test](https://github.com/verma-kartik/trantor/actions/workflows/build.yml/badge.svg)](https://github.com/verma-kartik/trantor/actions/workflows/build.yml)
[![Debug Build](https://github.com/verma-kartik/trantor/actions/workflows/debug.yml/badge.svg)](https://github.com/verma-kartik/trantor/actions/workflows/debug.yml)
[![Code Quality](https://github.com/verma-kartik/trantor/actions/workflows/quality.yml/badge.svg)](https://github.com/verma-kartik/trantor/actions/workflows/quality.yml)

A modern Object-Relational Mapping (ORM) library for C++20, designed to provide type-safe database operations with compile-time validation.

## Features

- **C++20 Modern Features**: Leverages concepts, constexpr if, and other modern C++ features
- **Type Safety**: Compile-time type checking for database operations
- **Header-Only Design**: Easy integration into existing projects (planned)
- **Cross-Platform**: Works on Linux, macOS, and Windows
- **Comprehensive Testing**: Google Test integration for reliable development

## Project Structure

```
ormcpp/
├── CMakeLists.txt          # Main CMake configuration
├── README.md              # This file
├── src/                   # Source files
│   └── main.cpp          # Example/demo application
├── include/               # Header files
│   └── ormcpp/           # Library headers (to be added)
├── tests/                 # Test files
│   ├── CMakeLists.txt    # Test configuration
│   └── basic_test.cpp    # Basic tests
├── docs/                  # Documentation
├── examples/              # Usage examples
└── build/                 # Build directory (generated)
```

## Prerequisites

- **CMake** 3.20 or higher
- **GCC 11+** or **Clang 12+** (C++20 support required)
- **Git** (for dependency management)
- **Internet connection** (for downloading Google Test)

## Getting Started

### 1. Clone the Repository

```bash
git clone <your-repo-url>
cd ormcpp
```

### 2. Build the Project

```bash
# Create build directory
mkdir -p build
cd build

# Configure with CMake
cmake ..

# Build the project
make

# Or use cmake to build
cmake --build .
```

### 3. Run the Application

```bash
# Run the main executable
./ormcpp
```

### 4. Run Tests

```bash
# Run tests directly
./tests/ormcpp_tests

# Or use CTest for detailed output
ctest --verbose

# Run specific test
ctest -R BasicTest
```

## Build Options

### Debug Build (Default)
```bash
cmake -DCMAKE_BUILD_TYPE=Debug ..
make
```

### Release Build
```bash
cmake -DCMAKE_BUILD_TYPE=Release ..
make
```

### Build with Specific Compiler
```bash
# Using GCC
cmake -DCMAKE_CXX_COMPILER=g++ ..

# Using Clang
cmake -DCMAKE_CXX_COMPILER=clang++ ..
```

## Development Workflow

### Adding New Features

1. **Create header files** in `include/ormcpp/`
2. **Create source files** in `src/` (if not header-only)
3. **Add tests** in `tests/`
4. **Update CMakeLists.txt** if needed
5. **Build and test**

### Testing

The project uses Google Test for unit testing:

```bash
# Add new test file in tests/
touch tests/my_new_test.cpp

# CMake will automatically detect and compile new test files
cd build && make

# Run all tests
./tests/ormcpp_tests
```

### Example Test Structure

```cpp
#include <gtest/gtest.h>
#include "ormcpp/your_header.hpp"

TEST(YourTestSuite, TestName) {
    // Your test code
    EXPECT_EQ(expected, actual);
}
```

## C++20 Features Used

This project leverages modern C++20 features:

- **Concepts**: Type constraints for template parameters
- **constexpr if**: Compile-time conditional compilation
- **Template improvements**: Enhanced template argument deduction
- **Modules**: (Planned for future versions)

### Example Usage (Planned)

```cpp
#include <ormcpp/orm.hpp>

// Define your entity
class User : public ormcpp::Entity {
public:
    int id;
    std::string name;
    std::string email;
    
    std::string getTableName() const override {
        return users;
    }
};

// Use the ORM
int main() {
    ormcpp::Repository<User> userRepo(sqlite:///users.db);
    
    // Find users
    auto users = userRepo.findAll();
    
    // Find by condition
    auto admins = userRepo.findBy(role, admin);
    
    return 0;
}
```

## Compiler Support

| Compiler | Minimum Version | Linux | macOS | Status |
|----------|-----------------|-------|-------|--------|
| GCC      | 11.0           | ✅     | ✅     | Fully Tested |
| Clang    | 12.0           | ✅     | ✅     | Fully Tested |
| MSVC     | 19.29          | ❌     | ❌     | Not Supported |

### CI/CD Pipeline

This project uses GitHub Actions for continuous integration with the following build matrix:
- **Linux (Ubuntu Latest)**: GCC 11, Clang 14
- **macOS (Latest)**: GCC 11, Clang (system)
- **Build Types**: Release and Debug modes
- **Architecture**: 64-bit only
- **Quality Checks**: Static analysis with clang-tidy, memory checks with Valgrind (Linux)

## CMake Targets

| Target        | Description                    |
|---------------|--------------------------------|
| `ormcpp`     | Main executable               |
| `ormcpp_lib` | Library (static/interface)    |
| `ormcpp_tests` | Test executable             |
| `run_tests`  | Custom target to run tests   |

## Configuration Summary

The build system provides a configuration summary:

```
-- ormcpp Configuration Summary:
--   Version: 1.0.0
--   Build type: Debug
--   C++ standard: 20
--   Compiler: GNU
```

## Troubleshooting

### Common Issues

1. **CMake version too old**:
   - Update CMake to 3.20 or higher

2. **C++20 not supported**:
   - Update compiler (GCC 11+, Clang 12+)

3. **Google Test download fails**:
   - Check internet connection
   - May need to configure proxy if behind corporate firewall

4. **Build fails with template errors**:
   - Ensure you're using a C++20 compatible compiler
   - Check that `-std=c++20` is being used

### Debugging Build Issues

```bash
# Verbose build output
make VERBOSE=1

# Clean rebuild
rm -rf build
mkdir build && cd build
cmake .. && make

# Check compiler version
g++ --version
cmake --version
```

## Contributing

1. Fork the repository
2. Create a feature branch: `git checkout -b feature/amazing-feature`
3. Make your changes
4. Add tests for new functionality
5. Run tests: `ctest`
6. Commit changes: `git commit -m 'Add amazing feature'`
7. Push to branch: `git push origin feature/amazing-feature`
8. Open a Pull Request

## Development Environment

This project is developed using:
- **Docker**: For consistent development environment
- **CLion**: IDE with remote development support
- **GCC 11**: Primary compiler for C++20 features

## License

[Add your license here]

## Roadmap

- [ ] Basic ORM functionality
- [ ] SQLite backend
- [ ] PostgreSQL backend
- [ ] MySQL backend
- [ ] Query builder
- [ ] Migrations support
- [ ] Connection pooling
- [ ] Header-only conversion

## Changelog

### v1.0.0 (Current)
- Initial project setup
- CMake configuration with C++20
- Google Test integration
- Basic project structure
