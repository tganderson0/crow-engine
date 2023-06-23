cmake_minimum_required(VERSION 3.12)
project(CrowEngine)



#
# Manually specifying all the header files.
#
set(HEADER_FILES
	src/vk_engine.hpp
	src/vk_initializers.hpp
	src/vk_types.hpp
)

#
# Manually specifying all the source files.
#
set(SOURCE_FILES
	src/vk_engine.cpp
	src/vk_initializers.cpp
)

# set(IMGUI_SOURCES
# 	external/imgui/imgui.cpp
# 	external/imgui/imgui_draw.cpp
# 	external/imgui/imgui_widgets.cpp
# )

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

add_subdirectory(external/glfw)
add_subdirectory(external/vk-bootstrap)

#
# This is the main target
#
add_executable(CrowEngine ${HEADER_FILES} ${SOURCE_FILES} src/main.cpp)
# target_include_directories(CrowEngine PRIVATE external/imgui)

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
target_link_libraries(CrowEngine vk-bootstrap)

target_include_directories(CrowEngine PRIVATE external/tiny_obj_loader)
target_include_directories(CrowEngine PRIVATE external/stb_image)
target_include_directories(CrowEngine PRIVATE external/vk_mem_alloc)

find_package(Vulkan REQUIRED)
target_link_libraries(CrowEngine Vulkan::Vulkan)

add_custom_target(copy_assets
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_LIST_DIR}/copy-assets.cmake
)
add_dependencies(CrowEngine copy_assets)