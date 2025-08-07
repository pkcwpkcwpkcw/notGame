# Dependencies.cmake - 외부 라이브러리 의존성 관리

message(STATUS "Configuring dependencies...")

# 스레드 라이브러리 (필수)
find_package(Threads REQUIRED)
if(Threads_FOUND)
    message(STATUS "Threads library found")
endif()

# SDL2 관리 옵션
option(USE_SDL2 "Use SDL2 for window and input" ON)
option(USE_BUNDLED_SDL2 "Download and build SDL2 from source" OFF)
option(SDL2_STATIC_LINK "Statically link SDL2" OFF)

if(USE_SDL2)
    if(USE_BUNDLED_SDL2)
        # FetchContent로 SDL2 자동 다운로드 및 빌드
        include(${CMAKE_SOURCE_DIR}/cmake/FetchSDL2.cmake)
    else()
        # 커스텀 FindSDL2 모듈 사용
        list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}")
        include(FindSDL2)
        
        if(NOT SDL2_FOUND)
            # SDL2 찾기 시도
            find_package(SDL2 QUIET)
        endif()
    
    if(NOT SDL2_FOUND)
        # vcpkg 경로 확인
        if(DEFINED ENV{VCPKG_ROOT})
            set(CMAKE_PREFIX_PATH "$ENV{VCPKG_ROOT}/installed/x64-windows" ${CMAKE_PREFIX_PATH})
            find_package(SDL2 QUIET)
        endif()
    endif()
    
    if(NOT SDL2_FOUND)
        # 수동 경로 확인
        if(WIN32)
            set(SDL2_SEARCH_PATHS
                "${CMAKE_BINARY_DIR}/vcpkg_installed/x64-windows"
                "${CMAKE_BINARY_DIR}/vcpkg_installed/x86-windows"
                "C:/SDL2"
                "C:/Libraries/SDL2"
                "${CMAKE_SOURCE_DIR}/extern/SDL2"
                "$ENV{PROGRAMFILES}/SDL2"
                "C:/msys64/ucrt64"
                "C:/msys64/mingw64"
            )
            
            find_path(SDL2_INCLUDE_DIR 
                NAMES SDL.h
                PATH_SUFFIXES include/SDL2 include
                PATHS ${SDL2_SEARCH_PATHS}
            )
            
            find_library(SDL2_LIBRARY
                NAMES SDL2 SDL2main libSDL2
                PATH_SUFFIXES lib/x64 lib bin
                PATHS ${SDL2_SEARCH_PATHS}
            )
            
            if(SDL2_INCLUDE_DIR AND SDL2_LIBRARY)
                set(SDL2_FOUND TRUE)
                set(SDL2_INCLUDE_DIRS ${SDL2_INCLUDE_DIR})
                set(SDL2_LIBRARIES ${SDL2_LIBRARY})
                message(STATUS "SDL2 found manually at ${SDL2_INCLUDE_DIR}")
            endif()
        endif()
    endif()
    endif()  # USE_BUNDLED_SDL2
    
    if(SDL2_FOUND)
        message(STATUS "SDL2 found: ${SDL2_LIBRARIES}")
        add_compile_definitions(HAS_SDL2)
    else()
        message(WARNING "SDL2 not found. Window creation will be disabled.")
        message(STATUS "To install SDL2:")
        if(WIN32)
            message(STATUS "  - Download from https://www.libsdl.org/download-2.0.php")
            message(STATUS "  - Or use vcpkg: vcpkg install sdl2:x64-windows")
        elseif(APPLE)
            message(STATUS "  - Use Homebrew: brew install sdl2")
        else()
            message(STATUS "  - Use package manager: sudo apt-get install libsdl2-dev")
        endif()
    endif()
endif()

# OpenGL (선택적)
option(USE_OPENGL "Use OpenGL for rendering" ON)
if(USE_OPENGL)
    find_package(OpenGL QUIET)
    if(OpenGL_FOUND)
        message(STATUS "OpenGL found: ${OPENGL_LIBRARIES}")
        add_compile_definitions(HAS_OPENGL)
        
        # GLAD 포함 (OpenGL 로더)
        set(GLAD_DIR "${CMAKE_SOURCE_DIR}/extern/glad")
        if(EXISTS "${GLAD_DIR}/src/glad.c")
            set(GLAD_SOURCE "${GLAD_DIR}/src/glad.c")
            set(GLAD_INCLUDE_DIR "${GLAD_DIR}/include")
            message(STATUS "GLAD found at ${GLAD_DIR}")
            add_compile_definitions(USE_GLAD)
        else()
            message(STATUS "GLAD not found. OpenGL loader will need to be added later.")
        endif()
    else()
        message(WARNING "OpenGL not found. GPU rendering will be disabled.")
    endif()
endif()

# Dear ImGui (선택적)
option(USE_IMGUI "Use Dear ImGui for UI" ON)
if(USE_IMGUI)
    set(IMGUI_DIR "${CMAKE_SOURCE_DIR}/extern/imgui")
    if(EXISTS "${IMGUI_DIR}/imgui.cpp")
        message(STATUS "Dear ImGui found at ${IMGUI_DIR}")
        add_compile_definitions(HAS_IMGUI)
        
        # ImGui 소스 파일 목록
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
            message(STATUS "ImGui SDL2 backend enabled")
        endif()
        
        # OpenGL 백엔드 (OpenGL이 있을 경우)
        if(OpenGL_FOUND AND EXISTS "${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp")
            list(APPEND IMGUI_SOURCES ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp)
            message(STATUS "ImGui OpenGL3 backend enabled")
        endif()
        
        set(IMGUI_INCLUDE_DIRS ${IMGUI_DIR} ${IMGUI_DIR}/backends)
        set(IMGUI_FOUND TRUE)
    else()
        message(STATUS "Dear ImGui not found. UI will be limited.")
        message(STATUS "To add ImGui:")
        message(STATUS "  git submodule add https://github.com/ocornut/imgui.git extern/imgui")
    endif()
endif()

# GLM (수학 라이브러리) (선택적)
option(USE_GLM "Use GLM for mathematics" ON)
if(USE_GLM)
    # vcpkg에서 GLM 찾기
    set(GLM_DIR "${CMAKE_BINARY_DIR}/vcpkg_installed/x64-windows/include")
    if(EXISTS "${GLM_DIR}/glm/glm.hpp")
        message(STATUS "GLM found at ${GLM_DIR}")
        set(GLM_INCLUDE_DIR ${GLM_DIR})
        add_compile_definitions(HAS_GLM)
    else()
        find_package(glm QUIET)
        if(glm_FOUND)
            message(STATUS "GLM found via find_package")
            add_compile_definitions(HAS_GLM)
        else()
            message(STATUS "GLM not found. Will use custom math implementation.")
        endif()
    endif()
endif()

# JSON 라이브러리 (선택적)
option(USE_JSON "Use JSON library for data files" ON)
if(USE_JSON)
    # nlohmann/json 찾기
    set(JSON_DIR "${CMAKE_SOURCE_DIR}/extern/json")
    if(EXISTS "${JSON_DIR}/single_include/nlohmann/json.hpp")
        message(STATUS "nlohmann/json found at ${JSON_DIR}")
        set(JSON_INCLUDE_DIR "${JSON_DIR}/single_include")
        add_compile_definitions(HAS_JSON)
    else()
        message(STATUS "JSON library not found. JSON support will be limited.")
        message(STATUS "To add nlohmann/json:")
        message(STATUS "  git submodule add https://github.com/nlohmann/json.git extern/json")
    endif()
endif()

# 플랫폼별 추가 라이브러리
if(WIN32)
    # Windows 특정 라이브러리
    set(PLATFORM_LIBS 
        winmm    # 타이머
        imm32    # 입력
        version  # 버전 정보
        setupapi # 장치 정보
    )
    message(STATUS "Windows platform libraries: ${PLATFORM_LIBS}")
elseif(APPLE)
    # macOS 특정 프레임워크
    find_library(COCOA_LIBRARY Cocoa)
    find_library(IOKIT_LIBRARY IOKit)
    find_library(COREVIDEO_LIBRARY CoreVideo)
    set(PLATFORM_LIBS 
        ${COCOA_LIBRARY} 
        ${IOKIT_LIBRARY} 
        ${COREVIDEO_LIBRARY}
    )
    message(STATUS "macOS frameworks: ${PLATFORM_LIBS}")
elseif(UNIX)
    # Linux 특정 라이브러리
    find_package(X11)
    if(X11_FOUND)
        set(PLATFORM_LIBS ${X11_LIBRARIES} dl)
        message(STATUS "Linux libraries: ${PLATFORM_LIBS}")
    else()
        set(PLATFORM_LIBS dl)
        message(WARNING "X11 not found. Window creation may be limited.")
    endif()
endif()

# 테스트 프레임워크 (선택적)
if(BUILD_TESTS)
    option(USE_CATCH2 "Use Catch2 for testing" ON)
    if(USE_CATCH2)
        # FetchContent를 사용한 Catch2 다운로드
        include(FetchContent)
        FetchContent_Declare(
            Catch2
            GIT_REPOSITORY https://github.com/catchorg/Catch2.git
            GIT_TAG v3.4.0
            GIT_SHALLOW TRUE
        )
        
        message(STATUS "Fetching Catch2...")
        FetchContent_MakeAvailable(Catch2)
        
        if(TARGET Catch2::Catch2WithMain)
            message(STATUS "Catch2 configured successfully")
        endif()
    endif()
endif()

# 의존성 요약
message(STATUS "=== Dependency Summary ===")
message(STATUS "Threads: FOUND")
if(SDL2_FOUND)
    message(STATUS "SDL2: FOUND")
else()
    message(STATUS "SDL2: NOT FOUND (optional)")
endif()
if(OpenGL_FOUND)
    message(STATUS "OpenGL: FOUND")
else()
    message(STATUS "OpenGL: NOT FOUND (optional)")
endif()
if(USE_IMGUI AND EXISTS "${IMGUI_DIR}/imgui.cpp")
    message(STATUS "ImGui: FOUND")
else()
    message(STATUS "ImGui: NOT FOUND (optional)")
endif()
if(USE_GLM AND (EXISTS "${GLM_DIR}/glm/glm.hpp" OR glm_FOUND))
    message(STATUS "GLM: FOUND")
else()
    message(STATUS "GLM: NOT FOUND (optional)")
endif()
if(USE_JSON AND EXISTS "${JSON_DIR}/single_include/nlohmann/json.hpp")
    message(STATUS "JSON: FOUND")
else()
    message(STATUS "JSON: NOT FOUND (optional)")
endif()
message(STATUS "=========================")