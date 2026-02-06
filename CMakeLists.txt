cmake_minimum_required(VERSION 3.15)
project(laIAUI)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Opción para compilación estática
option(BUILD_STATIC "Build as static executable" OFF)

# Configurar enlace estático si está activado
if(BUILD_STATIC)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
    set(BUILD_SHARED_LIBRARIES OFF)
    set(CMAKE_EXE_LINK_DYNAMIC_CXX_STD_LIBRARY OFF)
    message(STATUS "Building static executable")
endif()

find_package(CURL REQUIRED)
find_package(Threads REQUIRED)
find_package(nlohmann_json REQUIRED)

# Configuració de GLFW
find_package(glfw3 3.3 REQUIRED)
find_package(OpenGL REQUIRED)

message(STATUS "Building with GLFW backend")

# Fitxers font d'ImGui (comuns)
set(IMGUI_COMMON_SOURCES
    imgui/imgui.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_tables.cpp
    imgui/imgui_widgets.cpp
)

# Fitxers font específics del backend
set(IMGUI_BACKEND_SOURCES
    imgui/backends/imgui_impl_glfw.cpp
    imgui/backends/imgui_impl_opengl3.cpp
)

# Afegir executable amb TOTS els fitxers font
add_executable(${PROJECT_NAME} 
    main.cpp
    themes.cpp
    ${IMGUI_COMMON_SOURCES}
    ${IMGUI_BACKEND_SOURCES}
)

# Incloure directoris d'ImGui
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends
)

# Enllaçar llibreries comunes
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${CURL_LIBRARIES}
    Threads::Threads
    nlohmann_json::nlohmann_json
)

# Enllaçar llibreries específiques del backend
target_link_libraries(${PROJECT_NAME} PRIVATE
    glfw
    OpenGL::GL
)

if(UNIX AND NOT APPLE)
    if(BUILD_STATIC)
        # Para enlace estático en Linux
        target_link_libraries(${PROJECT_NAME} PRIVATE
            ${CMAKE_DL_LIBS}
            ${CMAKE_THREAD_LIBS_INIT}
            m
            rt
        )
    else()
        # Enlace dinámico (configuración original)
        target_link_libraries(${PROJECT_NAME} PRIVATE
            GL
            X11
            pthread
            dl
            m
        )
    endif()
endif()