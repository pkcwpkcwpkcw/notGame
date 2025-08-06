# FetchSDL2.cmake - SDL2를 소스에서 자동으로 다운로드하고 빌드

include(FetchContent)

message(STATUS "Fetching SDL2...")

# SDL2 옵션 설정 (빌드 시간 단축)
set(SDL_STATIC ON CACHE BOOL "Build SDL2 static library" FORCE)
set(SDL_SHARED ON CACHE BOOL "Build SDL2 shared library" FORCE)
set(SDL_TEST OFF CACHE BOOL "Build SDL2 test programs" FORCE)
set(SDL2_DISABLE_INSTALL ON CACHE BOOL "Disable SDL2 install" FORCE)

# 불필요한 SDL2 서브시스템 비활성화 (빌드 시간 단축)
set(SDL_AUDIO ON CACHE BOOL "" FORCE)        # 오디오는 나중에 필요할 수 있음
set(SDL_VIDEO ON CACHE BOOL "" FORCE)        # 필수
set(SDL_RENDER ON CACHE BOOL "" FORCE)       # 필수
set(SDL_EVENTS ON CACHE BOOL "" FORCE)       # 필수
set(SDL_JOYSTICK OFF CACHE BOOL "" FORCE)    # 조이스틱 불필요
set(SDL_HAPTIC OFF CACHE BOOL "" FORCE)      # 햅틱 불필요
set(SDL_HIDAPI OFF CACHE BOOL "" FORCE)      # HID API 불필요
set(SDL_POWER OFF CACHE BOOL "" FORCE)       # 전원 관리 불필요
set(SDL_SENSOR OFF CACHE BOOL "" FORCE)      # 센서 불필요

# SDL2 소스 가져오기
FetchContent_Declare(
    SDL2
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-2.28.5  # 안정된 최신 버전
    GIT_SHALLOW TRUE        # 히스토리 없이 최신 커밋만
    GIT_PROGRESS TRUE       # 다운로드 진행 상황 표시
)

# SDL2 빌드
FetchContent_MakeAvailable(SDL2)

# SDL2 타겟 별칭 생성 (일관된 인터페이스)
if(TARGET SDL2-static)
    add_library(SDL2::SDL2-static ALIAS SDL2-static)
endif()

if(TARGET SDL2)
    add_library(SDL2::SDL2 ALIAS SDL2)
endif()

if(TARGET SDL2main)
    add_library(SDL2::SDL2main ALIAS SDL2main)
endif()

# SDL2 찾기 성공 플래그 설정
set(SDL2_FOUND TRUE)
set(SDL2_INCLUDE_DIRS ${sdl2_SOURCE_DIR}/include)

# 정적 링크 옵션
if(SDL2_STATIC_LINK)
    set(SDL2_LIBRARIES SDL2::SDL2-static SDL2::SDL2main)
    message(STATUS "SDL2 will be statically linked")
else()
    set(SDL2_LIBRARIES SDL2::SDL2 SDL2::SDL2main)
    message(STATUS "SDL2 will be dynamically linked")
endif()

message(STATUS "SDL2 fetched and configured successfully")