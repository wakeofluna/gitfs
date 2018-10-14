cmake_minimum_required(VERSION 2.8.11)

project ( gitfs C )

if (NOT DEFINED CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build")
endif()

# message("Compiler ID : ${CMAKE_C_COMPILER_ID}")

if (CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
	set(CMAKE_C_FLAGS "-pipe -Wall")
endif()

add_subdirectory(src)
