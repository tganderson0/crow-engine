
# Add source to this project's executable.
add_executable (engine 
  main.cpp
  vk_types.h
  vk_initializers.cpp
  vk_initializers.h
  vk_images.h
  vk_images.cpp 
  vk_descriptors.h
  vk_descriptors.cpp
  vk_pipelines.h
  vk_pipelines.cpp
  vk_engine.h
  vk_engine.cpp
  vk_loader.h
  vk_loader.cpp
  camera.cpp
  camera.h
  networker.h
  networker.cpp
  vk_remote.cpp
  vk_remote.h
  toojpeg.h
  toojpeg.cpp
)

set_property(TARGET engine PROPERTY CXX_STANDARD 20)
target_compile_definitions(engine PUBLIC GLM_FORCE_DEPTH_ZERO_TO_ONE)
target_include_directories(engine PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(engine PRIVATE ${JPEG_INCLUDE_DIR})
target_link_libraries(engine PRIVATE ${JPEG_LIBRARIES})

find_package(Boost 1.82 REQUIRED)
include_directories(${Boost_INCLUDE_DIR})

find_package(libjpeg-turbo CONFIG REQUIRED)
target_link_libraries(engine PRIVATE $<IF:$<TARGET_EXISTS:libjpeg-turbo::turbojpeg>,libjpeg-turbo::turbojpeg,libjpeg-turbo::turbojpeg-static>)

target_link_libraries(engine PUBLIC vma glm Vulkan::Vulkan fmt::fmt stb_image stb_image_write SDL2::SDL2 SDL2_image vkbootstrap imgui fastgltf::fastgltf ${Boost_LIBRARIES})

target_precompile_headers(engine PUBLIC <optional> <vector> <memory> <string> <vector> <unordered_map> <glm/mat4x4.hpp>  <glm/vec4.hpp> <vulkan/vulkan.h>)

add_custom_command(TARGET engine POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:engine> $<TARGET_FILE_DIR:engine>
  COMMAND_EXPAND_LISTS
  )