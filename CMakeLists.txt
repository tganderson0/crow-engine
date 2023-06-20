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

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

add_subdirectory(external/glfw)


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

target_link_libraries(CrowEngine glfw)

target_include_directories(CrowEngine PRIVATE external/tiny_obj_loader)
target_include_directories(CrowEngine PRIVATE external/stb_image)

find_package(Vulkan REQUIRED)
target_link_libraries(CrowEngine Vulkan::Vulkan)

add_custom_target(copy_assets
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_LIST_DIR}/copy-assets.cmake
)
add_dependencies(CrowEngine copy_assets)