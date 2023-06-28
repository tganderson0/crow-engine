# GLFW
include_directories(${GLFW_DIR}/include)

set(IMGUI_DIR ../)
# include_directories(${IMGUI_DIR} ${IMGUI_DIR}/backends ..)

find_package(Vulkan REQUIRED)
set(LIBRARIES "glfw;Vulkan::Vulkan")

include_directories(${GLFW_DIR}/deps)
file(GLOB sources *.cpp)

