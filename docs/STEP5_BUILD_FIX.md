# Step 5: SDL2 빌드 문제 해결

## 문제
- MSYS64의 SDL2 헤더가 Visual Studio와 호환되지 않음
- `strings.h` 파일을 찾을 수 없다는 에러

## 해결 방법

### 옵션 1: vcpkg 사용 (권장)
```bash
# vcpkg 설치
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg integrate install

# SDL2 설치
vcpkg install sdl2:x64-windows
```

### 옵션 2: SDL2 직접 다운로드
1. https://www.libsdl.org/download-2.0.php 에서 SDL2-devel-2.x.x-VC.zip 다운로드
2. C:\SDL2 폴더에 압축 해제
3. CMakeLists.txt 수정:
```cmake
set(SDL2_DIR "C:/SDL2")
find_package(SDL2 REQUIRED)
```

### 옵션 3: 기존 빌드 사용
빌드 디렉토리를 삭제하지 말고 기존에 작동하던 빌드를 유지

## 현재 상태

### 완료된 작업
1. Application.h에 Gate Placement System 헤더와 멤버 변수 추가
2. Application.cpp에 초기화 코드 추가  
3. UI 렌더링 코드 추가 (Gate Palette)
4. 키보드 이벤트 처리 추가 (N, Delete, ESC)
5. 마우스 이벤트 처리 추가

### 코드 통합 위치
- **초기화**: Application.cpp 라인 75-88
- **UI 렌더링**: Application.cpp 라인 461-464
- **키보드 이벤트**: Application.cpp 라인 275-297
- **마우스 이벤트**: Application.cpp 라인 323-341

## 테스트 방법

SDL2 문제 해결 후:
```bash
cmake --build build --config Debug --target notgame
cd build\bin\Debug
notgame.exe
```

1. 콘솔에서 "Gate Placement System initialized successfully" 확인
2. Main Menu에서 Play 클릭
3. 좌측에 Gate Palette 창 확인
4. N 키로 NOT 게이트 배치 모드 활성화