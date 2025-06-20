cmake_minimum_required (VERSION 3.28)

project (ice)

set(CMAKE_CXX_STANDARD 20)

# windowing
find_package(glfw3 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)

# Vulkan and Utils
find_package(Vulkan REQUIRED)
find_package(glm REQUIRED)
find_path(STB_INCLUDE_DIRS "stb_image.h")
find_path(TINYGLTF_INCLUDE_DIRS "tiny_gltf.h")

# including here so you can override cmake variables through .env.cmake
include(./.env.cmake OPTIONAL RESULT_VARIABLE LOCAL_ENV)
message(STATUS "Local .env.cmake:   ${LOCAL_ENV}")

message(STATUS "Stb directory:      ${TINYGLTF_INCLUDE_DIRS}" )
message(STATUS "tinygltf directory: ${STB_INCLUDE_DIRS}" )

message(STATUS "Directory:          ${Vulkan_INCLUDE_DIRS}")
message(STATUS "Directory:          ${Vulkan_LIBRARIES}")

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS 
     ${PROJECT_SOURCE_DIR}/src/* 
     ${PROJECT_SOURCE_DIR}/third_party_patches/*
    )

add_executable (first_app ${SOURCES})

foreach( target IN ITEMS first_app)
  message(STATUS "Configuring target: ${target}")
  target_include_directories(${target} PRIVATE ${CMAKE_SOURCE_DIR}/src)
  target_link_libraries(${target} PRIVATE glfw)
  target_link_libraries(${target} PRIVATE imgui::imgui)
  target_include_directories(${target} PRIVATE ${STB_INCLUDE_DIRS} 
     ${TINYGLTF_INCLUDE_DIRS} ${Vulkan_INCLUDE_DIRS}
  )
  target_link_libraries(${target} PRIVATE ${Vulkan_LIBRARIES})
  target_link_libraries(${target} PRIVATE glm::glm)
endforeach()

set( source      "${CMAKE_SOURCE_DIR}/resources") 
set( destination "${CMAKE_CURRENT_BINARY_DIR}/resources") # or CMAKE_BINARY_DIR
set( vulkan_apps first_app )

if(MSVC)
  # startup project
  set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT first_app)


  # Needed in Visual Studio to enable file access to non-local paths
  # TargetDir is a VS macro containing path to executable directory 
  set_target_properties(
    ${vulkan_apps} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:${vulkan_apps}>/" 
  )
endif()

# Copy compiled shaders to executable directory
# Add post-build commands
add_custom_command(TARGET ${vulkan_apps} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/resources
        ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/resources COMMENT
        "copied resource folder from ${source} to ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>"
        VERBATIM
)
