cmake_minimum_required(VERSION 3.20)
project(NotGate3 VERSION 0.1.0 LANGUAGES CXX)

# C++20 설정
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 최적화 플래그
if(MSVC)
    add_compile_options(/O2 /arch:AVX2 /fp:fast /GL)
    add_link_options(/LTCG)
else()
    add_compile_options(-O3 -march=native -ffast-math -flto)
    add_link_options(-flto)
endif()

# SDL2 찾기
find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(Threads REQUIRED)

# Dear ImGui
add_subdirectory(external/imgui)

# 소스 파일
set(SOURCES
    src/main.cpp
    src/core/Circuit.cpp
    src/core/SimulationEngine.cpp
    src/render/Renderer.cpp
    src/render/Camera.cpp
    src/game/GameState.cpp
    src/game/LevelSystem.cpp
    src/ui/UIManager.cpp
)

# 실행 파일
add_executable(${PROJECT_NAME} ${SOURCES})

# 헤더 디렉토리
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${SDL2_INCLUDE_DIRS}
)

# 링크
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${SDL2_LIBRARIES}
    OpenGL::GL
    Threads::Threads
    imgui
)

# 리소스 복사
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets
)