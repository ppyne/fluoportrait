cmake_minimum_required(VERSION 3.20)
project(fluoportrait VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(PNG REQUIRED)
find_package(JPEG REQUIRED)

add_library(fluoportrait_core
  src/color_science.cpp
  src/image_processor.cpp
  src/png_loader.cpp
  src/icc_profile.cpp
  src/jpeg_io.cpp)
target_include_directories(fluoportrait_core PUBLIC include)
target_link_libraries(fluoportrait_core PUBLIC PNG::PNG JPEG::JPEG)
target_compile_options(fluoportrait_core PRIVATE -Wall -Wextra -Wpedantic)

add_executable(fluoportrait src/main.cpp)
target_link_libraries(fluoportrait PRIVATE fluoportrait_core)

include(CTest)
if(BUILD_TESTING)
  add_executable(fluoportrait_tests tests/tests.cpp)
  target_link_libraries(fluoportrait_tests PRIVATE fluoportrait_core)
  add_test(NAME fluoportrait_tests COMMAND fluoportrait_tests)
endif()

