# SDL2 + OpenGL 윈도우 생성 요구사항 명세서

## 1. 개요
NOT Gate 게임의 기본 렌더링 시스템 구축을 위한 SDL2와 OpenGL 윈도우 생성 요구사항을 정의한다.

## 2. 기능 요구사항

### 2.1 윈도우 시스템
- **윈도우 생성**: 1280x720 기본 해상도의 윈도우 생성
- **윈도우 모드**: 창 모드/전체화면 전환 지원
- **리사이즈**: 윈도우 크기 조절 가능 (최소 800x600)
- **타이틀**: "NOT Gate Sandbox v0.1.0"

### 2.2 OpenGL 컨텍스트
- **OpenGL 버전**: Core Profile 3.3 이상
- **더블 버퍼링**: 화면 깜빡임 방지
- **VSync**: 수직 동기화 옵션 (설정 가능)
- **깊이 버퍼**: 24비트 깊이 버퍼

### 2.3 이벤트 처리
- **종료 이벤트**: 윈도우 X 버튼, Alt+F4
- **리사이즈 이벤트**: 뷰포트 자동 조정
- **포커스 이벤트**: 포커스 상실시 일시정지
- **키보드/마우스**: 기본 입력 이벤트 수신

## 3. 성능 요구사항
- **프레임레이트**: 60 FPS 유지
- **초기화 시간**: 1초 이내 윈도우 표시
- **메모리 사용**: 초기 50MB 이하
- **CPU 사용률**: 유휴 상태시 1% 이하

## 4. 기술 요구사항

### 4.1 라이브러리 버전
- SDL2: 2.0.14 이상
- OpenGL: 3.3 Core Profile
- GLEW/GLAD: OpenGL 확장 로더

### 4.2 플랫폼 지원
- Windows 10/11 (MSVC 2019+)
- Linux (Ubuntu 20.04+, GCC 9+)
- macOS (10.14+, Clang)

### 4.3 빌드 시스템
- CMake 3.20+
- vcpkg 또는 시스템 패키지 매니저

## 5. 코드 구조 요구사항

### 5.1 클래스 설계
```cpp
class Window {
    // SDL 윈도우 관리
    // OpenGL 컨텍스트 생성
    // 이벤트 폴링
};

class Renderer {
    // OpenGL 초기화
    // 기본 렌더링 설정
    // 뷰포트 관리
};

class Application {
    // 메인 루프
    // 프레임 타이밍
    // 상태 관리
};
```

### 5.2 에러 처리
- SDL 초기화 실패 처리
- OpenGL 컨텍스트 생성 실패 처리
- 최소 OpenGL 버전 검증
- 그래픽 드라이버 호환성 체크

## 6. 테스트 요구사항

### 6.1 기능 테스트
- [ ] 윈도우 생성/파괴
- [ ] 전체화면 전환
- [ ] 리사이즈 동작
- [ ] 이벤트 처리

### 6.2 성능 테스트
- [ ] FPS 측정
- [ ] 메모리 누수 체크
- [ ] CPU 사용률 모니터링

### 6.3 호환성 테스트
- [ ] Windows MSVC 빌드
- [ ] Linux GCC 빌드
- [ ] 다양한 GPU 테스트 (Intel, NVIDIA, AMD)

## 7. 구현 체크리스트

### Phase 1: 기본 설정
- [ ] SDL2 라이브러리 연결
- [ ] OpenGL 로더 (GLEW/GLAD) 설정
- [ ] CMakeLists.txt 업데이트

### Phase 2: 윈도우 생성
- [ ] SDL 초기화
- [ ] OpenGL 컨텍스트 생성
- [ ] 기본 윈도우 표시

### Phase 3: 렌더링 준비
- [ ] OpenGL 뷰포트 설정
- [ ] 클리어 컬러 설정
- [ ] 더블 버퍼링 구현

### Phase 4: 이벤트 루프
- [ ] 메인 루프 구현
- [ ] 이벤트 처리
- [ ] 프레임 제한 (60 FPS)

### Phase 5: 검증
- [ ] 배경색 변경 테스트
- [ ] 리사이즈 테스트
- [ ] 종료 처리 테스트

## 8. 예상 파일 구조
```
src/
├── main.cpp              # 진입점
├── core/
│   ├── Application.h     # 애플리케이션 클래스
│   └── Application.cpp
├── render/
│   ├── Window.h         # SDL 윈도우 래퍼
│   ├── Window.cpp
│   ├── Renderer.h       # OpenGL 렌더러
│   └── Renderer.cpp
└── utils/
    ├── Logger.h         # 로깅 시스템
    └── Logger.cpp
```

## 9. 샘플 코드 스니펫

### main.cpp
```cpp
#include "core/Application.h"

int main(int argc, char* argv[]) {
    Application app;
    if (!app.Initialize()) {
        return -1;
    }
    app.Run();
    app.Shutdown();
    return 0;
}
```

### Application 초기화
```cpp
bool Application::Initialize() {
    // SDL 초기화
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        return false;
    }
    
    // OpenGL 속성 설정
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, 
                       SDL_GL_CONTEXT_PROFILE_CORE);
    
    // 윈도우 생성
    window = std::make_unique<Window>("NOT Gate Sandbox", 
                                      1280, 720);
    
    // 렌더러 초기화
    renderer = std::make_unique<Renderer>();
    
    return true;
}
```

## 10. 성공 기준
- 검은 배경의 윈도우가 표시됨
- 60 FPS로 안정적으로 동작
- 윈도우 리사이즈 가능
- ESC 또는 X 버튼으로 종료 가능
- 메모리 누수 없음
- 3개 주요 플랫폼에서 빌드 성공