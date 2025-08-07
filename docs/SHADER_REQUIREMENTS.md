# OpenGL 셰이더 요구사항 명세서

## 1. 개요

NOT Gate 게임의 렌더링 시스템을 위한 OpenGL 셰이더 프로그램 요구사항을 정의한다.
본 명세서는 Step 3-1 "OpenGL 셰이더 작성"을 위한 상세 기술 요구사항을 포함한다.

## 2. 셰이더 프로그램 구성

### 2.1 필수 셰이더 프로그램

#### 2.1.1 Grid Shader
- **용도**: 배경 그리드 렌더링
- **Vertex Shader**: 그리드 메시 변환
- **Fragment Shader**: 그리드 라인 색상 및 투명도

#### 2.1.2 Sprite Shader  
- **용도**: NOT 게이트 및 노드 렌더링
- **Vertex Shader**: 스프라이트 위치/크기 변환
- **Fragment Shader**: 텍스처 샘플링, 색상 변조

#### 2.1.3 Line Shader
- **용도**: 와이어 연결선 렌더링
- **Vertex Shader**: 라인 정점 변환
- **Fragment Shader**: 라인 색상, 신호 상태 표시

#### 2.1.4 UI Shader
- **용도**: Dear ImGui 통합용
- **Vertex Shader**: UI 요소 변환
- **Fragment Shader**: UI 텍스처 및 색상

## 3. 셰이더 기능 요구사항

### 3.1 Vertex Shader 공통 요구사항

#### 3.1.1 입력 속성 (Attributes)
```glsl
// 위치 데이터
layout(location = 0) in vec2 aPosition;  // 2D 좌표
layout(location = 1) in vec2 aTexCoord;  // 텍스처 좌표
layout(location = 2) in vec4 aColor;     // 정점 색상
```

#### 3.1.2 유니폼 변수 (Uniforms)
```glsl
// 변환 행렬
uniform mat4 uProjection;    // 투영 행렬
uniform mat4 uView;          // 뷰 행렬 (카메라)
uniform mat4 uModel;         // 모델 행렬

// 카메라 정보
uniform vec2 uCameraPos;     // 카메라 위치
uniform float uZoom;         // 줌 레벨
```

#### 3.1.3 출력 변수 (Varyings)
```glsl
out vec2 vTexCoord;          // 프래그먼트로 전달할 텍스처 좌표
out vec4 vColor;             // 프래그먼트로 전달할 색상
out vec2 vWorldPos;          // 월드 좌표 (그리드용)
```

### 3.2 Fragment Shader 공통 요구사항

#### 3.2.1 입력 변수
```glsl
in vec2 vTexCoord;           // 버텍스 셰이더로부터
in vec4 vColor;              // 버텍스 셰이더로부터
in vec2 vWorldPos;           // 월드 좌표
```

#### 3.2.2 유니폼 변수
```glsl
uniform sampler2D uTexture;  // 텍스처 샘플러
uniform float uTime;         // 애니메이션용 시간
uniform vec4 uTintColor;     // 색상 변조
```

#### 3.2.3 출력 변수
```glsl
out vec4 fragColor;          // 최종 픽셀 색상
```

## 4. 셰이더별 상세 요구사항

### 4.1 Grid Shader 요구사항

#### 4.1.1 기능
- 무한 그리드 렌더링 (화면 공간 기법)
- 주 그리드선과 보조 그리드선 구분
- 줌 레벨에 따른 그리드 밀도 자동 조절
- 페이드 아웃 효과 (원거리)

#### 4.1.2 특수 유니폼
```glsl
uniform float uGridSize;      // 그리드 칸 크기
uniform vec4 uGridColor;      // 그리드 색상
uniform vec4 uSubGridColor;   // 보조 그리드 색상
uniform float uLineWidth;     // 라인 두께
```

#### 4.1.3 성능 요구사항
- 단일 쿼드로 전체 그리드 렌더링
- 프래그먼트 셰이더에서 그리드 라인 계산
- 앤티앨리어싱 적용

### 4.2 Sprite Shader 요구사항

#### 4.2.1 기능
- 텍스처 아틀라스 지원
- 스프라이트 회전/스케일
- 색상 변조 (선택/하이라이트)
- 알파 블렌딩

#### 4.2.2 특수 유니폼
```glsl
uniform vec4 uSpriteRect;    // 아틀라스 내 스프라이트 영역
uniform float uRotation;      // 회전 각도
uniform vec2 uScale;          // 스케일
uniform float uSelected;      // 선택 상태 (0.0-1.0)
```

#### 4.2.3 인스턴싱 지원
```glsl
// 인스턴스 속성
layout(location = 3) in vec2 aInstancePos;
layout(location = 4) in float aInstanceRotation;
layout(location = 5) in vec4 aInstanceColor;
```

### 4.3 Line Shader 요구사항

#### 4.3.1 기능
- 와이어 렌더링 (직선, 꺾임)
- 신호 흐름 애니메이션
- 와이어 두께 조절
- 신호 상태별 색상 (on/off)

#### 4.3.2 특수 유니폼
```glsl
uniform float uLineThickness; // 라인 두께
uniform vec4 uSignalOnColor;  // 신호 켜짐 색상
uniform vec4 uSignalOffColor; // 신호 꺼짐 색상
uniform float uSignalFlow;    // 신호 흐름 애니메이션 (0.0-1.0)
```

#### 4.3.3 지오메트리 처리
- 라인을 삼각형 스트립으로 확장
- 끝점 캡 처리
- 연결점 부드러운 처리

### 4.4 UI Shader 요구사항

#### 4.4.1 기능
- Dear ImGui 렌더링 지원
- 클리핑 영역 처리
- 폰트 텍스처 렌더링
- 정확한 픽셀 정렬

#### 4.4.2 특수 처리
```glsl
// 스크린 공간 변환
uniform vec2 uScreenSize;     // 화면 크기
uniform vec4 uClipRect;       // 클리핑 사각형
```

## 5. 셰이더 컴파일 및 링킹 요구사항

### 5.1 버전 요구사항
```glsl
#version 330 core  // OpenGL 3.3 Core Profile
```

### 5.2 전처리기 매크로
```glsl
#define MAX_LIGHTS 4
#define GRID_SHADER 1
#define USE_INSTANCING 1
```

### 5.3 에러 처리
- 컴파일 에러 로깅
- 링킹 에러 로깅
- 유니폼 위치 캐싱
- 셰이더 핫 리로딩 (개발 모드)

## 6. 성능 최적화 요구사항

### 6.1 배칭
- 동일 셰이더/텍스처 객체 배칭
- 인스턴싱 활용 (대량 게이트)
- 드로우 콜 최소화

### 6.2 상태 변경 최소화
- 셰이더 프로그램 스위칭 최소화
- 텍스처 바인딩 최적화
- 유니폼 업데이트 배칭

### 6.3 메모리 최적화
- 정점 데이터 압축
- 16비트 인덱스 사용
- 텍스처 아틀라스 활용

## 7. 셰이더 파일 구조

```
shaders/
├── grid.vert         # 그리드 버텍스 셰이더
├── grid.frag         # 그리드 프래그먼트 셰이더
├── sprite.vert       # 스프라이트 버텍스 셰이더
├── sprite.frag       # 스프라이트 프래그먼트 셰이더
├── line.vert         # 라인 버텍스 셰이더
├── line.frag         # 라인 프래그먼트 셰이더
├── ui.vert           # UI 버텍스 셰이더
├── ui.frag           # UI 프래그먼트 셰이더
└── common.glsl       # 공통 함수/정의 (include)
```

## 8. 셰이더 관리 클래스 요구사항

### 8.1 ShaderProgram 클래스
```cpp
class ShaderProgram {
public:
    bool Load(const char* vertPath, const char* fragPath);
    void Use();
    void SetUniform(const char* name, float value);
    void SetUniform(const char* name, const glm::vec2& value);
    void SetUniform(const char* name, const glm::mat4& value);
    // ... 기타 유니폼 타입
private:
    GLuint m_program;
    std::unordered_map<std::string, GLint> m_uniformLocations;
};
```

### 8.2 ShaderManager 클래스
```cpp
class ShaderManager {
public:
    void LoadAllShaders();
    ShaderProgram* GetShader(const std::string& name);
    void ReloadShaders(); // 핫 리로딩
private:
    std::unordered_map<std::string, std::unique_ptr<ShaderProgram>> m_shaders;
};
```

## 9. 테스트 요구사항

### 9.1 기능 테스트
- 각 셰이더 개별 렌더링 테스트
- 다양한 해상도 테스트
- 극단적 줌 레벨 테스트

### 9.2 성능 테스트
- 1000개 게이트 렌더링 (60 FPS)
- 10000개 와이어 렌더링 (60 FPS)
- 드로우 콜 수 측정

### 9.3 호환성 테스트
- OpenGL 3.3 호환성
- 다양한 GPU 벤더 테스트
- 통합 그래픽 테스트

## 10. 확장 고려사항

### 10.1 미래 기능
- 컴퓨트 셰이더 (회로 시뮬레이션)
- 테셀레이션 (곡선 와이어)
- 지오메트리 셰이더 (파티클 효과)

### 10.2 플랫폼 확장
- OpenGL ES 지원 준비
- WebGL 호환성 고려
- Vulkan 마이그레이션 경로

## 11. 참고 자료

- OpenGL 3.3 Core Profile Specification
- GLSL 3.30 Specification
- Dear ImGui 렌더링 백엔드 가이드
- 게임 엔진 아키텍처 (Jason Gregory)

## 12. 구현 일정

1. **Phase 1** (1일): 기본 셰이더 작성 및 컴파일
2. **Phase 2** (1일): ShaderProgram/Manager 클래스 구현
3. **Phase 3** (2일): 각 셰이더별 기능 구현
4. **Phase 4** (1일): 최적화 및 테스트

총 예상 소요 시간: 5일