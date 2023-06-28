cmake_minimum_required(VERSION 3.12)
project(CrowEngine)



#
# Manually specifying all the header files.
#
set(HEADER_FILES
	src/vk_engine.hpp
	src/vk_initializers.hpp
	src/vk_types.hpp
	src/vk_mesh.hpp
	src/vk_textures.hpp
)

#
# Manually specifying all the source files.
#
set(SOURCE_FILES
	src/vk_engine.cpp
	src/vk_initializers.cpp
	src/vk_mesh.cpp
	src/vk_textures.cpp
)

# set(IMGUI_SOURCES
# 	external/imgui/imgui.cpp
# 	external/imgui/imgui_draw.cpp
# 	external/imgui/imgui_widgets.cpp
# )

set(src_files_imgui
  external/imgui/imgui.cpp  
  external/imgui/imgui_draw.cpp
  external/imgui/imgui_tables.cpp
  external/imgui/imgui_widgets.cpp
  external/imgui/backends/imgui_impl_glfw.cpp
  external/imgui/backends/imgui_impl_vulkan.cpp
  external/imgui/imgui_demo.cpp
)

source_group("ImGui" FILES ${src_files_imgui})


set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

add_subdirectory(external/glfw)
add_subdirectory(external/vk-bootstrap)
add_subdirectory(external)
#
# This is the main target
#
add_executable(CrowEngine ${HEADER_FILES} ${SOURCE_FILES} ${src_files_imgui} src/main.cpp)
target_include_directories(CrowEngine PRIVATE external/imgui)

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
target_include_directories(CrowEngine PRIVATE external/imgui)


find_package(Vulkan REQUIRED)
target_link_libraries(CrowEngine Vulkan::Vulkan)




file(GLOB_RECURSE GLSL_SOURCE_FILES
    "${PROJECT_SOURCE_DIR}/shaders/*.frag"
    "${PROJECT_SOURCE_DIR}/shaders/*.vert"
    "${PROJECT_SOURCE_DIR}/shaders/*.comp"
    )

foreach(GLSL ${GLSL_SOURCE_FILES})
  message(STATUS "BUILDING SHADER")
  get_filename_component(FILE_NAME ${GLSL} NAME)
  set(SPIRV "${PROJECT_SOURCE_DIR}/shaders/${FILE_NAME}.spv")
  message(STATUS ${GLSL})
  add_custom_command(
    OUTPUT ${SPIRV}
    COMMAND ${GLSL_VALIDATOR} -V ${GLSL} -o ${SPIRV}
    DEPENDS ${GLSL})
  list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)

add_custom_target(
    Shaders 
    DEPENDS ${SPIRV_BINARY_FILES}
    )

add_custom_target(copy_assets
COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_LIST_DIR}/copy-assets.cmake
)
add_dependencies(CrowEngine copy_assets)


# target_sources(imgui PRIVATE 
#   external/imgui/imgui.cpp  
#   external/imgui/imgui_draw.cpp
#   external/imgui/imgui_tables.cpp
#   external/imgui/imgui_widgets.cpp
#   external/imgui/imgui_impl_glfw.cpp
#   external/imgui/imgui_impl_vulkan.cpp
#   external/imgui/imgui_demo.cpp
# )