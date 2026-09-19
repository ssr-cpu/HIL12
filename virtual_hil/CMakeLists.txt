cmake_minimum_required(VERSION 3.16)
project(virtual_hil C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

file(GLOB APP_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c)
list(REMOVE_ITEM APP_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/main.c)
file(GLOB TEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.c)

add_executable(hil ${CMAKE_CURRENT_SOURCE_DIR}/src/main.c ${APP_SOURCES})
target_include_directories(hil PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_options(hil PRIVATE
  -Wall -Wextra -Wconversion -Wshadow -Wpedantic)

add_executable(hil_tests ${TEST_SOURCES} ${APP_SOURCES})
target_include_directories(hil_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_options(hil_tests PRIVATE
  -Wall -Wextra -Wconversion -Wshadow -Wpedantic)

include(CTest)
if(BUILD_TESTING)
  add_test(NAME hil_unit_and_e2e COMMAND hil_tests)
endif()

option(HIL_ENABLE_SANITIZERS "Build with AddressSanitizer and UBSan" OFF)
if(HIL_ENABLE_SANITIZERS)
  target_compile_options(hil PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
  target_link_options(hil PRIVATE -fsanitize=address,undefined)
  target_compile_options(hil_tests PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
  target_link_options(hil_tests PRIVATE -fsanitize=address,undefined)
endif()
