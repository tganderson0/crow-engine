cmake_minimum_required(VERSION 3.12)
project(CrowEngine)



#
# Manually specifying all the header files.
#
set(HEADER_FILES
)

#
# Manually specifying all the source files.
#
set(SOURCE_FILES
)


#
# This is the main target
#
add_executable(CrowEngine ${HEADER_FILES} ${SOURCE_FILES} src/main.cpp)


#
# We want the C++ 20 standard for our project
#
set_property(TARGET CrowEngine PROPERTY CXX_STANDARD 20)

#
# Enable a lot of warnings for both compilers, forcing the developer to write better code
#
if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
	target_compile_options(CrowEngine PRIVATE /W4 /permissive-)  #/WX will treat warnings as errors
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	target_compile_options(CrowEngine PRIVATE -Wall -Wextra -pedantic -O3) # -Wconversion -Wsign-conversion
endif()