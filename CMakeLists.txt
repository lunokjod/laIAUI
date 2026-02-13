cmake_minimum_required(VERSION 3.15)
project(laIA)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find packages
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(CURL REQUIRED)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/imgui)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends)

# Source files
set(SOURCES
    main.cpp
    token_counter.cpp
    terminal_emulator.cpp
    deepseek_client.cpp
    async_task_manager.cpp
    chat_application.cpp
    imgui/imgui.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_tables.cpp
    imgui/imgui_widgets.cpp
    imgui/backends/imgui_impl_glfw.cpp
    imgui/backends/imgui_impl_opengl3.cpp
    imgui/misc/cpp/imgui_stdlib.cpp
)

# Header files
set(HEADERS
    token_counter.hpp
    terminal_emulator.hpp
    deepseek_client.hpp
    async_task_manager.hpp
    chat_application.hpp
)

# Add executable
add_executable(laIA ${SOURCES} ${HEADERS})

# Link libraries
target_link_libraries(laIA
    ${OPENGL_LIBRARIES}
    glfw
    ${CURL_LIBRARIES}
    pthread
)

# Add nlohmann/json
find_package(nlohmann_json 3.2.0 REQUIRED)
target_link_libraries(laIA nlohmann_json::nlohmann_json)

# Compiler flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(laIA PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Debug symbols - always include them
target_compile_options(laIA PRIVATE -g)

# Installation
install(TARGETS laIA DESTINATION bin)
