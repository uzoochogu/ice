cmake_minimum_required (VERSION 3.8)

project (ice)

set(CMAKE_CXX_STANDARD 20)

# Vulkan project
# Set your Vulkan ENV variable.
# set(ENV{VULKAN_SDK} "C:/dev/VulkanSDK")
#include_directories(${VULKAN_SDK}/include/)

# Imgui Dependencies
find_package(glfw3 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)

# Vulkan and Utils
find_package(Vulkan REQUIRED)
find_package(glm REQUIRED)
find_package(Stb REQUIRED)
find_package(tinyobjloader REQUIRED)

message(STATUS "Directory: ${Vulkan_INCLUDE_DIRS}")
message(STATUS "Directory: ${Vulkan_LIBRARIES}")

file(GLOB_RECURSE SOURCES ${PROJECT_SOURCE_DIR}/src/*.cpp)

# add_executable(${PROJECT_NAME} ${SOURCES})
add_executable (first_app ${SOURCES})

foreach( target IN ITEMS first_app)
  message(STATUS "Configuring target: ${target}")
  target_include_directories(${target} PRIVATE ${CMAKE_SOURCE_DIR}/src)
  target_link_libraries(${target} PRIVATE glfw)
  target_link_libraries(${target} PRIVATE imgui::imgui)
  target_include_directories(${target} PRIVATE ${Vulkan_INCLUDE_DIRS})
  target_link_libraries(${target} PRIVATE ${Vulkan_LIBRARIES})
  target_link_libraries(${target} PRIVATE glm::glm)
endforeach()

set(source "${CMAKE_SOURCE_DIR}/resources") 
set(destination "${CMAKE_CURRENT_BINARY_DIR}/resources") # or CMAKE_BINARY_DIR
set( vulkan_apps  first_app )

if(MSVC) 
  # Needed in Visual Studio to enable file access to non-local paths
  # TargetDir is a VS macro containing path to executable directory 
  set_target_properties(${vulkan_apps} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "$(TargetDir)" )

  # Copy compiled shaders to executable directory
  # Add post-build commands
  add_custom_command(TARGET ${vulkan_apps} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/resources
        "$(TargetDir)/resources" COMMENT
        "copied resource folder from ${source} => ${destination}"
  )
elseif(UNIX)
  # For non-MSVC (Linux, macOS, etc.)
  add_custom_command(TARGET ${vulkan_apps}
  COMMAND ${CMAKE_COMMAND} -E create_symlink
  ${source} ${destination} DEPENDS ${
    destination} COMMENT
  "symbolic link resource folder from ${source} => ${destination}"
  )
else()
  # windows might not use MSVC
  add_custom_command(TARGET ${vulkan_apps} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
      ${source}
      ${destination} COMMENT
      "copied resource folder from ${source} => ${destination}"
  )
endif() 
