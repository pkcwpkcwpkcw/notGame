# 기본 이벤트 루프 테스트 케이스 명세서

## 1. 테스트 개요

### 1.1 테스트 목적
- 이벤트 루프의 모든 기능이 정상적으로 동작하는지 검증
- 다양한 입력 시나리오에서의 안정성 확인
- 성능 요구사항 충족 여부 검증
- 에러 상황에서의 복구 능력 확인

### 1.2 테스트 범위
- Application 생명주기 테스트
- 이벤트 처리 시스템 테스트
- 타이밍 시스템 테스트
- 입력 처리 테스트
- 성능 및 안정성 테스트

### 1.3 테스트 환경
| 항목 | 사양 |
|------|------|
| OS | Windows 10/11, Ubuntu 20.04+, macOS 12+ |
| CPU | Intel i5 이상 또는 동급 |
| RAM | 8GB 이상 |
| GPU | OpenGL 3.3 지원 |
| 빌드 | Debug/Release 모드 |

## 2. 테스트 케이스

### 2.1 Application 생명주기 테스트

#### TC-APP-001: 정상 초기화
**목적**: Application이 정상적으로 초기화되는지 확인

**전제조건**:
- SDL2 라이브러리 설치
- OpenGL 3.3+ 지원 그래픽 드라이버

**테스트 단계**:
1. AppConfig 객체 생성
2. Application 객체 생성
3. initialize() 호출
4. 반환값 확인

**입력 데이터**:
```cpp
AppConfig config;
config.windowWidth = 1280;
config.windowHeight = 720;
config.windowTitle = "Test Window";
```

**예상 결과**:
- initialize() 함수가 true 반환
- SDL 윈도우 생성 확인
- OpenGL 컨텍스트 생성 확인
- 에러 로그 없음

**실제 결과**: [ ]
**통과/실패**: [ ]

---

#### TC-APP-002: 초기화 실패 - SDL 에러
**목적**: SDL 초기화 실패 시 적절한 에러 처리 확인

**전제조건**:
- SDL_Init을 모킹하여 실패 반환 설정

**테스트 단계**:
1. SDL_Init 실패 시뮬레이션
2. Application::initialize() 호출
3. 반환값 및 에러 로그 확인

**예상 결과**:
- initialize() 함수가 false 반환
- "SDL 초기화 실패" 에러 로그 출력
- 프로그램 크래시 없음

**실제 결과**: [ ]
**통과/실패**: [ ]

---

#### TC-APP-003: 정상 종료
**목적**: Application이 정상적으로 종료되는지 확인

**테스트 단계**:
1. Application 초기화
2. quit() 호출
3. run() 메서드 실행
4. 리소스 정리 확인

**예상 결과**:
- 메인 루프 즉시 종료
- 모든 리소스 해제
- 메모리 누수 없음

**실제 결과**: [ ]
**통과/실패**: [ ]

---

#### TC-APP-004: 다중 초기화 방지
**목적**: 이미 초기화된 Application 재초기화 방지 확인

**테스트 단계**:
1. Application::initialize() 첫 번째 호출
2. Application::initialize() 두 번째 호출
3. 상태 확인

**예상 결과**:
- 두 번째 initialize() 호출 무시 또는 false 반환
- 기존 상태 유지
- 에러 로그 출력

**실제 결과**: [ ]
**통과/실패**: [ ]

### 2.2 이벤트 처리 테스트

#### TC-EVT-001: SDL_QUIT 이벤트 처리
**목적**: 윈도우 종료 이벤트가 정상 처리되는지 확인

**테스트 단계**:
1. Application 실행
2. SDL_QUIT 이벤트 생성 및 전송
3. 애플리케이션 상태 확인

**입력 데이터**:
```cpp
SDL_Event quit_event;
quit_event.type = SDL_QUIT;
SDL_PushEvent(&quit_event);
```

**예상 결과**:
- m_running이 false로 변경
- 메인 루프 종료
- 정상 종료 로그 출력

**실제 결과**: [ ]
**통과/실패**: [ ]

---

#### TC-EVT-002: 키보드 입력 처리
**목적**: 키보드 이벤트가 정상적으로 처리되는지 확인

**테스트 단계**:
1. Application 실행
2. 다양한 키 입력 이벤트 생성
3. KeyboardState 상태 확인

**테스트 데이터**:
| 키 | 동작 | 예상 상태 |
|----|------|-----------|
| ESC | Press | keys[SDL_SCANCODE_ESCAPE] = true |
| ESC | Release | keys[SDL_SCANCODE_ESCAPE] = false |
| Space | Press | keys[SDL_SCANCODE_SPACE] = true |
| Ctrl+Z | Press | Ctrl=true, Z=true |

**예상 결과**:
- 각 키의 상태가 올바르게 업데이트
- isJustPressed() 함수가 첫 프레임만 true 반환
- 수정자 키 조합 인식

**실제 결과**: [ ]
**통과/실패**: [ ]

---

#### TC-EVT-003: 마우스 입력 처리
**목적**: 마우스 이벤트가 정상적으로 처리되는지 확인

**테스트 단계**:
1. 마우스 이동 이벤트 생성
2. 마우스 클릭 이벤트 생성
3. 마우스 휠 이벤트 생성
4. MouseState 확인

**테스트 데이터**:
```cpp
// 마우스 이동
SDL_MouseMotionEvent motion;
motion.x = 100;
motion.y = 200;
motion.xrel = 10;
motion.yrel = -5;

// 마우스 클릭
SDL_MouseButtonEvent button;
button.button = SDL_BUTTON_LEFT;
button.state = SDL_PRESSED;
button.x = 100;
button.y = 200;

// 마우스 휠
SDL_MouseWheelEvent wheel;
wheel.y = 1;  // 위로 스크롤
```

**예상 결과**:
- 마우스 위치: (100, 200)
- 델타 이동: (10, -5)
- 왼쪽 버튼 상태: pressed
- 휠 델타: 1

**실제 결과**: [ ]
**통과/실패**: [ ]

---

#### TC-EVT-004: 윈도우 리사이즈 이벤트
**목적**: 윈도우 크기 변경 이벤트 처리 확인

**테스트 단계**:
1. 윈도우 리사이즈 이벤트 생성
2. 이벤트 처리
3. 뷰포트 업데이트 확인

**입력 데이터**:
```cpp
SDL_WindowEvent window_event;
window_event.event = SDL_WINDOWEVENT_RESIZED;
window_event.data1 = 1920;  // 새 너비
window_event.data2 = 1080;  // 새 높이
```

**예상 결과**:
- glViewport(0, 0, 1920, 1080) 호출
- 윈도우 크기 정보 업데이트
- 렌더링 정상 동작

**실제 결과**: [ ]
**통과/실패**: [ ]

---

#### TC-EVT-005: 이벤트 큐 오버플로우
**목적**: 대량의 이벤트 발생 시 안정성 확인

**테스트 단계**:
1. 1000개의 랜덤 이벤트 생성
2. 빠르게 이벤트 푸시
3. 시스템 안정성 확인

**예상 결과**:
- 크래시 없음
- 중요 이벤트(SDL_QUIT) 처리 보장
- 경고 로그 출력

**실제 결과**: [ ]
**통과/실패**: [ ]

### 2.3 타이밍 시스템 테스트

#### TC-TIME-001: 델타 타임 정확도
**목적**: 프레임 간 시간 계산의 정확도 확인

**테스트 단계**:
1. Timer 객체 생성
2. beginFrame() 호출
3. 16ms 대기
4. endFrame() 호출
5. getDeltaTime() 확인

**예상 결과**:
- 델타 타임: 0.016 ± 0.002초
- 연속 측정 시 일관성 유지

**실제 결과**: [ ]
**통과/실패**: [ ]

---

#### TC-TIME-002: FPS 계산 정확도
**목적**: FPS 측정의 정확도 확인

**테스트 단계**:
1. 60 FPS 타겟 설정
2. 120 프레임 실행
3. getCurrentFPS() 확인

**예상 결과**:
- 측정 FPS: 60 ± 5
- 1초 후 FPS 업데이트

**실제 결과**: [ ]
**통과/실패**: [ ]

---

#### TC-TIME-003: 프레임 레이트 제한
**목적**: 목표 FPS 유지 기능 확인

**테스트 단계**:
1. 30 FPS 설정
2. 빠른 루프 실행
3. 실제 프레임 시간 측정

**예상 결과**:
- 각 프레임 약 33.3ms
- CPU 사용률 적정 수준

**실제 결과**: [ ]
**통과/실패**: [ ]

---

#### TC-TIME-004: 큰 델타 타임 제한
**목적**: 프레임 드롭 시 델타 타임 제한 확인

**테스트 단계**:
1. beginFrame() 호출
2. 200ms 대기
3. endFrame() 호출
4. getDeltaTime() 확인

**예상 결과**:
- 델타 타임 최대 0.1초로 제한
- 게임 로직 안정성 유지

**실제 결과**: [ ]
**통과/실패**: [ ]

### 2.4 입력 상태 관리 테스트

#### TC-INPUT-001: 키 상태 전환
**목적**: 키보드 상태 전환 로직 확인

**테스트 케이스**:
| 상태 전환 | 현재 | 이전 | isPressed | isJustPressed | isJustReleased |
|-----------|------|------|-----------|---------------|----------------|
| 키 누름 시작 | true | false | true | true | false |
| 키 유지 | true | true | true | false | false |
| 키 해제 | false | true | false | false | true |
| 키 미사용 | false | false | false | false | false |

**실제 결과**: [ ]
**통과/실패**: [ ]

---

#### TC-INPUT-002: 마우스 드래그 감지
**목적**: 마우스 드래그 동작 감지 확인

**테스트 단계**:
1. 마우스 버튼 누름
2. 마우스 이동
3. isDragging() 확인
4. 마우스 버튼 해제
5. isDragging() 재확인

**예상 결과**:
- 버튼 누른 상태에서 이동 시 isDragging() = true
- 버튼 해제 후 isDragging() = false

**실제 결과**: [ ]
**통과/실패**: [ ]

---

#### TC-INPUT-003: 복합 키 입력
**목적**: 여러 키 동시 입력 처리 확인

**테스트 단계**:
1. Ctrl 키 누름
2. Shift 키 누름
3. A 키 누름
4. 상태 확인
5. 역순으로 해제

**예상 결과**:
- 모든 키 상태 독립적으로 추적
- 수정자 키 함수 정상 동작

**실제 결과**: [ ]
**통과/실패**: [ ]

### 2.5 성능 테스트

#### TC-PERF-001: 60 FPS 유지율
**목적**: 목표 프레임 레이트 유지 능력 확인

**테스트 조건**:
- 실행 시간: 60초
- 목표 FPS: 60
- 허용 편차: ±5%

**측정 항목**:
| 메트릭 | 목표값 | 허용 범위 |
|--------|--------|-----------|
| 평균 FPS | 60 | 57-63 |
| 최소 FPS | 55 | 50+ |
| 프레임 드롭 | < 1% | < 5% |

**실제 결과**: [ ]
**통과/실패**: [ ]

---

#### TC-PERF-002: 메모리 사용량
**목적**: 메모리 사용량 및 누수 확인

**테스트 단계**:
1. 초기 메모리 측정
2. 30분 실행
3. 최종 메모리 측정
4. 종료 후 메모리 확인

**허용 기준**:
| 항목 | 최대값 |
|------|--------|
| 초기 메모리 | 50 MB |
| 실행 중 증가량 | 10 MB |
| 메모리 누수 | 0 bytes |

**실제 결과**: [ ]
**통과/실패**: [ ]

---

#### TC-PERF-003: CPU 사용률
**목적**: CPU 사용 효율성 확인

**테스트 시나리오**:
| 상태 | 목표 CPU 사용률 |
|------|-----------------|
| 유휴 상태 | < 5% |
| 일반 실행 | < 25% |
| 최대 부하 | < 50% |

**실제 결과**: [ ]
**통과/실패**: [ ]

---

#### TC-PERF-004: 입력 지연
**목적**: 입력 응답성 측정

**테스트 방법**:
1. 키 입력 시점 기록
2. 이벤트 처리 시점 기록
3. 지연 시간 계산

**허용 기준**:
- 평균 지연: < 16ms (1 프레임)
- 최대 지연: < 33ms (2 프레임)

**실제 결과**: [ ]
**통과/실패**: [ ]

### 2.6 안정성 테스트

#### TC-STAB-001: 장시간 실행
**목적**: 장시간 실행 시 안정성 확인

**테스트 조건**:
- 실행 시간: 24시간
- 자동 입력 생성

**체크 항목**:
- [ ] 크래시 없음
- [ ] 메모리 누수 없음
- [ ] FPS 일관성 유지
- [ ] 에러 로그 없음

**실제 결과**: [ ]
**통과/실패**: [ ]

---

#### TC-STAB-002: 빠른 상태 전환
**목적**: 게임 상태 빠른 전환 시 안정성

**테스트 단계**:
1. MENU ↔ PLAYING 100회 전환
2. PLAYING ↔ PAUSED 100회 전환
3. 상태 일관성 확인

**예상 결과**:
- 상태 전환 중 크래시 없음
- 리소스 누수 없음
- 상태 정보 일관성 유지

**실제 결과**: [ ]
**통과/실패**: [ ]

---

#### TC-STAB-003: 윈도우 최소화/복원
**목적**: 윈도우 상태 변경 시 안정성

**테스트 단계**:
1. 윈도우 최소화
2. 1분 대기
3. 윈도우 복원
4. 렌더링 확인
5. 10회 반복

**예상 결과**:
- 복원 후 정상 렌더링
- GPU 리소스 유지
- 입력 처리 정상

**실제 결과**: [ ]
**통과/실패**: [ ]

### 2.7 에러 처리 테스트

#### TC-ERR-001: OpenGL 컨텍스트 실패
**목적**: OpenGL 초기화 실패 시 처리

**테스트 방법**:
- 지원하지 않는 OpenGL 버전 요청
- 에러 처리 확인

**예상 결과**:
- 적절한 에러 메시지
- 대체 버전 시도 또는 안전 종료

**실제 결과**: [ ]
**통과/실패**: [ ]

---

#### TC-ERR-002: 리소스 부족
**목적**: 시스템 리소스 부족 시 처리

**테스트 시나리오**:
1. 메모리 부족 시뮬레이션
2. GPU 메모리 부족 시뮬레이션

**예상 결과**:
- 크래시 방지
- 에러 로그 및 사용자 알림
- 안전 모드 전환

**실제 결과**: [ ]
**통과/실패**: [ ]

## 3. 테스트 자동화

### 3.1 단위 테스트 코드
```cpp
// test/EventLoopTest.cpp
#include <gtest/gtest.h>
#include "Application.h"
#include "EventSystem.h"
#include "Timer.h"

class EventLoopTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 테스트 환경 설정
        SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
    }
    
    void TearDown() override {
        SDL_Quit();
    }
};

TEST_F(EventLoopTest, ApplicationInitialization) {
    AppConfig config;
    config.windowWidth = 800;
    config.windowHeight = 600;
    
    Application app;
    EXPECT_TRUE(app.initialize(config));
    EXPECT_EQ(app.getState(), AppState::MENU);
}

TEST_F(EventLoopTest, KeyboardStateTracking) {
    KeyboardState keyboard;
    
    // 초기 상태
    EXPECT_FALSE(keyboard.isPressed(SDL_SCANCODE_A));
    
    // 키 누름
    keyboard.keys[SDL_SCANCODE_A] = true;
    keyboard.update();
    EXPECT_TRUE(keyboard.isPressed(SDL_SCANCODE_A));
    EXPECT_TRUE(keyboard.isJustPressed(SDL_SCANCODE_A));
    
    // 키 유지
    keyboard.update();
    EXPECT_TRUE(keyboard.isPressed(SDL_SCANCODE_A));
    EXPECT_FALSE(keyboard.isJustPressed(SDL_SCANCODE_A));
    
    // 키 해제
    keyboard.keys[SDL_SCANCODE_A] = false;
    keyboard.update();
    EXPECT_FALSE(keyboard.isPressed(SDL_SCANCODE_A));
    EXPECT_TRUE(keyboard.isJustReleased(SDL_SCANCODE_A));
}

TEST_F(EventLoopTest, TimerAccuracy) {
    Timer timer(60);
    
    auto start = std::chrono::high_resolution_clock::now();
    timer.beginFrame();
    
    std::this_thread::sleep_for(std::chrono::milliseconds(16));
    
    timer.endFrame();
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>
                   (end - start).count();
    
    EXPECT_NEAR(timer.getDeltaTime(), duration / 1000.0f, 0.002f);
}

TEST_F(EventLoopTest, MouseDragDetection) {
    MouseState mouse;
    
    // 드래그 시작
    mouse.buttons[0] = true;
    mouse.x = 100;
    mouse.y = 100;
    mouse.update();
    
    // 드래그 중
    mouse.x = 150;
    mouse.y = 120;
    mouse.deltaX = 50;
    mouse.deltaY = 20;
    mouse.update();
    
    EXPECT_TRUE(mouse.isDragging(0));
    EXPECT_EQ(mouse.deltaX, 50);
    EXPECT_EQ(mouse.deltaY, 20);
    
    // 드래그 종료
    mouse.buttons[0] = false;
    mouse.update();
    
    EXPECT_FALSE(mouse.isDragging(0));
}
```

### 3.2 통합 테스트 스크립트
```python
# test/integration_test.py
import subprocess
import time
import psutil
import pytest

class TestEventLoop:
    @pytest.fixture
    def app_process(self):
        """애플리케이션 프로세스 시작"""
        process = subprocess.Popen(['./build/notgate3', '--test-mode'])
        yield process
        process.terminate()
        process.wait()
    
    def test_startup_time(self, app_process):
        """시작 시간 테스트"""
        start_time = time.time()
        
        # 윈도우 생성 대기
        time.sleep(0.5)
        
        assert app_process.poll() is None, "프로세스가 예기치 않게 종료됨"
        startup_time = time.time() - start_time
        assert startup_time < 2.0, f"시작 시간이 너무 김: {startup_time}초"
    
    def test_memory_usage(self, app_process):
        """메모리 사용량 테스트"""
        time.sleep(1)  # 초기화 대기
        
        process = psutil.Process(app_process.pid)
        initial_memory = process.memory_info().rss / 1024 / 1024  # MB
        
        time.sleep(10)  # 10초 실행
        
        final_memory = process.memory_info().rss / 1024 / 1024  # MB
        memory_increase = final_memory - initial_memory
        
        assert initial_memory < 100, f"초기 메모리 사용량이 너무 큼: {initial_memory}MB"
        assert memory_increase < 10, f"메모리 증가량이 너무 큼: {memory_increase}MB"
    
    def test_cpu_usage(self, app_process):
        """CPU 사용률 테스트"""
        time.sleep(1)  # 초기화 대기
        
        process = psutil.Process(app_process.pid)
        
        # 5초간 평균 CPU 사용률 측정
        cpu_samples = []
        for _ in range(5):
            cpu_percent = process.cpu_percent(interval=1)
            cpu_samples.append(cpu_percent)
        
        avg_cpu = sum(cpu_samples) / len(cpu_samples)
        assert avg_cpu < 25, f"평균 CPU 사용률이 너무 높음: {avg_cpu}%"
```

### 3.3 성능 테스트 스크립트
```cpp
// test/performance_test.cpp
#include <chrono>
#include <vector>
#include <numeric>
#include <iostream>

class PerformanceTest {
public:
    void runFPSTest(int duration_seconds) {
        Application app;
        AppConfig config;
        
        if (!app.initialize(config)) {
            std::cerr << "초기화 실패\n";
            return;
        }
        
        auto start = std::chrono::steady_clock::now();
        auto end = start + std::chrono::seconds(duration_seconds);
        
        std::vector<float> fps_samples;
        int frame_count = 0;
        
        while (std::chrono::steady_clock::now() < end) {
            app.runSingleFrame();
            frame_count++;
            
            if (frame_count % 60 == 0) {
                fps_samples.push_back(app.getCurrentFPS());
            }
        }
        
        // 통계 계산
        float avg_fps = std::accumulate(fps_samples.begin(), 
                                       fps_samples.end(), 0.0f) 
                       / fps_samples.size();
        
        auto min_fps = *std::min_element(fps_samples.begin(), 
                                        fps_samples.end());
        auto max_fps = *std::max_element(fps_samples.begin(), 
                                        fps_samples.end());
        
        std::cout << "FPS 테스트 결과:\n";
        std::cout << "  평균 FPS: " << avg_fps << "\n";
        std::cout << "  최소 FPS: " << min_fps << "\n";
        std::cout << "  최대 FPS: " << max_fps << "\n";
        std::cout << "  총 프레임: " << frame_count << "\n";
        
        // 검증
        assert(avg_fps > 55.0f && avg_fps < 65.0f);
        assert(min_fps > 50.0f);
    }
    
    void runLatencyTest() {
        EventSystem eventSystem;
        
        std::vector<double> latencies;
        
        for (int i = 0; i < 1000; i++) {
            auto start = std::chrono::high_resolution_clock::now();
            
            SDL_Event event;
            event.type = SDL_KEYDOWN;
            event.key.keysym.scancode = SDL_SCANCODE_A;
            
            eventSystem.processEvent(event);
            
            auto end = std::chrono::high_resolution_clock::now();
            auto latency = std::chrono::duration<double, std::milli>
                          (end - start).count();
            
            latencies.push_back(latency);
        }
        
        double avg_latency = std::accumulate(latencies.begin(), 
                                            latencies.end(), 0.0) 
                           / latencies.size();
        
        std::cout << "입력 지연 테스트 결과:\n";
        std::cout << "  평균 지연: " << avg_latency << "ms\n";
        
        assert(avg_latency < 1.0);  // 1ms 미만
    }
};
```

## 4. 테스트 실행 계획

### 4.1 테스트 순서
1. **Phase 1**: 단위 테스트 (개발 중 지속적)
2. **Phase 2**: 통합 테스트 (기능 완성 후)
3. **Phase 3**: 성능 테스트 (최적화 전후)
4. **Phase 4**: 안정성 테스트 (릴리즈 전)

### 4.2 테스트 자동화
```yaml
# .github/workflows/test.yml
name: Event Loop Tests

on: [push, pull_request]

jobs:
  unit-tests:
    runs-on: ${{ matrix.os }}
    strategy:
      matrix:
        os: [ubuntu-latest, windows-latest, macos-latest]
    
    steps:
    - uses: actions/checkout@v2
    
    - name: Build
      run: |
        cmake -B build -DBUILD_TESTS=ON
        cmake --build build
    
    - name: Run Unit Tests
      run: |
        cd build
        ctest --output-on-failure
    
    - name: Run Performance Tests
      run: |
        ./build/performance_test
```

## 5. 테스트 보고서 템플릿

### 5.1 테스트 요약
| 항목 | 계획 | 실행 | 통과 | 실패 | 통과율 |
|------|------|------|------|------|--------|
| 단위 테스트 | 25 | | | | % |
| 통합 테스트 | 15 | | | | % |
| 성능 테스트 | 10 | | | | % |
| 안정성 테스트 | 5 | | | | % |
| **총계** | **55** | | | | % |

### 5.2 이슈 추적
| ID | 테스트 케이스 | 이슈 설명 | 심각도 | 상태 |
|----|--------------|-----------|--------|------|
| | | | | |

### 5.3 성능 메트릭
| 메트릭 | 목표 | 측정값 | 결과 |
|--------|------|--------|------|
| 평균 FPS | 60 | | |
| 메모리 사용량 | < 100MB | | |
| CPU 사용률 | < 25% | | |
| 시작 시간 | < 2초 | | |

## 6. 테스트 도구

### 6.1 필수 도구
- **Google Test**: 단위 테스트 프레임워크
- **Valgrind**: 메모리 누수 검사 (Linux)
- **Visual Studio Diagnostic Tools**: 성능 프로파일링 (Windows)
- **pytest**: Python 통합 테스트

### 6.2 설치 및 설정
```bash
# Ubuntu/Debian
sudo apt-get install libgtest-dev valgrind python3-pytest

# Windows (vcpkg)
vcpkg install gtest

# macOS
brew install googletest
pip3 install pytest psutil
```

## 7. 합격 기준

### 7.1 필수 합격 조건
- [ ] 모든 단위 테스트 통과
- [ ] 메모리 누수 없음
- [ ] 60 FPS ±5% 유지
- [ ] 크래시 없이 1시간 이상 실행

### 7.2 권장 합격 조건
- [ ] 모든 통합 테스트 통과
- [ ] CPU 사용률 25% 미만
- [ ] 입력 지연 16ms 미만
- [ ] 24시간 안정성 테스트 통과