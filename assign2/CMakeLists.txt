cmake_minimum_required(VERSION 3.1.0)
project(assign2)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# set(LLVM_DIR /usr/local/llvm-10)
set(CMAKE_C_FLAGS "-fno-rtti")
set(CMAKE_CXX_FLAGS "-fno-rtti")
set(CMAKE_BUILD_TYPE debug)
message(STATUS "LLVM_DIR: ${LLVM_DIR}")

find_package(LLVM REQUIRED CONFIG HINTS ${LLVM_DIR} ${LLVM_DIR}/lib/cmake/llvm
	                NO_DEFAULT_PATH)

include_directories(${LLVM_INCLUDE_DIRS} ${CLANG_INCLUDE_DIRS} SYSTEM)
link_directories(${LLVM_LIBRARY_DIRS})
message(STATUS "LLVM_INCLUDE_DIR : ${LLVM_INCLUDE_DIRS}")
message(STATUS "LLVM_LIB DIR : ${LLVM_LIBRARY_DIRS}")
set(LLVM_LINK_COMPONENTS
  LLVMCore
  LLVMIRReader
  LLVMPasses
  )

message(STATUS "LLVM LIBS : ${LLVM_LINK_COMPONENTS}")
# Support plugins.

add_executable(llvmassignment
  LLVMAssignment.cpp
  )

target_link_libraries(llvmassignment
	${LLVM_LINK_COMPONENTS}
	)
