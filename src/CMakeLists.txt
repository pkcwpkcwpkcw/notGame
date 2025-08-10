# src/CMakeLists.txt - 소스 파일 빌드 설정

# 소스 파일 수집
file(GLOB_RECURSE CORE_SOURCES "core/*.cpp")
file(GLOB_RECURSE RENDER_SOURCES "render/*.cpp")
file(GLOB_RECURSE GAME_SOURCES "game/*.cpp")
file(GLOB_RECURSE UI_SOURCES "ui/*.cpp")
file(GLOB_RECURSE UTILS_SOURCES "utils/*.cpp")
file(GLOB_RECURSE SIMULATION_SOURCES "simulation/*.cpp")

# 정적 라이브러리 생성 (소스 파일이 있을 경우에만)
if(CORE_SOURCES)
    add_library(notgate_core STATIC ${CORE_SOURCES})
    target_include_directories(notgate_core PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/extern
        ${SDL2_INCLUDE_DIRS}
        ${CMAKE_SOURCE_DIR}/extern/imgui
        ${CMAKE_SOURCE_DIR}/extern/imgui/backends
        ${CMAKE_SOURCE_DIR}/extern/glad/include
        ${GLM_INCLUDE_DIR}
    )
    target_link_libraries(notgate_core
        ${SDL2_LIBRARIES}
        notgate_utils
    )
    if(TARGET imgui)
        target_link_libraries(notgate_core imgui)
    endif()
else()
    # 소스 파일이 없으면 인터페이스 라이브러리로 생성
    add_library(notgate_core INTERFACE)
    target_include_directories(notgate_core INTERFACE 
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/extern
    )
endif()

if(RENDER_SOURCES)
    add_library(notgate_render STATIC ${RENDER_SOURCES})
    target_include_directories(notgate_render PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${SDL2_INCLUDE_DIRS}
        ${OPENGL_INCLUDE_DIR}
        ${GLAD_INCLUDE_DIR}
        ${GLM_INCLUDE_DIR}
    )
    target_link_libraries(notgate_render 
        ${SDL2_LIBRARIES}
        ${OPENGL_LIBRARIES}
        notgate_utils
    )
else()
    add_library(notgate_render INTERFACE)
    target_include_directories(notgate_render INTERFACE 
        ${CMAKE_CURRENT_SOURCE_DIR}
    )
endif()

if(GAME_SOURCES)
    add_library(notgate_game STATIC ${GAME_SOURCES})
    target_include_directories(notgate_game PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/extern/json/include
    )
    target_link_libraries(notgate_game 
        notgate_core
        notgate_utils
    )
else()
    add_library(notgate_game INTERFACE)
    target_include_directories(notgate_game INTERFACE 
        ${CMAKE_CURRENT_SOURCE_DIR}
    )
endif()

if(UI_SOURCES OR IMGUI_FOUND)
    if(UI_SOURCES)
        add_library(notgate_ui STATIC ${UI_SOURCES} ${IMGUI_SOURCES})
    else()
        add_library(notgate_ui STATIC ${IMGUI_SOURCES})
    endif()
    target_include_directories(notgate_ui PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${IMGUI_INCLUDE_DIRS}
        ${SDL2_INCLUDE_DIRS}
        ${OPENGL_INCLUDE_DIR}
    )
    target_link_libraries(notgate_ui 
        ${SDL2_LIBRARIES}
        ${OPENGL_LIBRARIES}
        notgate_render
    )
    target_compile_definitions(notgate_ui PUBLIC IMGUI_IMPL_OPENGL_LOADER_GLAD)
else()
    add_library(notgate_ui INTERFACE)
    target_include_directories(notgate_ui INTERFACE 
        ${CMAKE_CURRENT_SOURCE_DIR}
    )
endif()

if(UTILS_SOURCES)
    add_library(notgate_utils STATIC ${UTILS_SOURCES})
    target_include_directories(notgate_utils PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}
    )
else()
    add_library(notgate_utils INTERFACE)
    target_include_directories(notgate_utils INTERFACE 
        ${CMAKE_CURRENT_SOURCE_DIR}
    )
endif()

# Step 7: 시뮬레이션 시스템 라이브러리
if(SIMULATION_SOURCES)
    add_library(notgate_simulation STATIC ${SIMULATION_SOURCES})
    target_include_directories(notgate_simulation PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${OPENGL_INCLUDE_DIR}
    )
    target_link_libraries(notgate_simulation 
        notgate_core
        notgate_utils
    )
    
    # SIMD 최적화 컴파일러 플래그 추가
    if(MSVC)
        target_compile_options(notgate_simulation PRIVATE /arch:AVX2)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(notgate_simulation PRIVATE -mavx2 -mfma)
    endif()
    
    # 성능 최적화 플래그 (Release 빌드에서)
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        if(MSVC)
            target_compile_options(notgate_simulation PRIVATE /O2 /Ot /GL)
            target_link_options(notgate_simulation PRIVATE /LTCG)
        else()
            target_compile_options(notgate_simulation PRIVATE -O3 -march=native -flto)
            target_link_options(notgate_simulation PRIVATE -flto)
        endif()
    endif()
    
    message(STATUS "Added notgate_simulation library with SIMD optimizations")
else()
    add_library(notgate_simulation INTERFACE)
    target_include_directories(notgate_simulation INTERFACE 
        ${CMAKE_CURRENT_SOURCE_DIR}
    )
endif()

# 실행 파일 생성 (이미 루트 CMakeLists.txt에서 처리하므로 주석 처리)
# main.cpp가 있고 별도 실행 파일이 필요한 경우에만 활성화
if(FALSE)  # 루트에서 이미 처리
    add_executable(notgate3 main.cpp ${GLAD_SOURCE})
    
    target_link_libraries(notgate3 
        notgate_core
        notgate_render
        notgate_game
        notgate_ui
        notgate_utils
        ${PLATFORM_LIBS}
        Threads::Threads
    )
    
    target_include_directories(notgate3 PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${GLAD_INCLUDE_DIR}
    )
    
    # 리소스 파일 복사 (개발 중)
    add_custom_command(TARGET notgate3 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:notgate3>/assets
    )
endif()

# 상태 메시지
message(STATUS "=== Source Libraries Configuration ===")
if(CORE_SOURCES)
    message(STATUS "notgate_core: ${CORE_SOURCES}")
else()
    message(STATUS "notgate_core: No source files (interface library)")
endif()

if(RENDER_SOURCES)
    message(STATUS "notgate_render: ${RENDER_SOURCES}")
else()
    message(STATUS "notgate_render: No source files (interface library)")
endif()

if(GAME_SOURCES)
    message(STATUS "notgate_game: ${GAME_SOURCES}")
else()
    message(STATUS "notgate_game: No source files (interface library)")
endif()

if(UI_SOURCES)
    message(STATUS "notgate_ui: ${UI_SOURCES}")
else()
    message(STATUS "notgate_ui: No source files (interface library)")
endif()

if(UTILS_SOURCES)
    message(STATUS "notgate_utils: ${UTILS_SOURCES}")
else()
    message(STATUS "notgate_utils: No source files (interface library)")
endif()

if(SIMULATION_SOURCES)
    message(STATUS "notgate_simulation: ${SIMULATION_SOURCES}")
else()
    message(STATUS "notgate_simulation: No source files (interface library)")
endif()
message(STATUS "======================================")

# 테스트 실행 파일 추가
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test/TestShaders.cpp")
    add_executable(test_shaders test/TestShaders.cpp ${GLAD_SOURCE})
    
    target_include_directories(test_shaders PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${SDL2_INCLUDE_DIRS}
        ${OPENGL_INCLUDE_DIR}
        ${GLAD_INCLUDE_DIR}
        ${GLM_INCLUDE_DIR}
    )
    
    target_link_libraries(test_shaders PRIVATE
        notgate_render
        ${SDL2_LIBRARIES}
        ${OPENGL_LIBRARIES}
        ${PLATFORM_LIBS}
    )
    
    # 셰이더 파일 복사
    add_custom_command(TARGET test_shaders POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/shaders
        $<TARGET_FILE_DIR:test_shaders>/shaders
    )
    
    message(STATUS "Added test_shaders executable")
endif()

# 셰이더 컴파일 테스트 실행 파일 추가
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test/TestShaderCompile.cpp")
    add_executable(test_shader_compile test/TestShaderCompile.cpp ${GLAD_SOURCE})
    
    target_include_directories(test_shader_compile PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${SDL2_INCLUDE_DIRS}
        ${OPENGL_INCLUDE_DIR}
        ${GLAD_INCLUDE_DIR}
        ${GLM_INCLUDE_DIR}
    )
    
    target_link_libraries(test_shader_compile PRIVATE
        notgate_render
        ${SDL2_LIBRARIES}
        ${OPENGL_LIBRARIES}
        ${PLATFORM_LIBS}
    )
    
    # 셰이더 파일 복사
    add_custom_command(TARGET test_shader_compile POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/shaders
        $<TARGET_FILE_DIR:test_shader_compile>/shaders
    )
    
    message(STATUS "Added test_shader_compile executable")
endif()

# 렌더링 시스템 테스트 실행 파일 추가
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test/TestRenderSystem.cpp")
    add_executable(test_render_system test/TestRenderSystem.cpp ${GLAD_SOURCE})
    
    target_include_directories(test_render_system PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${SDL2_INCLUDE_DIRS}
        ${OPENGL_INCLUDE_DIR}
        ${GLAD_INCLUDE_DIR}
        ${GLM_INCLUDE_DIR}
    )
    
    target_link_libraries(test_render_system PRIVATE
        notgate_core
        notgate_render
        notgate_game
        ${SDL2_LIBRARIES}
        ${OPENGL_LIBRARIES}
        ${PLATFORM_LIBS}
    )
    
    message(STATUS "Added test_render_system executable")
endif()