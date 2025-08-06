cmake_minimum_required(VERSION 3.20)
project(NotGame 
    VERSION 1.0.0
    DESCRIPTION "High-performance NOT gate logic circuit sandbox"
    LANGUAGES CXX C
)

# C++ 표준 설정
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 빌드 타입 기본값 설정
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

# 옵션 설정
option(BUILD_TESTS "Build unit tests" OFF)
option(BUILD_DOCS "Build documentation" OFF)
option(ENABLE_PROFILING "Enable profiling support" OFF)
option(USE_NATIVE_ARCH "Use native architecture optimizations" ON)

# 모듈 경로 추가
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# 출력 디렉토리 설정
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# 컴파일러 옵션 로드
include(CompilerOptions)

# 의존성 찾기
include(Dependencies)

# 메인 실행 파일 생성
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")
    add_executable(notgame src/main.cpp)
    
    # 테스트 실행 파일 추가
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/test.cpp")
        add_executable(test_cpp src/test.cpp)
    endif()
    
    # 간단한 SDL2 테스트 추가
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/test_simple.cpp" AND SDL2_FOUND)
        add_executable(test_simple src/test_simple.cpp)
        target_include_directories(test_simple PRIVATE ${SDL2_INCLUDE_DIRS})
        target_link_libraries(test_simple PRIVATE ${SDL2_LIBRARIES})
    endif()
    
    # 포함 디렉토리 설정
    target_include_directories(notgame PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )
    
    # SDL2가 있으면 포함
    if(SDL2_FOUND)
        target_include_directories(notgame PRIVATE ${SDL2_INCLUDE_DIRS})
        target_link_libraries(notgame PRIVATE ${SDL2_LIBRARIES})
    endif()
    
    # OpenGL이 있으면 포함
    if(OpenGL_FOUND)
        target_include_directories(notgame PRIVATE ${OPENGL_INCLUDE_DIR})
        target_link_libraries(notgame PRIVATE ${OPENGL_LIBRARIES})
        
        # GLAD 소스 추가
        if(DEFINED GLAD_SOURCE)
            target_sources(notgame PRIVATE ${GLAD_SOURCE})
            target_include_directories(notgame PRIVATE ${GLAD_INCLUDE_DIR})
        endif()
    endif()
    
    # ImGui는 별도 라이브러리로 링크됨 (소스 직접 추가 안함)
    if(IMGUI_FOUND)
        target_link_libraries(notgame PRIVATE imgui)
        target_include_directories(notgame PRIVATE ${IMGUI_INCLUDE_DIRS})
    endif()
    
    # GLM이 있으면 포함
    if(DEFINED GLM_INCLUDE_DIR)
        target_include_directories(notgame PRIVATE ${GLM_INCLUDE_DIR})
    endif()
    
    # JSON이 있으면 포함
    if(DEFINED JSON_INCLUDE_DIR)
        target_include_directories(notgame PRIVATE ${JSON_INCLUDE_DIR})
    endif()
    
    # 플랫폼 라이브러리 링크
    if(DEFINED PLATFORM_LIBS)
        target_link_libraries(notgame PRIVATE ${PLATFORM_LIBS})
    endif()
    
    # 모든 라이브러리 링크
    target_link_libraries(notgame PRIVATE 
        notgate_core
        notgate_game  
        notgate_ui
        notgate_render
        notgate_utils
        Threads::Threads
    )
    
    # Windows 서브시스템 설정
    if(WIN32)
        if(SDL2_FOUND)
            # SDL2가 있으면 Windows 서브시스템 사용 (콘솔 창 숨김)
            set_target_properties(notgame PROPERTIES
                LINK_FLAGS "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup"
            )
        else()
            # SDL2가 없으면 콘솔 서브시스템 사용
            set_target_properties(notgame PROPERTIES
                LINK_FLAGS "/SUBSYSTEM:CONSOLE"
            )
        endif()
    endif()
endif()

# 서브디렉토리 추가
# extern을 먼저 처리하여 외부 라이브러리 변수들을 설정
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/extern/CMakeLists.txt")
    add_subdirectory(extern)
endif()

# src 디렉토리 추가 (extern 이후에 처리하여 라이브러리 사용 가능)
add_subdirectory(src)

if(BUILD_TESTS AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt")
    enable_testing()
    add_subdirectory(tests)
endif()

# 패키징 설정
include(Packaging)