cmake_minimum_required(VERSION 3.13)

option(COLOR_OUTPUT "Color build output" ON)
option(ADDRESS_SANITIZER "Enable Address Sanitizer" OFF)

if(${COLOR_OUTPUT})
  add_compile_options(-fdiagnostics-color=always)
endif()

if(${ADDRESS_SANITIZER})
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address\
                                      -fno-omit-frame-pointer\
                                      -fsanitize-address-use-after-scope\
                                      -fno-optimize-sibling-calls")
endif()

project(open-reckless-drivin)

find_package(SDL2 REQUIRED)

# set(EXECUTABLE_OUTPUT_PATH ..)

add_subdirectory(src)

# Tests

option(TESTS "Enable testing" ON)

if(${TESTS})
  enable_testing()
  add_subdirectory(tests)
endif()
