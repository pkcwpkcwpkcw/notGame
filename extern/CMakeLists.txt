# extern/CMakeLists.txt - 외부 라이브러리 빌드 설정

message(STATUS "Configuring external libraries...")

# ImGui 라이브러리 (있을 경우)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui.cpp")
    message(STATUS "Building Dear ImGui...")
    
    set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/imgui)
    
    set(IMGUI_SOURCES
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_demo.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp
    )
    
    # SDL2 백엔드 (SDL2가 있을 경우)
    if(SDL2_FOUND AND EXISTS "${IMGUI_DIR}/backends/imgui_impl_sdl2.cpp")
        list(APPEND IMGUI_SOURCES ${IMGUI_DIR}/backends/imgui_impl_sdl2.cpp)
        message(STATUS "  - ImGui SDL2 backend enabled")
    endif()
    
    # OpenGL 백엔드 (OpenGL과 GLAD가 있을 경우)
    if(OpenGL_FOUND AND EXISTS "${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp")
        list(APPEND IMGUI_SOURCES ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp)
        message(STATUS "  - ImGui OpenGL3 backend enabled")
    endif()
    
    # Win32 백엔드 (Windows에서 SDL2가 없을 경우)
    if(WIN32 AND NOT SDL2_FOUND AND EXISTS "${IMGUI_DIR}/backends/imgui_impl_win32.cpp")
        list(APPEND IMGUI_SOURCES ${IMGUI_DIR}/backends/imgui_impl_win32.cpp)
        message(STATUS "  - ImGui Win32 backend enabled")
    endif()
    
    add_library(imgui STATIC ${IMGUI_SOURCES})
    
    target_include_directories(imgui PUBLIC 
        ${IMGUI_DIR}
        ${IMGUI_DIR}/backends
    )
    
    # Add GLAD include directory FIRST (before SDL2/OpenGL to ensure proper header inclusion)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/glad/include")
        target_include_directories(imgui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/glad/include)
    endif()
    
    if(SDL2_FOUND)
        target_include_directories(imgui PUBLIC ${SDL2_INCLUDE_DIRS})
        target_link_libraries(imgui PUBLIC ${SDL2_LIBRARIES})
    endif()
    
    if(OpenGL_FOUND)
        target_include_directories(imgui PUBLIC ${OPENGL_INCLUDE_DIR})
        # Tell ImGui to use GLAD as the OpenGL loader
        target_compile_definitions(imgui PUBLIC IMGUI_IMPL_OPENGL_LOADER_GLAD)
    endif()
    
    # Set ImGui variables for parent scope
    set(IMGUI_FOUND TRUE PARENT_SCOPE)
    set(IMGUI_INCLUDE_DIRS ${IMGUI_DIR} ${IMGUI_DIR}/backends PARENT_SCOPE)
    set(IMGUI_LIBRARY imgui PARENT_SCOPE)
    
    message(STATUS "Dear ImGui configured successfully")
endif()

# GLM (header-only library)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/glm/glm/glm.hpp")
    message(STATUS "GLM found at ${CMAKE_CURRENT_SOURCE_DIR}/glm")
    add_library(glm INTERFACE)
    target_include_directories(glm INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/glm)
    set(GLM_FOUND TRUE PARENT_SCOPE)
    set(GLM_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/glm PARENT_SCOPE)
endif()

# JSON (header-only library)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/json/single_include/nlohmann/json.hpp")
    message(STATUS "nlohmann/json found at ${CMAKE_CURRENT_SOURCE_DIR}/json")
    add_library(json INTERFACE)
    target_include_directories(json INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/json/single_include)
    set(JSON_FOUND TRUE PARENT_SCOPE)
    set(JSON_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/json/single_include PARENT_SCOPE)
endif()

# GLAD (OpenGL loader)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/glad/src/glad.c")
    message(STATUS "GLAD found at ${CMAKE_CURRENT_SOURCE_DIR}/glad")
    add_library(glad STATIC ${CMAKE_CURRENT_SOURCE_DIR}/glad/src/glad.c)
    target_include_directories(glad PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/glad/include)
    set(GLAD_FOUND TRUE PARENT_SCOPE)
    set(GLAD_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/glad/src/glad.c PARENT_SCOPE)
    set(GLAD_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/glad/include PARENT_SCOPE)
endif()

# stb libraries (single-header image loading, etc.)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/stb/stb_image.h")
    message(STATUS "stb libraries found at ${CMAKE_CURRENT_SOURCE_DIR}/stb")
    add_library(stb INTERFACE)
    target_include_directories(stb INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/stb)
    set(STB_FOUND TRUE PARENT_SCOPE)
    set(STB_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/stb PARENT_SCOPE)
endif()

message(STATUS "External libraries configuration complete")