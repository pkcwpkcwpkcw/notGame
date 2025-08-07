# 렌더링 시스템 기능 명세서

## 1. 문서 개요

### 1.1 목적
NOT Gate 게임의 기본 렌더링 시스템(Step 3)의 기능적 요구사항을 정의하고, 각 서브시스템의 동작 방식과 인터페이스를 명세한다.

### 1.2 범위
- OpenGL 기반 2D 렌더링 시스템
- 그리드, 게이트, 와이어 시각화
- 카메라 제어 시스템
- 기본 렌더링 파이프라인

### 1.3 관련 문서
- [SHADER_REQUIREMENTS.md](SHADER_REQUIREMENTS.md) - 셰이더 요구사항
- [CPP_ARCHITECTURE.md](CPP_ARCHITECTURE.md) - 전체 아키텍처
- [GAME_SPEC.md](GAME_SPEC.md) - 게임 사양

## 2. 시스템 개요

### 2.1 렌더링 파이프라인
```
입력 데이터 → 변환 → 버텍스 처리 → 래스터화 → 프래그먼트 처리 → 화면 출력
     ↑                      ↑                           ↑
  게임 상태            카메라 행렬                  셰이더 프로그램
```

### 2.2 주요 구성 요소
1. **Renderer** - 메인 렌더링 관리자
2. **Camera** - 뷰 변환 및 투영
3. **ShaderManager** - 셰이더 프로그램 관리
4. **RenderBatch** - 드로우 콜 최적화
5. **TextureManager** - 텍스처 리소스 관리

## 3. 기능 요구사항

### 3.1 그리드 렌더링 (Grid Rendering)

#### 3.1.1 기본 기능
- **무한 그리드**: 카메라 이동에 따라 동적 생성
- **적응형 밀도**: 줌 레벨에 따른 그리드 라인 밀도 조절
- **계층적 그리드**: 주 그리드(10칸)와 보조 그리드 구분

#### 3.1.2 시각적 요소
```
주 그리드선: 두께 2px, 불투명도 50%
보조 그리드선: 두께 1px, 불투명도 20%
원점 표시: X축(빨강), Y축(초록)
```

#### 3.1.3 성능 사양
- 렌더링 방식: 단일 쿼드 + 프래그먼트 셰이더
- 목표 성능: < 0.5ms per frame
- 메모리 사용: < 1KB (정점 데이터)

#### 3.1.4 인터페이스
```cpp
class GridRenderer {
public:
    void SetGridSize(float size);
    void SetGridColor(const Color& main, const Color& sub);
    void SetViewport(const Rectangle& viewport);
    void Render(const Camera& camera);
};
```

### 3.2 게이트 스프라이트 렌더링 (Gate Sprite Rendering)

#### 3.2.1 기본 기능
- **스프라이트 배치**: 그리드 정렬된 위치에 게이트 표시
- **상태 표현**: 활성/비활성, 선택, 하이라이트
- **포트 표시**: 입력(3개), 출력(1개) 포트 시각화

#### 3.2.2 게이트 시각적 사양
```
크기: 64x64 픽셀 (그리드 1칸)
입력 포트: 왼쪽 3개 (상/중/하)
출력 포트: 오른쪽 1개 (중앙)
색상 상태:
  - 기본: 회색
  - 활성: 밝은 파랑
  - 선택: 노란 테두리
  - 에러: 빨간 테두리
```

#### 3.2.3 배칭 및 인스턴싱
- **스프라이트 배칭**: 동일 텍스처 게이트 일괄 렌더링
- **인스턴싱**: 1000개 이상 게이트시 자동 활성화
- **컬링**: 화면 밖 게이트 제외

#### 3.2.4 인터페이스
```cpp
class GateRenderer {
public:
    void AddGate(const Gate& gate);
    void UpdateGate(uint32_t gateId, const GateState& state);
    void RemoveGate(uint32_t gateId);
    void SetSelection(const std::vector<uint32_t>& selected);
    void Render(const Camera& camera);
    void Clear();
};
```

### 3.3 와이어 라인 렌더링 (Wire Line Rendering)

#### 3.3.1 기본 기능
- **연결선 그리기**: 게이트 간 연결 표시
- **경로 유형**: 직선, L자, 계단식
- **신호 상태**: on/off 상태 색상 구분

#### 3.3.2 와이어 시각적 사양
```
두께: 3px (기본), 5px (선택)
색상:
  - 신호 없음: 어두운 회색 (#404040)
  - 신호 있음: 밝은 초록 (#00FF00)
  - 선택됨: 노랑 (#FFFF00)
  - 드래그 중: 반투명 흰색
애니메이션:
  - 신호 전파: 점선 흐름 효과
  - 펄스: 0.1초 주기
```

#### 3.3.3 경로 계산
- **자동 라우팅**: 장애물 회피
- **수동 조절**: 중간점 추가/이동
- **스냅**: 그리드 정렬

#### 3.3.4 인터페이스
```cpp
class WireRenderer {
public:
    void AddWire(const Wire& wire);
    void UpdateWireSignal(uint32_t wireId, bool signalOn);
    void SetWireSelection(uint32_t wireId, bool selected);
    void StartDragWire(const Vec2& start);
    void UpdateDragWire(const Vec2& current);
    void EndDragWire();
    void Render(const Camera& camera);
};
```

### 3.4 카메라 시스템 (Camera System)

#### 3.4.1 기본 기능
- **팬(Pan)**: 마우스 드래그 또는 키보드
- **줌(Zoom)**: 마우스 휠 또는 단축키
- **포커스**: 특정 객체 중심 이동

#### 3.4.2 카메라 사양
```
줌 레벨: 0.1x ~ 10x (로그 스케일)
줌 단계: 휠당 1.2배
팬 속도: 5 픽셀/프레임 (키보드)
부드러운 이동: 0.15초 보간
경계 제한: 옵션 (샌드박스는 무제한)
```

#### 3.4.3 변환 행렬
```
View Matrix = Translation × Rotation × Scale
Projection Matrix = Orthographic(left, right, bottom, top)
```

#### 3.4.4 인터페이스
```cpp
class Camera {
public:
    void Pan(const Vec2& delta);
    void Zoom(float factor, const Vec2& pivot);
    void SetPosition(const Vec2& pos);
    void SetZoom(float zoom);
    void FocusOn(const Rectangle& bounds);
    
    glm::mat4 GetViewMatrix() const;
    glm::mat4 GetProjectionMatrix() const;
    Vec2 ScreenToWorld(const Vec2& screen) const;
    Vec2 WorldToScreen(const Vec2& world) const;
};
```

## 4. 렌더링 최적화 기능

### 4.1 배칭 시스템

#### 4.1.1 동적 배칭
```cpp
class RenderBatch {
    static constexpr size_t MAX_VERTICES = 65536;
    static constexpr size_t MAX_INDICES = 98304;
    
    void Begin();
    void AddQuad(const Quad& quad);
    void AddLine(const Line& line);
    void End();
    void Flush();
};
```

#### 4.1.2 배칭 규칙
- 동일 셰이더 객체 그룹화
- 동일 텍스처 객체 그룹화
- 정점 버퍼 오버플로우시 자동 플러시

### 4.2 컬링 시스템

#### 4.2.1 프러스텀 컬링
```cpp
bool IsInViewport(const Rectangle& bounds, const Camera& camera);
```

#### 4.2.2 LOD (Level of Detail)
- 거리별 디테일 조절
- 줌 레벨별 렌더링 생략

### 4.3 상태 캐싱

#### 4.3.1 OpenGL 상태 최소화
```cpp
class RenderState {
    void SetShader(ShaderProgram* shader);
    void SetTexture(uint32_t slot, Texture* texture);
    void SetBlendMode(BlendMode mode);
    // 중복 상태 변경 방지
};
```

## 5. 렌더 타겟 및 포스트 프로세싱

### 5.1 프레임버퍼
- **메인 프레임버퍼**: 최종 출력
- **피킹 프레임버퍼**: 마우스 선택용
- **미니맵 프레임버퍼**: 전체 뷰 (옵션)

### 5.2 포스트 이펙트 (향후 확장)
- FXAA 앤티앨리어싱
- 블룸 효과 (신호 강조)
- 색상 보정

## 6. 리소스 관리

### 6.1 텍스처 관리

#### 6.1.1 텍스처 아틀라스
```
gates_atlas.png (512x512):
  - NOT 게이트: 0,0,64,64
  - 입력 노드: 64,0,32,32
  - 출력 노드: 96,0,32,32
  - 포트 마커: 128,0,16,16
```

#### 6.1.2 텍스처 로딩
```cpp
class TextureManager {
    Texture* Load(const std::string& path);
    void Unload(const std::string& path);
    Texture* GetTexture(const std::string& name);
};
```

### 6.2 셰이더 관리
- 런타임 컴파일
- 에러 리포팅
- 핫 리로딩 (개발 모드)

## 7. 에러 처리

### 7.1 OpenGL 에러
```cpp
void CheckGLError(const char* operation);
#ifdef DEBUG
    #define GL_CHECK(x) x; CheckGLError(#x)
#else
    #define GL_CHECK(x) x
#endif
```

### 7.2 리소스 로딩 실패
- 폴백 텍스처 (체커보드)
- 기본 셰이더 (단색)
- 에러 로그 출력

## 8. 성능 지표

### 8.1 목표 성능
| 항목 | 목표값 | 조건 |
|------|--------|------|
| FPS | 60+ | 1,000 게이트 |
| FPS | 30+ | 10,000 게이트 |
| 렌더 시간 | < 16ms | 일반 플레이 |
| 드로우 콜 | < 100 | 화면당 |
| 메모리 사용 | < 100MB | VRAM |

### 8.2 프로파일링 지점
```cpp
class RenderProfiler {
    void BeginFrame();
    void EndFrame();
    void BeginSection(const char* name);
    void EndSection();
    
    struct Stats {
        float frameTime;
        uint32_t drawCalls;
        uint32_t vertices;
        uint32_t triangles;
    };
};
```

## 9. 테스트 시나리오

### 9.1 기능 테스트
1. 그리드 무한 스크롤
2. 1000개 게이트 배치
3. 복잡한 와이어 연결
4. 극단적 줌 (0.1x ~ 10x)
5. 화면 크기 변경

### 9.2 스트레스 테스트
1. 10,000개 게이트 렌더링
2. 50,000개 와이어 렌더링
3. 급속 팬/줌
4. 메모리 누수 검사

### 9.3 호환성 테스트
1. Intel 통합 그래픽
2. NVIDIA GPU
3. AMD GPU
4. 4K 해상도
5. 다중 모니터

## 10. 구현 계획

### Phase 1: 기초 구현 (2일)
- [ ] OpenGL 컨텍스트 설정
- [ ] 기본 셰이더 컴파일
- [ ] 간단한 사각형 렌더링

### Phase 2: 핵심 기능 (3일)
- [ ] 그리드 렌더링
- [ ] 게이트 스프라이트
- [ ] 와이어 라인
- [ ] 카메라 시스템

### Phase 3: 최적화 (2일)
- [ ] 배칭 시스템
- [ ] 인스턴싱
- [ ] 컬링

### Phase 4: 완성 (1일)
- [ ] 버그 수정
- [ ] 성능 테스트
- [ ] 문서화

## 11. API 사용 예제

### 11.1 초기화
```cpp
// 렌더러 초기화
Renderer renderer;
renderer.Initialize(window, 1920, 1080);

// 셰이더 로드
ShaderManager shaderMgr;
shaderMgr.LoadShader("grid", "shaders/grid.vert", "shaders/grid.frag");
shaderMgr.LoadShader("sprite", "shaders/sprite.vert", "shaders/sprite.frag");
```

### 11.2 렌더링 루프
```cpp
void RenderFrame() {
    renderer.Clear(Color::DarkGray);
    renderer.BeginFrame();
    
    // 그리드
    gridRenderer.Render(camera);
    
    // 게이트
    gateRenderer.Render(camera);
    
    // 와이어
    wireRenderer.Render(camera);
    
    // UI
    ImGui::Render();
    
    renderer.EndFrame();
    renderer.Present();
}
```

### 11.3 이벤트 처리
```cpp
void OnMouseMove(int x, int y) {
    Vec2 worldPos = camera.ScreenToWorld(Vec2(x, y));
    
    if (isDragging) {
        camera.Pan(worldPos - lastWorldPos);
    }
    
    lastWorldPos = worldPos;
}

void OnMouseWheel(float delta) {
    Vec2 mouseWorld = camera.ScreenToWorld(mousePos);
    camera.Zoom(1.0f + delta * 0.1f, mouseWorld);
}
```

## 12. 의존성 및 제약사항

### 12.1 필수 의존성
- OpenGL 3.3+
- SDL2 2.0+
- GLM 0.9.9+
- stb_image

### 12.2 제약사항
- 2D 렌더링만 지원
- 최대 텍스처 크기: 4096x4096
- 최대 드로우 콜: 1000/frame
- 최대 정점 수: 1M/frame

## 13. 향후 확장 계획

### 13.1 단기 (v1.1)
- 파티클 시스템
- 트레일 효과
- 그림자/광원

### 13.2 장기 (v2.0)
- Vulkan 지원
- 컴퓨트 셰이더
- GPU 기반 시뮬레이션
- 3D 뷰 모드