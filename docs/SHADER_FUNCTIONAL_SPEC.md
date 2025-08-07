# OpenGL 셰이더 기능 명세서

## 1. 문서 개요

### 1.1 목적
Step 3-1 "OpenGL 셰이더 작성"의 구체적인 기능 동작과 구현 세부사항을 정의한다.

### 1.2 범위
- GLSL 셰이더 코드의 기능적 동작
- 셰이더 프로그램의 입출력 처리
- 렌더링 파이프라인 내 셰이더 역할
- 셰이더 간 데이터 흐름

### 1.3 관련 문서
- [SHADER_REQUIREMENTS.md](SHADER_REQUIREMENTS.md) - 셰이더 요구사항

## 2. 셰이더 프로그램 기능 정의

### 2.1 Grid Shader 기능

#### 2.1.1 Vertex Shader 기능
```glsl
// grid.vert 주요 기능
void main() {
    // 1. 화면을 덮는 전체 크기 쿼드 생성
    // 2. 클립 공간 좌표 계산 (-1 to 1)
    // 3. 월드 공간 좌표 역산 후 전달
}
```

**입력 처리**
- 단순 쿼드 정점 4개 (또는 삼각형 2개)
- 정점 위치만 필요 (텍스처 좌표 불필요)

**변환 처리**
```glsl
// 클립 공간 직접 매핑
gl_Position = vec4(aPosition * 2.0 - 1.0, 0.0, 1.0);

// 월드 좌표 계산
vWorldPos = (uInvViewProj * gl_Position).xy;
```

**출력 데이터**
- `gl_Position`: 클립 공간 좌표
- `vWorldPos`: 프래그먼트용 월드 좌표

#### 2.1.2 Fragment Shader 기능
```glsl
// grid.frag 주요 기능
void main() {
    // 1. 월드 좌표에서 그리드 위치 계산
    // 2. 그리드 라인 근접도 측정
    // 3. 앤티앨리어싱 적용
    // 4. 주/보조 그리드 구분
    // 5. 페이드 효과 적용
}
```

**그리드 계산 알고리즘**
```glsl
vec2 grid = abs(fract(vWorldPos / uGridSize) - 0.5);
float line = min(grid.x, grid.y);
float lineWidth = fwidth(line) * uLineWidth;
float alpha = 1.0 - smoothstep(0.0, lineWidth, line);
```

**계층적 그리드**
```glsl
// 10칸마다 주 그리드
bool isMajor = mod(vWorldPos / uGridSize, 10.0) < 0.5;
vec4 color = isMajor ? uGridColor : uSubGridColor;
```

**거리 기반 페이드**
```glsl
float dist = length(vWorldPos - uCameraPos);
float fade = 1.0 - smoothstep(uFadeStart, uFadeEnd, dist);
fragColor = vec4(color.rgb, color.a * alpha * fade);
```

### 2.2 Sprite Shader 기능

#### 2.2.1 Vertex Shader 기능
```glsl
// sprite.vert 주요 기능
void main() {
    // 1. 스프라이트 정점 변환
    // 2. 회전 행렬 적용
    // 3. 스케일 적용
    // 4. 월드->클립 공간 변환
    // 5. 텍스처 좌표 전달
}
```

**변환 파이프라인**
```glsl
// 로컬 -> 월드 변환
mat2 rotation = mat2(cos(uRotation), -sin(uRotation),
                     sin(uRotation), cos(uRotation));
vec2 worldPos = uPosition + rotation * (aPosition * uScale);

// 월드 -> 클립 변환
gl_Position = uProjection * uView * vec4(worldPos, 0.0, 1.0);
```

**인스턴싱 처리**
```glsl
#ifdef USE_INSTANCING
    vec2 worldPos = aInstancePos + rotation * (aPosition * uScale);
    vColor = aInstanceColor;
#else
    vec2 worldPos = uPosition + rotation * (aPosition * uScale);
    vColor = uColor;
#endif
```

#### 2.2.2 Fragment Shader 기능
```glsl
// sprite.frag 주요 기능
void main() {
    // 1. 텍스처 샘플링
    // 2. 색상 변조
    // 3. 선택 하이라이트
    // 4. 알파 테스트
}
```

**텍스처 아틀라스 샘플링**
```glsl
vec2 atlasCoord = uSpriteRect.xy + vTexCoord * uSpriteRect.zw;
vec4 texColor = texture(uTexture, atlasCoord);
```

**상태별 색상 처리**
```glsl
// 선택 상태 하이라이트
vec3 highlight = mix(texColor.rgb, vec3(1.0, 1.0, 0.0), uSelected * 0.3);

// 활성 상태 발광
vec3 glow = highlight + vec3(0.0, 0.2, 0.5) * uActive;

fragColor = vec4(glow * vColor.rgb * uTintColor.rgb, texColor.a);
```

### 2.3 Line Shader 기능

#### 2.3.1 Vertex Shader 기능
```glsl
// line.vert 주요 기능
void main() {
    // 1. 라인 정점을 두께있는 쿼드로 확장
    // 2. 라인 방향 계산
    // 3. 수직 벡터로 확장
    // 4. 끝점 캡 처리
}
```

**라인 확장 알고리즘**
```glsl
// 라인 방향 벡터
vec2 dir = normalize(aEndPos - aStartPos);
vec2 normal = vec2(-dir.y, dir.x);

// 두께 적용
vec2 offset = normal * uLineThickness * aVertexID;
vec2 worldPos = mix(aStartPos, aEndPos, aTexCoord.x) + offset;
```

**미터 조인트 처리**
```glsl
// 연결점에서 각도 계산
float angle = acos(dot(prevDir, nextDir));
float miterLength = uLineThickness / sin(angle * 0.5);
vec2 miterDir = normalize(prevDir + nextDir);
```

#### 2.3.2 Fragment Shader 기능
```glsl
// line.frag 주요 기능
void main() {
    // 1. 신호 상태 색상 결정
    // 2. 애니메이션 패턴 생성
    // 3. 앤티앨리어싱
    // 4. 글로우 효과
}
```

**신호 애니메이션**
```glsl
// 흐름 애니메이션
float flow = fract(vTexCoord.x - uTime * uFlowSpeed);
float pulse = step(0.5, flow);

// 신호 색상
vec4 signalColor = mix(uSignalOffColor, uSignalOnColor, uSignalState);
signalColor.rgb *= (1.0 + pulse * 0.3);
```

**엣지 앤티앨리어싱**
```glsl
float dist = abs(vTexCoord.y - 0.5) * 2.0;
float edge = 1.0 - smoothstep(0.8, 1.0, dist);
fragColor = vec4(signalColor.rgb, signalColor.a * edge);
```

### 2.4 UI Shader 기능

#### 2.4.1 Vertex Shader 기능
```glsl
// ui.vert 주요 기능
void main() {
    // 1. 스크린 공간 좌표 변환
    // 2. 픽셀 퍼펙트 정렬
    // 3. 클리핑 영역 전달
}
```

**스크린 공간 변환**
```glsl
// 픽셀 좌표 -> NDC
vec2 ndc = (aPosition / uScreenSize) * 2.0 - 1.0;
gl_Position = vec4(ndc * vec2(1, -1), 0.0, 1.0);

// 픽셀 스냅
gl_Position.xy = round(gl_Position.xy * uScreenSize) / uScreenSize;
```

#### 2.4.2 Fragment Shader 기능
```glsl
// ui.frag 주요 기능
void main() {
    // 1. 텍스처/색상 처리
    // 2. 클리핑 테스트
    // 3. 폰트 앤티앨리어싱
}
```

**클리핑 처리**
```glsl
if (gl_FragCoord.x < uClipRect.x || 
    gl_FragCoord.y < uClipRect.y ||
    gl_FragCoord.x > uClipRect.z || 
    gl_FragCoord.y > uClipRect.w) {
    discard;
}
```

## 3. 셰이더 간 데이터 흐름

### 3.1 유니폼 버퍼 구조
```cpp
struct CommonUniforms {
    glm::mat4 projection;
    glm::mat4 view;
    glm::vec2 cameraPos;
    float zoom;
    float time;
};
```

### 3.2 정점 속성 레이아웃
```cpp
// Grid
struct GridVertex {
    glm::vec2 position;  // location = 0
};

// Sprite
struct SpriteVertex {
    glm::vec2 position;  // location = 0
    glm::vec2 texCoord;  // location = 1
};

// Line
struct LineVertex {
    glm::vec2 position;  // location = 0
    float side;          // location = 1 (-1 or 1)
    float t;            // location = 2 (0 to 1)
};

// UI
struct UIVertex {
    glm::vec2 position;  // location = 0
    glm::vec2 texCoord;  // location = 1
    glm::vec4 color;     // location = 2
};
```

## 4. 셰이더 최적화 기능

### 4.1 조건부 컴파일
```glsl
#ifdef HIGH_QUALITY
    // 고품질 앤티앨리어싱
    #define AA_SAMPLES 4
#else
    // 기본 품질
    #define AA_SAMPLES 1
#endif
```

### 4.2 LOD 기반 셰이더 변형
```glsl
#if LOD_LEVEL == 0
    // 최고 품질: 모든 효과
#elif LOD_LEVEL == 1
    // 중간 품질: 기본 효과만
#else
    // 낮은 품질: 최소 렌더링
#endif
```

### 4.3 동적 브랜칭 최소화
```glsl
// 나쁜 예
if (uEnableGlow) {
    color += glowEffect();
}

// 좋은 예
color += glowEffect() * uGlowStrength; // uGlowStrength = 0 or 1
```

## 5. 에러 처리 및 폴백

### 5.1 셰이더 컴파일 실패
```cpp
// 폴백 셰이더 (단색)
const char* fallbackVert = R"(
    #version 330 core
    layout(location = 0) in vec2 aPos;
    uniform mat4 uMVP;
    void main() {
        gl_Position = uMVP * vec4(aPos, 0.0, 1.0);
    }
)";

const char* fallbackFrag = R"(
    #version 330 core
    out vec4 fragColor;
    void main() {
        fragColor = vec4(1.0, 0.0, 1.0, 1.0); // 자홍색 = 에러
    }
)";
```

### 5.2 유니폼 검증
```cpp
void ValidateUniforms(GLuint program) {
    GLint count;
    glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &count);
    
    for (GLint i = 0; i < count; i++) {
        char name[256];
        GLsizei length;
        GLint size;
        GLenum type;
        glGetActiveUniform(program, i, sizeof(name), 
                          &length, &size, &type, name);
        // 로깅 및 검증
    }
}
```

## 6. 성능 메트릭

### 6.1 목표 성능
| 셰이더 | 픽셀/프레임 | 실행 시간 |
|--------|-------------|-----------|
| Grid | 1920×1080 | < 0.5ms |
| Sprite | 1000 instances | < 1.0ms |
| Line | 10000 segments | < 2.0ms |
| UI | Full screen | < 0.3ms |

### 6.2 GPU 프로파일링 포인트
```glsl
// 타이머 쿼리 삽입점
#ifdef PROFILING
    // 셰이더 시작
    atomicCounterIncrement(perfCounter);
#endif
```

## 7. 테스트 케이스

### 7.1 Grid Shader 테스트
1. **기본 렌더링**: 1x1 그리드 표시
2. **줌 테스트**: 0.1x ~ 10x 줌
3. **이동 테스트**: 큰 거리 이동
4. **페이드 테스트**: 거리별 투명도

### 7.2 Sprite Shader 테스트
1. **단일 스프라이트**: 위치, 회전, 스케일
2. **인스턴싱**: 1000개 동시 렌더링
3. **아틀라스**: 다양한 스프라이트 영역
4. **상태 변화**: 선택, 활성화 효과

### 7.3 Line Shader 테스트
1. **직선**: 수평, 수직, 대각선
2. **연결선**: 다중 세그먼트
3. **두께 변화**: 1px ~ 10px
4. **애니메이션**: 신호 흐름

### 7.4 UI Shader 테스트
1. **텍스트 렌더링**: 다양한 폰트 크기
2. **클리핑**: 영역 제한
3. **블렌딩**: 투명도 처리
4. **픽셀 정렬**: 흐림 방지

## 8. 디버그 기능

### 8.1 시각적 디버그 모드
```glsl
#ifdef DEBUG_MODE
    // 와이어프레임 표시
    if (uDebugMode == 1) {
        fragColor = vec4(1.0, 0.0, 0.0, 1.0);
    }
    // UV 좌표 시각화
    else if (uDebugMode == 2) {
        fragColor = vec4(vTexCoord, 0.0, 1.0);
    }
    // 노멀 시각화
    else if (uDebugMode == 3) {
        fragColor = vec4(vNormal * 0.5 + 0.5, 1.0);
    }
#endif
```

### 8.2 성능 시각화
```glsl
// 오버드로우 표시
fragColor.a = 0.1; // 반투명으로 겹침 확인

// 복잡도 히트맵
float complexity = float(instructionCount) / 100.0;
fragColor = vec4(complexity, 1.0 - complexity, 0.0, 1.0);
```

## 9. 구현 체크리스트

### Phase 1: 기본 구조 (Day 1)
- [ ] GLSL 파일 생성
- [ ] 버전 및 기본 구조
- [ ] 입출력 변수 정의
- [ ] 컴파일 테스트

### Phase 2: 핵심 로직 (Day 2-3)
- [ ] Grid 계산 로직
- [ ] Sprite 변환 및 샘플링
- [ ] Line 확장 알고리즘
- [ ] UI 스크린 공간 변환

### Phase 3: 최적화 (Day 4)
- [ ] 인스턴싱 구현
- [ ] 조건부 컴파일
- [ ] 성능 프로파일링
- [ ] 메모리 최적화

### Phase 4: 완성 (Day 5)
- [ ] 에러 처리
- [ ] 디버그 기능
- [ ] 전체 통합 테스트
- [ ] 문서 업데이트