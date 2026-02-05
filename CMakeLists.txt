cmake_minimum_required(VERSION 3.15)
project(laIAUI)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(CURL REQUIRED)
find_package(Threads REQUIRED)
find_package(nlohmann_json REQUIRED)

# Configuració de GLFW
find_package(glfw3 3.3 REQUIRED)

# Configuració d'OpenGL
find_package(OpenGL REQUIRED)

# Configuració de compilació
#if(MSVC)
#    add_compile_options(/W4 /WX)
#else()
#    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
#endif()

# Fitxers font d'ImGui
set(IMGUI_SOURCES
    imgui/imgui.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_tables.cpp
    imgui/imgui_widgets.cpp
    imgui/backends/imgui_impl_glfw.cpp
    imgui/backends/imgui_impl_opengl3.cpp
)

# Afegir executable amb TOTS els fitxers font
add_executable(${PROJECT_NAME} 
    main.cpp
    ${IMGUI_SOURCES}
)

# Incloure directoris d'ImGui
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends
)

# Enllaçar llibreries
target_link_libraries(${PROJECT_NAME} PRIVATE
    glfw
    OpenGL::GL
)


# Configuració específica per a Linux
if(UNIX AND NOT APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        GL
        X11
        pthread
        dl
        ${CURL_LIBRARIES}
        ${READLINE_LIB}
        Threads::Threads
        nlohmann_json::nlohmann_json
    )
endif()
