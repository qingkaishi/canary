cmake_minimum_required(VERSION 3.10.0)
project(canary C CXX)

unset(LLVMCONFIG CACHE)

# find_program(LLVMCONFIG llvm-config)
# if (NOT LLVMCONFIG)
# message(STATUS "llvm-config not found!")
# find_program(LLVMCONFIG llvm-config-12)
# if (NOT LLVMCONFIG)
# message(FATAL_ERROR "llvm-config-12 not found!")
# else ()
# message(STATUS "llvm-config-12 found!")
# endif ()
# endif ()
set(LLVMCONFIG "/home/yl/LLVM/clang+llvm-12.0.1-x86_64-linux-gnu-ubuntu-/bin/llvm-config")
execute_process(COMMAND ${LLVMCONFIG} --cxxflags OUTPUT_VARIABLE CMAKE_CXX_FLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND ${LLVMCONFIG} --cflags OUTPUT_VARIABLE CMAKE_C_FLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND ${LLVMCONFIG} --cppflags OUTPUT_VARIABLE CMAKE_CPP_FLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND ${LLVMCONFIG} --ldflags OUTPUT_VARIABLE CMAKE_EXE_LINKER_FLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND ${LLVMCONFIG} --system-libs OUTPUT_VARIABLE LIBS OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND ${LLVMCONFIG} --includedir OUTPUT_VARIABLE LLVM_INCLUDE_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${LIBS} -lz3")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include_directories(${LLVM_INCLUDE_DIRS})
include_directories(AFTER ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_subdirectory(lib)
add_subdirectory(tools)
add_subdirectory(benchmarks)


