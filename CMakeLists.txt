cmake_minimum_required(VERSION 3.10)

# Project Name
set(NAME vulkan_game)
project(${NAME})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
#set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")

# Select all Files in src/
#file(GLOB_RECURSE SOURCE_FILES src/*.cpp src/*.h)

set(SOURCE_FILES src/main.cpp src/vulkan_base/vulkan_swapchain.cpp src/vulkan_base/vulkan_renderpass.cpp src/game_engine/worldmanager.cpp
src/vulkan_base/vulkan_pipeline.cpp src/vulkan_base/vulkan_utils.cpp src/game_engine/block.cpp src/app.cpp src/game_engine/chunk.cpp
src/game_engine/window.cpp src/vulkan_base.cpp src/vulkan_base/vulkan_creates.cpp src/vulkan_base/vulkan_render.cpp src/game_engine/cameramanager.cpp)

# Find SDL2
add_subdirectory(libs/SDL)

# Find Vulkan
find_package(Vulkan REQUIRED)

# CONVERT GLSL INTO SVP
#if (UNIX)
#add_custom_target(build_shaders ALL
#    COMMAND "${PROJECT_SOURCE_DIR}/shaders/compile.sh"
#    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/shaders"
#)
#endif(UNIX)

#if (WIN32)
#add_custom_target(build_shaders ALL
#    COMMAND "${PROJECT_SOUmmRCE_DIR}/shaders/compile.bat"
#    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/shaders"
#)
#endif(WIN32)

if (WIN32)
add_custom_target(build_shaders ALL
    #COMMAND ${CMAKE_COMMAND} -E echo "Running shader compilation script..."
    COMMAND ${PROJECT_SOURCE_DIR}/shaders/compile.bat
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/shaders
    #COMMAND ${CMAKE_COMMAND} -E echo "Shader compilation script finished."
)
endif(WIN32)

# ${NAME} (vulkan_game) executable
add_executable(${NAME} ${SOURCE_FILES})
add_dependencies(${NAME} build_shaders)
target_include_directories(${NAME} PUBLIC libs/SDL/include)
target_include_directories(${NAME} PUBLIC libs)
target_link_libraries(${NAME} PUBLIC SDL2-static)
target_include_directories(${NAME} PUBLIC ${Vulkan_INCLUDE_DIRS})
target_link_libraries(${NAME} PUBLIC ${Vulkan_LIBRARIES})