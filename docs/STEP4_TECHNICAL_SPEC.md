# Step 4: 입력 처리 시스템 기술 명세서

## 1. 아키텍처 설계

### 1.1 시스템 구조
```
┌──────────────────────────────────────────────────┐
│                  Application Layer                │
├──────────────────────────────────────────────────┤
│                  InputManager                     │
│  ┌──────────────────────────────────────────┐   │
│  │ EventHandler │ CoordTransform │ HitDetect │   │
│  └──────────────────────────────────────────┘   │
├──────────────────────────────────────────────────┤
│         SDL2          │          OpenGL          │
└──────────────────────────────────────────────────┘
```

### 1.2 클래스 다이어그램
```cpp
class InputManager {
    // 의존성
    Camera* m_camera;
    Circuit* m_circuit;
    Renderer* m_renderer;
    
    // 상태 관리
    MouseState m_mouseState;
    DragState m_dragState;
    SelectionState m_selectionState;
    
    // 이벤트 처리
    EventDispatcher m_dispatcher;
    CoordinateTransformer m_transformer;
    HitDetector m_hitDetector;
}
```

## 2. 핵심 컴포넌트 설계

### 2.1 좌표 변환 시스템

#### 2.1.1 CoordinateTransformer 클래스
```cpp
class CoordinateTransformer {
private:
    glm::mat4 m_viewMatrix;
    glm::mat4 m_projMatrix;
    glm::vec2 m_viewportSize;
    float m_gridSize;
    
public:
    // 변환 함수들
    glm::vec2 screenToNDC(const glm::vec2& screenPos) {
        return glm::vec2(
            2.0f * screenPos.x / m_viewportSize.x - 1.0f,
            1.0f - 2.0f * screenPos.y / m_viewportSize.y
        );
    }
    
    glm::vec2 ndcToWorld(const glm::vec2& ndcPos) {
        glm::mat4 invVP = glm::inverse(m_projMatrix * m_viewMatrix);
        glm::vec4 worldPos = invVP * glm::vec4(ndcPos, 0.0f, 1.0f);
        return glm::vec2(worldPos.x / worldPos.w, worldPos.y / worldPos.w);
    }
    
    glm::ivec2 worldToGrid(const glm::vec2& worldPos) {
        return glm::ivec2(
            static_cast<int>(std::floor(worldPos.x / m_gridSize)),
            static_cast<int>(std::floor(worldPos.y / m_gridSize))
        );
    }
    
    glm::vec2 gridToWorld(const glm::ivec2& gridPos) {
        return glm::vec2(
            (gridPos.x + 0.5f) * m_gridSize,
            (gridPos.y + 0.5f) * m_gridSize
        );
    }
};
```

#### 2.1.2 변환 행렬 캐싱
```cpp
class TransformCache {
private:
    struct CacheEntry {
        glm::mat4 viewProj;
        glm::mat4 invViewProj;
        uint32_t frameNumber;
    };
    
    CacheEntry m_cache;
    
public:
    void update(const glm::mat4& view, const glm::mat4& proj, uint32_t frame) {
        if (m_cache.frameNumber != frame) {
            m_cache.viewProj = proj * view;
            m_cache.invViewProj = glm::inverse(m_cache.viewProj);
            m_cache.frameNumber = frame;
        }
    }
    
    const glm::mat4& getInvViewProj() const { return m_cache.invViewProj; }
};
```

### 2.2 히트 감지 시스템

#### 2.2.1 HitDetector 클래스
```cpp
class HitDetector {
private:
    // 공간 분할 구조
    struct SpatialGrid {
        static constexpr int CELL_SIZE = 10;  // 10x10 그리드 셀
        std::unordered_map<uint64_t, std::vector<uint32_t>> gateIndex;
        std::unordered_map<uint64_t, std::vector<uint32_t>> wireIndex;
        
        uint64_t hashPosition(const glm::ivec2& pos) {
            int32_t cellX = pos.x / CELL_SIZE;
            int32_t cellY = pos.y / CELL_SIZE;
            return (static_cast<uint64_t>(cellX) << 32) | static_cast<uint64_t>(cellY);
        }
    };
    
    SpatialGrid m_spatialGrid;
    Circuit* m_circuit;
    
public:
    struct HitResult {
        enum Type { None, Gate, Wire, Port } type;
        uint32_t objectId;
        float distance;
        glm::vec2 hitPoint;
    };
    
    HitResult detectHit(const glm::vec2& worldPos) {
        HitResult result = {HitResult::None, 0, FLT_MAX, worldPos};
        
        // 1. 게이트 히트 체크 (우선순위 높음)
        if (auto gateHit = checkGateHit(worldPos)) {
            return gateHit;
        }
        
        // 2. 와이어 히트 체크
        if (auto wireHit = checkWireHit(worldPos)) {
            return wireHit;
        }
        
        return result;
    }
    
private:
    HitResult checkGateHit(const glm::vec2& worldPos) {
        glm::ivec2 gridPos = worldToGrid(worldPos);
        
        // 공간 인덱스에서 후보 게이트 조회
        uint64_t cellHash = m_spatialGrid.hashPosition(gridPos);
        auto it = m_spatialGrid.gateIndex.find(cellHash);
        if (it == m_spatialGrid.gateIndex.end()) {
            return {HitResult::None, 0, FLT_MAX, worldPos};
        }
        
        // AABB 테스트
        for (uint32_t gateId : it->second) {
            const Gate& gate = m_circuit->getGate(gateId);
            glm::vec2 min = gate.position - glm::vec2(0.5f);
            glm::vec2 max = gate.position + glm::vec2(0.5f);
            
            if (worldPos.x >= min.x && worldPos.x <= max.x &&
                worldPos.y >= min.y && worldPos.y <= max.y) {
                return {HitResult::Gate, gateId, 0, worldPos};
            }
        }
        
        return {HitResult::None, 0, FLT_MAX, worldPos};
    }
    
    HitResult checkWireHit(const glm::vec2& worldPos) {
        constexpr float HIT_THRESHOLD = 0.1f;
        HitResult closest = {HitResult::None, 0, FLT_MAX, worldPos};
        
        // 주변 셀 검색 (3x3)
        glm::ivec2 gridPos = worldToGrid(worldPos);
        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                glm::ivec2 checkPos = gridPos + glm::ivec2(dx, dy);
                uint64_t cellHash = m_spatialGrid.hashPosition(checkPos);
                
                auto it = m_spatialGrid.wireIndex.find(cellHash);
                if (it == m_spatialGrid.wireIndex.end()) continue;
                
                for (uint32_t wireId : it->second) {
                    const Wire& wire = m_circuit->getWire(wireId);
                    float dist = distanceToLineSegment(worldPos, wire.start, wire.end);
                    
                    if (dist < HIT_THRESHOLD && dist < closest.distance) {
                        closest = {HitResult::Wire, wireId, dist, worldPos};
                    }
                }
            }
        }
        
        return closest;
    }
    
    float distanceToLineSegment(const glm::vec2& p, const glm::vec2& a, const glm::vec2& b) {
        glm::vec2 ab = b - a;
        glm::vec2 ap = p - a;
        float t = glm::clamp(glm::dot(ap, ab) / glm::dot(ab, ab), 0.0f, 1.0f);
        glm::vec2 closest = a + t * ab;
        return glm::length(p - closest);
    }
};
```

### 2.3 이벤트 처리 시스템

#### 2.3.1 이벤트 구조체 정의
```cpp
// 기본 이벤트
struct MouseEvent {
    enum Type { Move, Down, Up, Wheel } type;
    glm::vec2 screenPos;
    glm::vec2 worldPos;
    glm::ivec2 gridPos;
    int button;  // 0=left, 1=middle, 2=right
    float wheelDelta;
    uint32_t timestamp;
};

// 클릭 이벤트
struct ClickEvent {
    glm::vec2 worldPos;
    glm::ivec2 gridPos;
    HitDetector::HitResult hit;
    int button;
    bool doubleClick;
    uint32_t timestamp;
};

// 드래그 이벤트
struct DragEvent {
    enum Phase { Start, Move, End, Cancel } phase;
    glm::vec2 startWorld;
    glm::vec2 currentWorld;
    glm::vec2 deltaWorld;
    glm::ivec2 startGrid;
    glm::ivec2 currentGrid;
    HitDetector::HitResult dragTarget;
    float distance;
    float duration;
};
```

#### 2.3.2 EventDispatcher 클래스
```cpp
template<typename EventType>
using EventCallback = std::function<void(const EventType&)>;

class EventDispatcher {
private:
    // 타입별 콜백 저장
    std::unordered_map<std::type_index, std::vector<std::any>> m_callbacks;
    
    // 이벤트 큐 (프레임 배칭)
    std::queue<std::function<void()>> m_eventQueue;
    
public:
    template<typename EventType>
    void subscribe(EventCallback<EventType> callback) {
        auto& callbacks = m_callbacks[std::type_index(typeid(EventType))];
        callbacks.push_back(callback);
    }
    
    template<typename EventType>
    void dispatch(const EventType& event) {
        auto it = m_callbacks.find(std::type_index(typeid(EventType)));
        if (it != m_callbacks.end()) {
            for (const auto& anyCallback : it->second) {
                auto callback = std::any_cast<EventCallback<EventType>>(anyCallback);
                callback(event);
            }
        }
    }
    
    template<typename EventType>
    void enqueue(const EventType& event) {
        m_eventQueue.push([this, event]() { dispatch(event); });
    }
    
    void processQueue() {
        while (!m_eventQueue.empty()) {
            m_eventQueue.front()();
            m_eventQueue.pop();
        }
    }
};
```

### 2.4 드래그 처리 시스템

#### 2.4.1 DragState 관리
```cpp
class DragManager {
private:
    enum State { Idle, Potential, Active };
    
    struct DragInfo {
        State state = Idle;
        glm::vec2 startPos;
        glm::vec2 currentPos;
        glm::vec2 lastPos;
        HitDetector::HitResult target;
        float accumDistance = 0;
        uint32_t startTime;
        int button;
    };
    
    DragInfo m_dragInfo;
    static constexpr float DRAG_THRESHOLD = 5.0f;  // 픽셀
    
public:
    void onMouseDown(const MouseEvent& event) {
        if (m_dragInfo.state == Idle) {
            m_dragInfo.state = Potential;
            m_dragInfo.startPos = event.screenPos;
            m_dragInfo.currentPos = event.screenPos;
            m_dragInfo.lastPos = event.screenPos;
            m_dragInfo.button = event.button;
            m_dragInfo.startTime = event.timestamp;
            m_dragInfo.accumDistance = 0;
            
            // 드래그 대상 감지
            HitDetector detector;
            m_dragInfo.target = detector.detectHit(event.worldPos);
        }
    }
    
    void onMouseMove(const MouseEvent& event) {
        if (m_dragInfo.state == Potential) {
            float distance = glm::length(event.screenPos - m_dragInfo.startPos);
            if (distance > DRAG_THRESHOLD) {
                m_dragInfo.state = Active;
                
                // 드래그 시작 이벤트
                DragEvent dragEvent;
                dragEvent.phase = DragEvent::Start;
                dragEvent.startWorld = screenToWorld(m_dragInfo.startPos);
                dragEvent.currentWorld = event.worldPos;
                dragEvent.dragTarget = m_dragInfo.target;
                dispatcher.dispatch(dragEvent);
            }
        } else if (m_dragInfo.state == Active) {
            // 드래그 이동 이벤트
            DragEvent dragEvent;
            dragEvent.phase = DragEvent::Move;
            dragEvent.startWorld = screenToWorld(m_dragInfo.startPos);
            dragEvent.currentWorld = event.worldPos;
            dragEvent.deltaWorld = event.worldPos - screenToWorld(m_dragInfo.lastPos);
            dragEvent.distance = glm::length(event.screenPos - m_dragInfo.startPos);
            dragEvent.duration = (event.timestamp - m_dragInfo.startTime) / 1000.0f;
            dragEvent.dragTarget = m_dragInfo.target;
            dispatcher.dispatch(dragEvent);
            
            m_dragInfo.lastPos = event.screenPos;
            m_dragInfo.currentPos = event.screenPos;
        }
    }
    
    void onMouseUp(const MouseEvent& event) {
        if (m_dragInfo.state == Potential) {
            // 클릭으로 처리
            ClickEvent clickEvent;
            clickEvent.worldPos = event.worldPos;
            clickEvent.gridPos = event.gridPos;
            clickEvent.button = event.button;
            clickEvent.hit = m_dragInfo.target;
            dispatcher.dispatch(clickEvent);
        } else if (m_dragInfo.state == Active) {
            // 드래그 종료 이벤트
            DragEvent dragEvent;
            dragEvent.phase = DragEvent::End;
            dragEvent.startWorld = screenToWorld(m_dragInfo.startPos);
            dragEvent.currentWorld = event.worldPos;
            dragEvent.distance = glm::length(event.screenPos - m_dragInfo.startPos);
            dragEvent.duration = (event.timestamp - m_dragInfo.startTime) / 1000.0f;
            dragEvent.dragTarget = m_dragInfo.target;
            dispatcher.dispatch(dragEvent);
        }
        
        m_dragInfo.state = Idle;
    }
};
```

## 3. InputManager 통합 클래스

### 3.1 전체 구조
```cpp
class InputManager {
private:
    // 의존성 주입
    Camera* m_camera;
    Circuit* m_circuit;
    
    // 컴포넌트
    CoordinateTransformer m_transformer;
    HitDetector m_hitDetector;
    EventDispatcher m_dispatcher;
    DragManager m_dragManager;
    
    // 상태
    struct MouseState {
        glm::vec2 position;
        glm::vec2 lastPosition;
        bool buttons[3] = {false, false, false};
        bool buttonsPressed[3] = {false, false, false};
        bool buttonsReleased[3] = {false, false, false};
        float scrollDelta = 0;
    } m_mouseState;
    
    struct SelectionState {
        std::unordered_set<uint32_t> selectedGates;
        std::unordered_set<uint32_t> selectedWires;
        uint32_t primarySelection = 0;
    } m_selection;
    
    // 설정
    struct Settings {
        float dragThreshold = 5.0f;
        float doubleClickTime = 0.3f;
        float wireHitThreshold = 0.1f;
        bool invertScroll = false;
    } m_settings;
    
public:
    InputManager(Camera* camera, Circuit* circuit) 
        : m_camera(camera), m_circuit(circuit) {
        m_hitDetector.setCircuit(circuit);
        m_transformer.setCamera(camera);
    }
    
    void handleEvent(const SDL_Event& event) {
        switch (event.type) {
            case SDL_MOUSEMOTION:
                handleMouseMove(event.motion);
                break;
            case SDL_MOUSEBUTTONDOWN:
                handleMouseDown(event.button);
                break;
            case SDL_MOUSEBUTTONUP:
                handleMouseUp(event.button);
                break;
            case SDL_MOUSEWHEEL:
                handleMouseWheel(event.wheel);
                break;
        }
    }
    
    void update(float deltaTime) {
        // 상태 업데이트
        updateMouseState();
        
        // 이벤트 큐 처리
        m_dispatcher.processQueue();
        
        // 호버 감지
        updateHover();
    }
    
private:
    void handleMouseMove(const SDL_MouseMotionEvent& event) {
        m_mouseState.lastPosition = m_mouseState.position;
        m_mouseState.position = glm::vec2(event.x, event.y);
        
        MouseEvent mouseEvent;
        mouseEvent.type = MouseEvent::Move;
        mouseEvent.screenPos = m_mouseState.position;
        mouseEvent.worldPos = m_transformer.screenToWorld(mouseEvent.screenPos);
        mouseEvent.gridPos = m_transformer.worldToGrid(mouseEvent.worldPos);
        mouseEvent.timestamp = SDL_GetTicks();
        
        m_dragManager.onMouseMove(mouseEvent);
    }
    
    void handleMouseDown(const SDL_MouseButtonEvent& event) {
        int button = event.button - 1;  // SDL uses 1-based indexing
        if (button >= 0 && button < 3) {
            m_mouseState.buttons[button] = true;
            m_mouseState.buttonsPressed[button] = true;
            
            MouseEvent mouseEvent;
            mouseEvent.type = MouseEvent::Down;
            mouseEvent.button = button;
            mouseEvent.screenPos = glm::vec2(event.x, event.y);
            mouseEvent.worldPos = m_transformer.screenToWorld(mouseEvent.screenPos);
            mouseEvent.gridPos = m_transformer.worldToGrid(mouseEvent.worldPos);
            mouseEvent.timestamp = SDL_GetTicks();
            
            m_dragManager.onMouseDown(mouseEvent);
        }
    }
    
    void handleMouseUp(const SDL_MouseButtonEvent& event) {
        int button = event.button - 1;
        if (button >= 0 && button < 3) {
            m_mouseState.buttons[button] = false;
            m_mouseState.buttonsReleased[button] = true;
            
            MouseEvent mouseEvent;
            mouseEvent.type = MouseEvent::Up;
            mouseEvent.button = button;
            mouseEvent.screenPos = glm::vec2(event.x, event.y);
            mouseEvent.worldPos = m_transformer.screenToWorld(mouseEvent.screenPos);
            mouseEvent.gridPos = m_transformer.worldToGrid(mouseEvent.worldPos);
            mouseEvent.timestamp = SDL_GetTicks();
            
            m_dragManager.onMouseUp(mouseEvent);
        }
    }
    
    void updateMouseState() {
        // 프레임 단위 버튼 상태 리셋
        for (int i = 0; i < 3; i++) {
            m_mouseState.buttonsPressed[i] = false;
            m_mouseState.buttonsReleased[i] = false;
        }
        m_mouseState.scrollDelta = 0;
    }
};
```

## 4. 성능 최적화

### 4.1 공간 인덱싱
```cpp
class SpatialIndex {
private:
    struct Cell {
        std::vector<uint32_t> gates;
        std::vector<uint32_t> wires;
    };
    
    std::unordered_map<uint64_t, Cell> m_cells;
    int m_cellSize = 10;
    
public:
    uint64_t hashPosition(int x, int y) {
        uint32_t ux = static_cast<uint32_t>(x + 0x7FFFFFFF);
        uint32_t uy = static_cast<uint32_t>(y + 0x7FFFFFFF);
        return (static_cast<uint64_t>(ux) << 32) | uy;
    }
    
    void insertGate(uint32_t id, const glm::ivec2& pos) {
        int cellX = pos.x / m_cellSize;
        int cellY = pos.y / m_cellSize;
        uint64_t hash = hashPosition(cellX, cellY);
        m_cells[hash].gates.push_back(id);
    }
    
    std::vector<uint32_t> queryGates(const glm::ivec2& pos, int radius = 0) {
        std::vector<uint32_t> result;
        int cellX = pos.x / m_cellSize;
        int cellY = pos.y / m_cellSize;
        
        for (int dy = -radius; dy <= radius; dy++) {
            for (int dx = -radius; dx <= radius; dx++) {
                uint64_t hash = hashPosition(cellX + dx, cellY + dy);
                auto it = m_cells.find(hash);
                if (it != m_cells.end()) {
                    result.insert(result.end(), 
                                it->second.gates.begin(), 
                                it->second.gates.end());
                }
            }
        }
        
        return result;
    }
};
```

### 4.2 이벤트 배칭
```cpp
class EventBatcher {
private:
    struct BatchedEvents {
        std::vector<MouseEvent> mouseEvents;
        std::vector<ClickEvent> clickEvents;
        std::vector<DragEvent> dragEvents;
    };
    
    BatchedEvents m_currentBatch;
    BatchedEvents m_processingBatch;
    std::mutex m_batchMutex;
    
public:
    template<typename EventType>
    void addEvent(const EventType& event) {
        std::lock_guard<std::mutex> lock(m_batchMutex);
        if constexpr (std::is_same_v<EventType, MouseEvent>) {
            m_currentBatch.mouseEvents.push_back(event);
        }
        // ... 다른 이벤트 타입들
    }
    
    void processBatch() {
        {
            std::lock_guard<std::mutex> lock(m_batchMutex);
            std::swap(m_currentBatch, m_processingBatch);
        }
        
        // 배치 처리
        for (const auto& event : m_processingBatch.mouseEvents) {
            // 처리...
        }
        
        m_processingBatch.mouseEvents.clear();
    }
};
```

## 5. 메모리 관리

### 5.1 객체 풀
```cpp
template<typename T>
class ObjectPool {
private:
    std::vector<T> m_pool;
    std::queue<T*> m_available;
    
public:
    ObjectPool(size_t initialSize = 100) {
        m_pool.reserve(initialSize);
        for (size_t i = 0; i < initialSize; i++) {
            m_pool.emplace_back();
            m_available.push(&m_pool.back());
        }
    }
    
    T* acquire() {
        if (m_available.empty()) {
            m_pool.emplace_back();
            return &m_pool.back();
        }
        
        T* obj = m_available.front();
        m_available.pop();
        return obj;
    }
    
    void release(T* obj) {
        obj->reset();  // T must have reset() method
        m_available.push(obj);
    }
};
```

### 5.2 메모리 레이아웃
```cpp
// 캐시 친화적 구조체 정의
struct alignas(64) CacheAlignedMouseState {
    glm::vec2 position;
    glm::vec2 lastPosition;
    uint8_t buttons;         // 비트 플래그
    uint8_t buttonsPressed;
    uint8_t buttonsReleased;
    uint8_t _padding[5];
    float scrollDelta;
};
```

## 6. 스레드 안전성

### 6.1 락프리 이벤트 큐
```cpp
template<typename T>
class LockFreeQueue {
private:
    struct Node {
        std::atomic<Node*> next;
        T data;
    };
    
    std::atomic<Node*> m_head;
    std::atomic<Node*> m_tail;
    
public:
    void enqueue(const T& item) {
        Node* newNode = new Node{nullptr, item};
        Node* prevTail = m_tail.exchange(newNode);
        prevTail->next.store(newNode);
    }
    
    bool dequeue(T& item) {
        Node* head = m_head.load();
        Node* next = head->next.load();
        
        if (next == nullptr) {
            return false;
        }
        
        item = next->data;
        m_head.store(next);
        delete head;
        return true;
    }
};
```

## 7. 디버그 및 프로파일링

### 7.1 디버그 시각화
```cpp
class InputDebugOverlay {
private:
    bool m_enabled = false;
    
public:
    void render(const InputManager& input) {
        if (!m_enabled) return;
        
        ImGui::Begin("Input Debug");
        
        // 마우스 정보
        auto mousePos = input.getMousePos();
        auto worldPos = input.getWorldPos();
        auto gridPos = input.getGridPos();
        
        ImGui::Text("Screen: %.1f, %.1f", mousePos.x, mousePos.y);
        ImGui::Text("World: %.2f, %.2f", worldPos.x, worldPos.y);
        ImGui::Text("Grid: %d, %d", gridPos.x, gridPos.y);
        
        // 히트 정보
        auto hit = input.getLastHit();
        ImGui::Text("Hit: %s", hit.type == HitResult::Gate ? "Gate" : 
                              hit.type == HitResult::Wire ? "Wire" : "None");
        
        // 드래그 정보
        if (input.isDragging()) {
            ImGui::Text("Dragging: %.1f pixels", input.getDragDistance());
        }
        
        ImGui::End();
    }
};
```

### 7.2 성능 프로파일링
```cpp
class InputProfiler {
private:
    struct TimingData {
        double transformTime = 0;
        double hitDetectTime = 0;
        double eventDispatchTime = 0;
        int eventCount = 0;
    };
    
    TimingData m_current;
    TimingData m_average;
    
public:
    template<typename Func>
    void measure(const char* name, Func func) {
        auto start = std::chrono::high_resolution_clock::now();
        func();
        auto end = std::chrono::high_resolution_clock::now();
        
        double ms = std::chrono::duration<double, std::milli>(end - start).count();
        
        if (strcmp(name, "transform") == 0) {
            m_current.transformTime = ms;
        } else if (strcmp(name, "hitdetect") == 0) {
            m_current.hitDetectTime = ms;
        }
        // ...
    }
    
    void report() {
        ImGui::Text("Transform: %.3fms", m_average.transformTime);
        ImGui::Text("Hit Detect: %.3fms", m_average.hitDetectTime);
        ImGui::Text("Event Dispatch: %.3fms", m_average.eventDispatchTime);
        ImGui::Text("Events/Frame: %d", m_average.eventCount);
    }
};
```

## 8. 플랫폼별 고려사항

### 8.1 Windows
```cpp
#ifdef _WIN32
    // 고DPI 지원
    void adjustForDPI() {
        UINT dpi = GetDpiForWindow(GetActiveWindow());
        float scale = dpi / 96.0f;
        m_transformer.setDPIScale(scale);
    }
#endif
```

### 8.2 Linux
```cpp
#ifdef __linux__
    // X11 이벤트 처리
    void handleX11Event(XEvent* event) {
        // 추가 X11 특화 이벤트 처리
    }
#endif
```

### 8.3 macOS
```cpp
#ifdef __APPLE__
    // Retina 디스플레이 지원
    void adjustForRetina() {
        int drawableW, drawableH;
        int windowW, windowH;
        SDL_GL_GetDrawableSize(window, &drawableW, &drawableH);
        SDL_GetWindowSize(window, &windowW, &windowH);
        float scale = (float)drawableW / windowW;
        m_transformer.setPixelScale(scale);
    }
#endif
```

## 9. 테스트 코드

### 9.1 단위 테스트
```cpp
TEST(CoordinateTransform, ScreenToWorld) {
    CoordinateTransformer transformer;
    transformer.setViewport(800, 600);
    transformer.setCamera(glm::vec2(0, 0), 1.0f);
    
    // 중앙 클릭
    glm::vec2 screen(400, 300);
    glm::vec2 world = transformer.screenToWorld(screen);
    EXPECT_NEAR(world.x, 0.0f, 0.001f);
    EXPECT_NEAR(world.y, 0.0f, 0.001f);
    
    // 코너 클릭
    screen = glm::vec2(0, 0);
    world = transformer.screenToWorld(screen);
    EXPECT_LT(world.x, 0);
    EXPECT_GT(world.y, 0);
}

TEST(HitDetector, GateHit) {
    Circuit circuit;
    HitDetector detector(&circuit);
    
    // 게이트 추가
    circuit.addGate(Gate{1, GateType::NOT, glm::vec2(5, 5)});
    
    // 중앙 클릭
    auto hit = detector.detectHit(glm::vec2(5, 5));
    EXPECT_EQ(hit.type, HitResult::Gate);
    EXPECT_EQ(hit.objectId, 1);
    
    // 경계 클릭
    hit = detector.detectHit(glm::vec2(5.49f, 5.49f));
    EXPECT_EQ(hit.type, HitResult::Gate);
    
    // 외부 클릭
    hit = detector.detectHit(glm::vec2(5.51f, 5.51f));
    EXPECT_EQ(hit.type, HitResult::None);
}
```

## 10. 빌드 설정

### 10.1 CMake 구성
```cmake
# InputManager 라이브러리
add_library(InputManager STATIC
    src/input/InputManager.cpp
    src/input/CoordinateTransformer.cpp
    src/input/HitDetector.cpp
    src/input/EventDispatcher.cpp
    src/input/DragManager.cpp
)

target_include_directories(InputManager PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(InputManager PUBLIC
    SDL2::SDL2
    glm::glm
    Circuit
    Camera
)

# 최적화 플래그
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(InputManager PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-O3 -march=native>
        $<$<CXX_COMPILER_ID:MSVC>:/O2 /arch:AVX2>
    )
endif()

# 테스트
if(BUILD_TESTS)
    add_executable(InputManagerTest
        test/InputManagerTest.cpp
    )
    target_link_libraries(InputManagerTest
        InputManager
        gtest_main
    )
endif()
```