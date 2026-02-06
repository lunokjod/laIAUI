cmake_minimum_required(VERSION 3.15)
project(laIAUI)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(CURL REQUIRED)
find_package(Threads REQUIRED)
find_package(nlohmann_json REQUIRED)

# Opció per triar entre SDL i GLFW
option(USE_SDL "Use SDL instead of GLFW" OFF)

if(USE_SDL)
    # Configuració de SDL2
    find_package(SDL2 REQUIRED)
    
    message(STATUS "Building with SDL2 backend")
else()
    # Configuració de GLFW
    find_package(glfw3 3.3 REQUIRED)
    find_package(OpenGL REQUIRED)
    
    message(STATUS "Building with GLFW backend")
endif()

# Fitxers font d'ImGui (comuns)
set(IMGUI_COMMON_SOURCES
    imgui/imgui.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_tables.cpp
    imgui/imgui_widgets.cpp
)

# Fitxers font específics del backend
if(USE_SDL)
    set(IMGUI_BACKEND_SOURCES
        imgui/backends/imgui_impl_sdl2.cpp
        imgui/backends/imgui_impl_sdlrenderer2.cpp
    )
    
    # Definir macro per al codi
    add_definitions(-DUSE_SDL)
else()
    set(IMGUI_BACKEND_SOURCES
        imgui/backends/imgui_impl_glfw.cpp
        imgui/backends/imgui_impl_opengl3.cpp
    )
endif()

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
if(USE_SDL)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        SDL2::SDL2
    )
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE
        glfw
        OpenGL::GL
    )
endif()

# Configuració específica per a Linux
if(UNIX AND NOT APPLE)
    if(USE_SDL)
        target_link_libraries(${PROJECT_NAME} PRIVATE
            dl
            m
        )
    else()
        target_link_libraries(${PROJECT_NAME} PRIVATE
            GL
            X11
            pthread
            dl
            m
        )
    endif()
endif()

# Configuració específica per a Windows
if(WIN32)
    if(USE_SDL)
        # SDL2 ja inclou les llibreries necessàries
    else()
        target_link_libraries(${PROJECT_NAME} PRIVATE
            opengl32
        )
    endif()
endif()

# Configuració específica per a macOS
if(APPLE)
    if(USE_SDL)
        # SDL2 per macOS
    else()
        find_library(COCOA_LIBRARY Cocoa)
        find_library(IOKIT_LIBRARY IOKit)
        find_library(COREVIDEO_LIBRARY CoreVideo)
        target_link_libraries(${PROJECT_NAME} PRIVATE
            ${COCOA_LIBRARY}
            ${IOKIT_LIBRARY}
            ${COREVIDEO_LIBRARY}
        )
    endif()
endif()