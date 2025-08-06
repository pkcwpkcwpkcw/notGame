# CompilerOptions.cmake - 컴파일러별 최적화 옵션 설정

message(STATUS "Setting up compiler options...")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# 플랫폼별 컴파일 옵션 설정
if(MSVC)
    # Windows (MSVC)
    message(STATUS "Configuring for MSVC")
    
    # 기본 컴파일 옵션
    add_compile_options(
        /W4                 # 경고 레벨 4
        /WX-                # 경고를 오류로 처리하지 않음
        /MP                 # 멀티프로세서 컴파일
        /permissive-        # 표준 준수 모드
        /Zc:__cplusplus     # __cplusplus 매크로 올바르게 설정
        /utf-8              # 소스 파일 UTF-8 인코딩
    )
    
    # MSVC는 멀티 컨피그 생성기이므로 생성자 표현식 사용
    # 릴리즈 최적화
    add_compile_options(
        $<$<CONFIG:Release>:/O2>           # 속도 최적화
        $<$<CONFIG:Release>:/GL>           # 전체 프로그램 최적화
        $<$<CONFIG:Release>:/fp:fast>      # 빠른 부동소수점 연산
        $<$<CONFIG:Release>:/GS->          # 버퍼 보안 검사 비활성화 (성능)
    )
    
    # AVX2 지원 (Release 모드에서만)
    if(USE_NATIVE_ARCH)
        add_compile_options($<$<CONFIG:Release>:/arch:AVX2>)
        message(STATUS "AVX2 instructions enabled for Release")
    endif()
    
    # 릴리즈 링크 옵션
    add_link_options(
        $<$<CONFIG:Release>:/LTCG>         # 링크 시간 코드 생성
        $<$<CONFIG:Release>:/OPT:REF>      # 참조되지 않는 함수/데이터 제거
        $<$<CONFIG:Release>:/OPT:ICF>      # 동일한 COMDAT 폴딩
    )
    
    # 디버그 옵션
    add_compile_options(
        $<$<CONFIG:Debug>:/Od>              # 최적화 비활성화
        $<$<CONFIG:Debug>:/RTC1>            # 런타임 체크
        $<$<CONFIG:Debug>:/Zi>              # 디버그 정보 생성
        $<$<CONFIG:Debug>:/JMC>             # Just My Code 디버깅
    )
    
    # RelWithDebInfo 옵션
    add_compile_options(
        $<$<CONFIG:RelWithDebInfo>:/O2>    # 속도 최적화
        $<$<CONFIG:RelWithDebInfo>:/Zi>    # 디버그 정보 생성
    )
    add_link_options(
        $<$<CONFIG:RelWithDebInfo>:/DEBUG>  # 디버그 정보 링크
    )
    
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    # Linux/macOS (GCC/Clang)
    message(STATUS "Configuring for ${CMAKE_CXX_COMPILER_ID}")
    
    # 기본 컴파일 옵션
    add_compile_options(
        -Wall               # 모든 경고 활성화
        -Wextra             # 추가 경고
        -Wpedantic          # 표준 준수 경고
        -Wno-unused-parameter
        -Wno-missing-field-initializers
    )
    
    # 릴리즈 최적화
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        message(STATUS "Enabling Release optimizations for ${CMAKE_CXX_COMPILER_ID}")
        add_compile_options(
            -O3                 # 최고 수준 최적화
            -flto               # 링크 시간 최적화
            -ffast-math         # 빠른 수학 연산
            -funroll-loops      # 루프 언롤링
            -finline-functions  # 함수 인라이닝
        )
        
        if(USE_NATIVE_ARCH)
            add_compile_options(-march=native)  # 네이티브 아키텍처 최적화
            message(STATUS "Native architecture optimizations enabled")
        else()
            add_compile_options(-march=x86-64-v3)  # AVX2 지원
            message(STATUS "Using x86-64-v3 (AVX2)")
        endif()
        
        add_link_options(-flto)
    endif()
    
    # 디버그 옵션
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        message(STATUS "Enabling Debug options for ${CMAKE_CXX_COMPILER_ID}")
        add_compile_options(
            -O0                 # 최적화 비활성화
            -g3                 # 최대 디버그 정보
            -ggdb               # GDB 디버그 정보
        )
        
        # 새니타이저 (옵션)
        option(ENABLE_SANITIZERS "Enable address and UB sanitizers" OFF)
        if(ENABLE_SANITIZERS)
            add_compile_options(
                -fsanitize=address
                -fsanitize=undefined
                -fno-omit-frame-pointer
            )
            add_link_options(
                -fsanitize=address
                -fsanitize=undefined
            )
            message(STATUS "Sanitizers enabled")
        endif()
        
        add_compile_definitions(
            _DEBUG
            DEBUG
        )
    endif()
    
    # RelWithDebInfo 옵션
    if(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
        add_compile_options(
            -O2                 # 중간 수준 최적화
            -g                  # 디버그 정보
        )
    endif()
endif()

# 프로파일링 옵션
if(ENABLE_PROFILING)
    message(STATUS "Profiling support enabled")
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        add_compile_options(-pg -fno-omit-frame-pointer)
        add_link_options(-pg)
    elseif(MSVC)
        add_compile_options(/Zi)
        add_link_options(/PROFILE)
    endif()
endif()

# C++ 기능 정의
add_compile_definitions(
    $<$<CONFIG:Debug>:DEBUG_BUILD>
    $<$<CONFIG:Release>:RELEASE_BUILD>
    $<$<CONFIG:RelWithDebInfo>:RELWITHDEBINFO_BUILD>
)

# 플랫폼별 정의
if(WIN32)
    add_compile_definitions(
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        _CRT_SECURE_NO_WARNINGS
    )
elseif(UNIX AND NOT APPLE)
    add_compile_definitions(LINUX_BUILD)
elseif(APPLE)
    add_compile_definitions(MACOS_BUILD)
endif()

message(STATUS "Compiler options configured successfully")