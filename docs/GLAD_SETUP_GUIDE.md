# GLAD 설치 가이드

## GLAD 다운로드 방법

GLAD는 OpenGL 함수 로더로, OpenGL 3.3 Core Profile을 사용하기 위해 필요합니다.

### 방법 1: GLAD 웹 생성기 사용 (권장)

1. **GLAD 생성기 웹사이트 방문**
   - https://glad.dav1d.de/ 접속

2. **설정 선택**
   - **Language**: C/C++
   - **Specification**: OpenGL
   - **API gl**: Version 3.3
   - **Profile**: Core
   - **Options**: Generate a loader 체크

3. **Generate 버튼 클릭**

4. **다운로드한 ZIP 파일 압축 해제**

5. **파일 복사**
   ```
   다운로드한 파일들을 다음 위치에 복사:
   
   include/glad/glad.h     → C:\project\notgame3\extern\glad\include\glad\glad.h
   include/KHR/khrplatform.h → C:\project\notgame3\extern\glad\include\KHR\khrplatform.h
   src/glad.c              → C:\project\notgame3\extern\glad\src\glad.c
   ```

### 방법 2: vcpkg 사용 (이미 설치된 경우)

```bash
vcpkg install glad:x64-windows
```

### 방법 3: 사전 생성된 파일 다운로드

GitHub에서 OpenGL 3.3 Core Profile용 GLAD 파일을 다운로드:
- https://github.com/Dav1dde/glad/tree/glad2

### 디렉토리 구조 확인

설치 후 다음과 같은 구조가 되어야 합니다:

```
C:\project\notgame3\extern\glad\
├── include/
│   ├── glad/
│   │   └── glad.h
│   └── KHR/
│       └── khrplatform.h
└── src/
    └── glad.c
```

## 설치 확인

CMake를 실행하여 GLAD가 제대로 감지되는지 확인:

```bash
cmake -B build -G "Visual Studio 17 2022"
```

출력에서 다음 메시지를 확인:
```
GLAD found at C:/project/notgame3/extern/glad
```

## 문제 해결

### GLAD 파일을 찾을 수 없는 경우
- 파일 경로가 정확한지 확인
- CMakeLists.txt의 GLAD_DIR 경로 확인

### 컴파일 오류가 발생하는 경우
- OpenGL 3.3 Core Profile로 생성했는지 확인
- glad.h가 glad.c보다 먼저 include되는지 확인