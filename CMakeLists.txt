cmake_minimum_required(VERSION 3.12)
project(CrowEngine)



# Add source files
file(GLOB_RECURSE SOURCE_FILES
  ${CMAKE_SOURCE_DIR}/src/*.c
  ${CMAKE_SOURCE_DIR}/src/*.cpp
)

# Add header files
file(GLOB_RECURSE HEADER_FILES
  ${CMAKE_SOURCE_DIR}/src/*.h
  ${CMAKE_SOURCE_DIR}/src/*.hpp
)

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
target_include_directories(CrowEngine PRIVATE external/imgui)

find_package(Vulkan REQUIRED)
target_link_libraries(CrowEngine Vulkan::Vulkan)

# Create virtual folders to make it look nicer in VS
if(MSVC_IDE)
	# Macro to preserve source files hierarchy in the IDE
	macro(GroupSources curdir)
		file(GLOB children RELATIVE ${PROJECT_SOURCE_DIR}/${curdir} ${PROJECT_SOURCE_DIR}/${curdir}/*)

		foreach(child ${children})
			if(IS_DIRECTORY ${PROJECT_SOURCE_DIR}/${curdir}/${child})
				GroupSources(${curdir}/${child})
			else()
				string(REPLACE "/" "\\" groupname ${curdir})
				string(REPLACE "src" "Sources" groupname ${groupname})
				source_group(${groupname} FILES ${PROJECT_SOURCE_DIR}/${curdir}/${child})
			endif()
		endforeach()
	endmacro()

	# Run macro
	GroupSources(src)
endif()

add_custom_target(copy_assets
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_LIST_DIR}/copy-assets.cmake
)
add_dependencies(CrowEngine copy_assets)