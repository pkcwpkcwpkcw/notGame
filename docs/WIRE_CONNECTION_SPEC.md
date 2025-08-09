# Wire Connection System - Technical Specification

## Executive Summary

The Wire Connection System implements Step 6 of the NOT Gate game roadmap, providing intuitive drag-based wire creation, connection validation, visual feedback, and high-performance wire management for supporting 100,000+ wires in sandbox mode. This system integrates with existing Circuit, Gate, InputManager, and rendering components while maintaining the project's cache-friendly, SIMD-optimized architecture.

**Key Architectural Decisions:**
- **Performance-First Design**: Structure-of-Arrays (SoA) memory layout with 64-byte cache line alignment
- **Drag-Based Interaction**: Leverages existing DragManager for consistent user experience  
- **Visual Feedback System**: Real-time connection previews and port highlighting
- **Spatial Optimization**: Grid-based spatial indexing for efficient wire hit detection
- **Memory Pool Architecture**: Pre-allocated pools to eliminate runtime allocations

**Technology Stack Summary:**
- C++20 with performance-critical SIMD optimizations
- OpenGL-accelerated rendering with batched draw calls
- SDL2-based input handling through existing InputManager
- Cache-friendly memory layouts (64-byte alignment)

## System Architecture

### High-Level Architecture Diagram (ASCII)

```
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│   InputManager  │────│ WireConnection  │────│   Circuit       │
│   - DragManager │    │   Manager       │    │   - Gates       │
│   - HitDetector │    │                 │    │   - Wires       │
└─────────────────┘    └─────────────────┘    └─────────────────┘
         │                       │                       │
         │              ┌─────────────────┐               │
         └──────────────│ WireRenderer    │───────────────┘
                        │ - BatchRenderer │
                        │ - PortRenderer  │
                        └─────────────────┘
                                │
                        ┌─────────────────┐
                        │   OpenGL        │
                        │   Shaders       │
                        └─────────────────┘
```

### Component Breakdown

1. **WireConnectionManager**: Core logic for wire creation and validation
2. **WirePreviewSystem**: Visual feedback during drag operations  
3. **PortHighlightSystem**: Interactive port highlighting and validation
4. **WireSpatialIndex**: Grid-based spatial indexing for performance
5. **WireConnectionValidator**: Connection rule enforcement
6. **WirePathCalculator**: Automatic wire routing algorithms

### Data Flow Diagram

```
Mouse Down (Port) → DragStart → Preview Creation
       │                            │
       v                            v
   Hit Detection ──────→ Port Highlighting
       │                            │  
       v                            v
Mouse Move → Update Preview → Visual Feedback
       │                            │
       v                            v
Mouse Up (Port) → Validation → Wire Creation
       │                            │
       v                            v
   Connection → Circuit Update → Rendering
```

### Integration Points with Existing Systems

- **InputManager**: Subscribes to drag events from existing DragManager
- **Circuit**: Uses existing wire creation API with enhanced validation
- **Renderer**: Extends WireRenderer with preview and highlight capabilities
- **Gate**: Leverages existing port position and connection methods

## Technology Stack Details

### Core Technologies

**Programming Language**: C++20
- **Rationale**: Native performance required for 100k+ wire simulation
- **Key Features Used**: constexpr, concepts, structured bindings, designated initializers

**Graphics Pipeline**: OpenGL 3.3+ with SDL2
- **Vertex Shaders**: Efficient wire path rendering with instanced drawing
- **Fragment Shaders**: Anti-aliased wire rendering with signal state colors
- **Geometry Shaders**: Dynamic wire thickness and connection indicators

**Memory Architecture**: Structure of Arrays (SoA)
- **Cache Optimization**: 64-byte aligned data structures
- **SIMD Ready**: AVX2-compatible layouts for future optimization

### Performance Requirements

| Mode | Wires | Target FPS | Memory | Response Time |
|------|-------|------------|--------|---------------|
| Puzzle | 1,000 | 60 FPS | 1MB | <16ms |
| Normal | 10,000 | 60 FPS | 10MB | <16ms |
| Sandbox | 100,000+ | 60 FPS | 100MB | <16ms |

## API Specification

### WireConnectionManager Class Interface

```cpp
class WireConnectionManager {
public:
    // Core API
    ErrorCode initialize(Circuit* circuit, InputManager* input, 
                        WireRenderer* renderer) noexcept;
    void update(float deltaTime) noexcept;
    void shutdown() noexcept;
    
    // Wire Creation API
    [[nodiscard]] Result<WireId> createWire(
        GateId fromGate, PortIndex fromPort,
        GateId toGate, PortIndex toPort) noexcept;
    
    ErrorCode deleteWire(WireId wireId) noexcept;
    ErrorCode deleteWireAt(Vec2 worldPosition, float tolerance = 0.1f) noexcept;
    
    // Preview System API
    void startWirePreview(GateId fromGate, PortIndex fromPort, 
                         Vec2 worldPosition) noexcept;
    void updateWirePreview(Vec2 currentPosition) noexcept;
    void endWirePreview() noexcept;
    void cancelWirePreview() noexcept;
    
    // Port Highlighting API
    void highlightCompatiblePorts(GateId sourceGate, PortIndex sourcePort) noexcept;
    void clearPortHighlights() noexcept;
    [[nodiscard]] HighlightState getPortHighlight(GateId gateId, 
                                                PortIndex port) const noexcept;
    
    // Validation API
    [[nodiscard]] bool canConnect(GateId fromGate, PortIndex fromPort,
                                GateId toGate, PortIndex toPort) const noexcept;
    [[nodiscard]] ValidationResult validateConnection(
        GateId fromGate, PortIndex fromPort,
        GateId toGate, PortIndex toPort) const noexcept;
    
    // Query API
    [[nodiscard]] WireId getWireAt(Vec2 worldPosition, 
                                  float tolerance = 0.1f) const noexcept;
    [[nodiscard]] std::vector<WireId> getWiresInRect(const Rect& bounds) const noexcept;
    [[nodiscard]] PortInfo getPortAt(Vec2 worldPosition, 
                                   float tolerance = 0.2f) const noexcept;
    
    // Settings API
    void setDragThreshold(float threshold) noexcept;
    void setWireHitTolerance(float tolerance) noexcept;
    void setPortHighlightRadius(float radius) noexcept;
    
    // Statistics API
    [[nodiscard]] WireConnectionStats getStats() const noexcept;
    
private:
    // Implementation details hidden
};
```

### Request/Response Schemas

#### Wire Creation Request
```cpp
struct WireCreateRequest {
    GateId fromGateId{Constants::INVALID_GATE_ID};
    PortIndex fromPort{Constants::INVALID_PORT};
    GateId toGateId{Constants::INVALID_GATE_ID};
    PortIndex toPort{Constants::INVALID_PORT};
    
    // Optional path override
    std::vector<Vec2> customPath;
    bool useCustomPath{false};
    
    // Connection metadata
    uint32_t priority{0};
    std::string debugName;
};
```

#### Validation Response
```cpp
enum class ValidationError : uint8_t {
    None = 0,
    InvalidSourceGate,
    InvalidTargetGate,
    InvalidPort,
    PortAlreadyConnected,
    SameGateConnection,
    CircularDependency,
    TooManyConnections
};

struct ValidationResult {
    bool isValid{false};
    ValidationError error{ValidationError::None};
    std::string errorMessage;
    
    // Suggestion for fixing
    struct Suggestion {
        std::string description;
        Vec2 suggestedPosition;
        GateId suggestedGate{Constants::INVALID_GATE_ID};
        PortIndex suggestedPort{Constants::INVALID_PORT};
    };
    std::vector<Suggestion> suggestions;
};
```

#### Port Information
```cpp
struct PortInfo {
    bool found{false};
    GateId gateId{Constants::INVALID_GATE_ID};
    PortIndex portIndex{Constants::INVALID_PORT};
    bool isInput{true};
    Vec2 worldPosition{0, 0};
    bool isConnected{false};
    WireId connectedWire{Constants::INVALID_WIRE_ID};
    
    // Visual properties
    float radius{0.2f};
    bool isHighlighted{false};
    HighlightState highlightState{HighlightState::None};
};
```

### Authentication and Authorization

**Authentication**: Not applicable (single-user desktop application)

**Access Control**: Component-level access through initialization
```cpp
// Only initialized components can access the API
ErrorCode initialize(Circuit* circuit, InputManager* input, WireRenderer* renderer);
```

### Error Handling and Status Codes

| Error Code | HTTP Equivalent | Description | Recovery Action |
|------------|-----------------|-------------|-----------------|
| SUCCESS | 200 | Operation completed | Continue |
| INVALID_ID | 400 | Invalid gate/wire ID | Validate input |
| POSITION_OCCUPIED | 409 | Position already taken | Find alternative |
| PORT_ALREADY_CONNECTED | 409 | Port has existing connection | Disconnect first |
| CIRCULAR_DEPENDENCY | 400 | Would create loop | Reject connection |
| OUT_OF_BOUNDS | 400 | Position outside grid | Clamp to bounds |
| OUT_OF_MEMORY | 500 | Memory allocation failed | Free resources |

### Rate Limiting and Throttling

**Wire Creation Throttling**:
```cpp
constexpr float WIRE_CREATION_COOLDOWN = 0.016f; // 60 FPS limit
constexpr size_t MAX_WIRES_PER_FRAME = 10;
constexpr size_t MAX_PREVIEW_UPDATES_PER_FRAME = 1;
```

**Performance Budgets**:
- Wire creation: 1ms per frame
- Preview updates: 0.5ms per frame  
- Port highlighting: 0.3ms per frame
- Hit detection: 2ms per frame

## Database Design

### Memory-Optimized Data Structures

The system uses Structure-of-Arrays (SoA) layout for maximum cache efficiency and SIMD compatibility.

#### Wire Data Layout (SoA)

```cpp
struct alignas(64) WireSystemData {
    // Basic wire information (hot data)
    alignas(64) WireId* wireIds;                    // 4 bytes * count
    alignas(64) GateId* fromGateIds;               // 4 bytes * count  
    alignas(64) GateId* toGateIds;                 // 4 bytes * count
    alignas(64) PortIndex* fromPorts;              // 1 byte * count
    alignas(64) PortIndex* toPorts;                // 1 byte * count
    alignas(64) SignalState* signalStates;         // 1 byte * count
    alignas(64) uint8_t* statusFlags;              // 1 byte * count
    
    // Path data (cold data - separate allocation)
    std::vector<WirePathData> pathData;
    
    // Spatial indexing
    GridIndex<WireId> spatialIndex;
    
    // Memory management
    size_t capacity{0};
    size_t count{0};
    
    static constexpr size_t CACHE_LINE_SIZE = 64;
    static constexpr size_t DEFAULT_CAPACITY = 50000;
};
```

#### Wire Path Data Structure

```cpp
struct WirePathData {
    WireId wireId{Constants::INVALID_WIRE_ID};
    uint16_t pointCount{0};
    uint16_t pathType{0}; // 0=straight, 1=L-shaped, 2=custom
    
    // Path points (allocated separately for cache efficiency)
    std::unique_ptr<Vec2[]> points;
    
    // Bounding box for spatial queries
    struct {
        float minX, minY, maxX, maxY;
    } bounds;
    
    // Path-specific data
    float totalLength{0.0f};
    uint32_t segmentCount{0};
};
```

#### Port Highlight Data Structure

```cpp
enum class HighlightState : uint8_t {
    None = 0,
    Compatible = 1,
    Incompatible = 2,
    Active = 3,
    Preview = 4
};

struct alignas(32) PortHighlightData {
    struct PortHighlight {
        GateId gateId{Constants::INVALID_GATE_ID};
        PortIndex portIndex{Constants::INVALID_PORT};
        HighlightState state{HighlightState::None};
        float intensity{0.0f}; // 0.0 to 1.0 for animation
        uint32_t timestamp{0}; // For fade animations
    };
    
    std::array<PortHighlight, 256> highlights; // Max simultaneous highlights
    size_t activeCount{0};
    uint32_t frameNumber{0};
};
```

### Spatial Indexing Schema

#### Grid-Based Spatial Index

```cpp
template<typename T>
class GridIndex {
private:
    struct Cell {
        std::vector<T> objects;
        uint32_t lastUpdate{0};
        bool isDirty{false};
    };
    
    std::vector<Cell> cells;
    int32_t gridWidth;
    int32_t gridHeight;
    float cellSize;
    Vec2 worldOffset;
    
public:
    void insert(T object, const Rect& bounds) noexcept;
    void remove(T object, const Rect& bounds) noexcept;
    void update(T object, const Rect& oldBounds, const Rect& newBounds) noexcept;
    
    [[nodiscard]] std::vector<T> query(const Rect& bounds) const noexcept;
    [[nodiscard]] std::vector<T> queryPoint(Vec2 point, float radius) const noexcept;
    
    void clear() noexcept;
    void optimize() noexcept; // Defragment and rebalance
};
```

### Migration Strategy

```cpp
// Version 1.0: Basic wire storage
struct WireV1 {
    WireId id;
    GateId fromGate, toGate;
    PortIndex fromPort, toPort;
};

// Version 1.1: Add signal state
struct WireV1_1 {
    WireId id;
    GateId fromGate, toGate;
    PortIndex fromPort, toPort;
    SignalState signalState; // NEW
};

// Migration function
ErrorCode migrateWireData(uint32_t fromVersion, uint32_t toVersion) noexcept;
```

## Class/Module Design

### UML Class Diagram (Text Format)

```
┌─────────────────────────────────┐
│        WireConnectionManager    │
├─────────────────────────────────┤
│ - m_circuit: Circuit*           │
│ - m_input: InputManager*        │
│ - m_renderer: WireRenderer*     │
│ - m_wireData: WireSystemData    │
│ - m_previewSystem: WirePreview  │
│ - m_validator: ConnectionValid. │
│ - m_spatialIndex: GridIndex     │
├─────────────────────────────────┤
│ + initialize(): ErrorCode       │
│ + createWire(): Result<WireId>  │
│ + deleteWire(): ErrorCode       │
│ + startPreview(): void          │
│ + updatePreview(): void         │
│ + validateConnection(): bool    │
└─────────────────────────────────┘
                 │
                 │ uses
                 ▼
┌─────────────────────────────────┐
│       WirePreviewSystem         │
├─────────────────────────────────┤
│ - m_isActive: bool              │
│ - m_startGate: GateId           │
│ - m_startPort: PortIndex        │
│ - m_currentPos: Vec2            │
│ - m_previewPath: Vec2[]         │
├─────────────────────────────────┤
│ + start(): void                 │
│ + update(): void                │
│ + end(): Result<WireId>         │
│ + cancel(): void                │
│ + render(): void                │
└─────────────────────────────────┘
                 │
                 │ collaborates
                 ▼
┌─────────────────────────────────┐
│     PortHighlightSystem         │
├─────────────────────────────────┤
│ - m_highlights: PortHighlight[] │
│ - m_activeCount: size_t         │
│ - m_animationTime: float        │
├─────────────────────────────────┤
│ + highlightPorts(): void        │
│ + clearHighlights(): void       │
│ + updateAnimation(): void       │
│ + render(): void                │
└─────────────────────────────────┘
```

### Interface Definitions

```cpp
// Core interface for wire connection operations
class IWireConnectionSystem {
public:
    virtual ~IWireConnectionSystem() = default;
    
    virtual ErrorCode initialize() noexcept = 0;
    virtual void shutdown() noexcept = 0;
    virtual void update(float deltaTime) noexcept = 0;
    
    virtual Result<WireId> createWire(const WireCreateRequest& request) noexcept = 0;
    virtual ErrorCode deleteWire(WireId wireId) noexcept = 0;
    
    virtual bool canConnect(GateId from, PortIndex fromPort,
                           GateId to, PortIndex toPort) const noexcept = 0;
};

// Interface for wire validation
class IConnectionValidator {
public:
    virtual ~IConnectionValidator() = default;
    
    virtual ValidationResult validate(const WireCreateRequest& request) const noexcept = 0;
    virtual bool hasCircularDependency(GateId from, GateId to) const noexcept = 0;
    virtual bool isPortAvailable(GateId gate, PortIndex port) const noexcept = 0;
};

// Interface for spatial queries
class ISpatialIndex {
public:
    virtual ~ISpatialIndex() = default;
    
    virtual void insert(WireId wireId, const Rect& bounds) noexcept = 0;
    virtual void remove(WireId wireId) noexcept = 0;
    virtual std::vector<WireId> query(const Rect& area) const noexcept = 0;
    virtual WireId queryNearest(Vec2 point, float maxDistance) const noexcept = 0;
};
```

### Design Patterns Employed

1. **Strategy Pattern**: Multiple wire path calculation algorithms
```cpp
class IWirePathStrategy {
public:
    virtual std::vector<Vec2> calculatePath(Vec2 start, Vec2 end) const = 0;
};

class StraightPathStrategy : public IWirePathStrategy { /* ... */ };
class LShapedPathStrategy : public IWirePathStrategy { /* ... */ };  
class BezierPathStrategy : public IWirePathStrategy { /* ... */ };
```

2. **Observer Pattern**: Event notifications for wire state changes
```cpp
class IWireObserver {
public:
    virtual void onWireCreated(WireId wireId) = 0;
    virtual void onWireDeleted(WireId wireId) = 0;
    virtual void onWireSignalChanged(WireId wireId, SignalState newState) = 0;
};
```

3. **Command Pattern**: Undoable wire operations
```cpp
class IWireCommand {
public:
    virtual ErrorCode execute() = 0;
    virtual ErrorCode undo() = 0;
    virtual bool canUndo() const = 0;
};

class CreateWireCommand : public IWireCommand { /* ... */ };
class DeleteWireCommand : public IWireCommand { /* ... */ };
```

4. **Factory Pattern**: Wire creation with different path types
```cpp
class WireFactory {
public:
    static Result<WireId> createStraightWire(const WireCreateRequest& request);
    static Result<WireId> createLShapedWire(const WireCreateRequest& request);
    static Result<WireId> createCustomPathWire(const WireCreateRequest& request);
};
```

### Dependency Injection Strategy

```cpp
// Dependency container for wire system
struct WireSystemDependencies {
    Circuit* circuit;
    InputManager* inputManager;
    WireRenderer* wireRenderer;
    IConnectionValidator* validator;
    ISpatialIndex* spatialIndex;
    IWirePathStrategy* pathStrategy;
    
    // Factory function
    static std::unique_ptr<WireConnectionManager> create(
        const WireSystemDependencies& deps);
};

// Usage
auto deps = WireSystemDependencies{
    .circuit = &gameCircuit,
    .inputManager = &gameInput,
    .wireRenderer = &gameRenderer,
    .validator = std::make_unique<DefaultConnectionValidator>(),
    .spatialIndex = std::make_unique<GridSpatialIndex>(),
    .pathStrategy = std::make_unique<LShapedPathStrategy>()
};

auto wireSystem = WireSystemDependencies::create(deps);
```

## Security Specifications

### Input Validation and Sanitization

**Coordinate Validation**:
```cpp
constexpr float MIN_WORLD_COORD = -10000.0f;
constexpr float MAX_WORLD_COORD = 10000.0f;

bool validateWorldPosition(Vec2 pos) noexcept {
    return pos.x >= MIN_WORLD_COORD && pos.x <= MAX_WORLD_COORD &&
           pos.y >= MIN_WORLD_COORD && pos.y <= MAX_WORLD_COORD &&
           !std::isnan(pos.x) && !std::isnan(pos.y) &&
           std::isfinite(pos.x) && std::isfinite(pos.y);
}
```

**ID Validation**:
```cpp
bool validateGateId(GateId id) noexcept {
    return id != Constants::INVALID_GATE_ID && id <= MAX_GATE_ID;
}

bool validatePortIndex(PortIndex port) noexcept {
    return port >= 0 && port < Constants::MAX_INPUT_PORTS ||
           port == Constants::OUTPUT_PORT;
}
```

**Memory Safety**:
```cpp
// Bounds checking for all array accesses
template<typename T, size_t N>
class SafeArray {
private:
    std::array<T, N> data;
    
public:
    T& at(size_t index) {
        if (index >= N) {
            throw std::out_of_range("SafeArray index out of bounds");
        }
        return data[index];
    }
    
    const T& at(size_t index) const {
        if (index >= N) {
            throw std::out_of_range("SafeArray index out of bounds");
        }
        return data[index];
    }
};
```

### Resource Protection

**Memory Pool Bounds**:
```cpp
class BoundedMemoryPool {
private:
    static constexpr size_t MAX_WIRE_COUNT = 1000000;
    static constexpr size_t MAX_MEMORY_USAGE = 500 * 1024 * 1024; // 500MB
    
    size_t currentWireCount{0};
    size_t currentMemoryUsage{0};
    
public:
    Result<WireId> allocateWire() noexcept {
        if (currentWireCount >= MAX_WIRE_COUNT) {
            return {Constants::INVALID_WIRE_ID, ErrorCode::OUT_OF_MEMORY};
        }
        
        size_t requiredMemory = sizeof(Wire) + sizeof(WirePathData);
        if (currentMemoryUsage + requiredMemory > MAX_MEMORY_USAGE) {
            return {Constants::INVALID_WIRE_ID, ErrorCode::OUT_OF_MEMORY};
        }
        
        // Allocate wire...
    }
};
```

## Performance Requirements

### Response Time Targets

| Operation | Target Time | Maximum Time | Measurement Method |
|-----------|-------------|--------------|-------------------|
| Wire Creation | <1ms | 5ms | High-resolution timer |
| Wire Deletion | <0.5ms | 2ms | High-resolution timer |
| Hit Detection | <0.1ms | 1ms | Per-query timer |
| Preview Update | <0.2ms | 1ms | Per-frame timer |
| Port Highlighting | <0.5ms | 2ms | Per-frame timer |
| Path Calculation | <0.3ms | 1ms | Per-wire timer |

### Throughput Expectations

**Wire Operations per Second**:
- Creation: 1000 wires/sec (burst), 100 wires/sec (sustained)
- Deletion: 2000 wires/sec  
- Queries: 10000 queries/sec
- Updates: 100000 updates/sec

**Memory Throughput**:
- Wire data access: 1GB/s (sequential)
- Spatial queries: 100MB/s (random)
- Rendering data: 50MB/s

### Scalability Considerations

**Horizontal Scaling**:
```cpp
// Multi-threaded wire processing
class ThreadedWireProcessor {
private:
    ThreadPool m_threadPool;
    std::vector<WireChunk> m_wireChunks;
    
public:
    void processWiresParallel(float deltaTime) {
        auto futures = std::vector<std::future<void>>();
        
        for (auto& chunk : m_wireChunks) {
            futures.push_back(
                m_threadPool.enqueue([&chunk, deltaTime]() {
                    chunk.processSignals(deltaTime);
                })
            );
        }
        
        // Wait for all chunks to complete
        for (auto& future : futures) {
            future.wait();
        }
    }
};
```

**Vertical Scaling**:
```cpp
// SIMD optimization for signal processing
void processWireSignals_AVX2(WireSystemData* data, size_t count) noexcept {
    const __m256i* signalStates = (__m256i*)data->signalStates;
    __m256i* outputStates = (__m256i*)data->outputBuffer;
    
    size_t simdCount = count / 32; // 32 signals per 256-bit vector
    
    for (size_t i = 0; i < simdCount; ++i) {
        __m256i signals = _mm256_load_si256(&signalStates[i]);
        // Process 32 signals simultaneously
        __m256i processed = processSignalVector(signals);
        _mm256_store_si256(&outputStates[i], processed);
    }
    
    // Handle remaining signals
    size_t remaining = count % 32;
    if (remaining > 0) {
        processWireSignalsScalar(data->signalStates + count - remaining, remaining);
    }
}
```

### Caching Strategies

**Wire Data Caching**:
```cpp
class WireDataCache {
private:
    struct CacheEntry {
        WireId wireId;
        WirePathData pathData;
        uint32_t lastAccess;
        bool isDirty;
    };
    
    LRUCache<WireId, CacheEntry> m_pathCache{1000}; // 1000 most recent paths
    std::unordered_map<WireId, SignalState> m_signalCache;
    
public:
    const WirePathData* getWirePath(WireId wireId) {
        auto entry = m_pathCache.get(wireId);
        if (entry) {
            entry->lastAccess = getCurrentFrame();
            return &entry->pathData;
        }
        
        // Load from main storage
        return loadWirePathSlow(wireId);
    }
};
```

**Spatial Query Caching**:
```cpp
class SpatialQueryCache {
private:
    struct QueryKey {
        Rect bounds;
        uint32_t frameNumber;
        
        bool operator==(const QueryKey& other) const {
            return bounds == other.bounds && frameNumber == other.frameNumber;
        }
    };
    
    std::unordered_map<QueryKey, std::vector<WireId>> m_queryCache;
    
public:
    std::vector<WireId> getCachedQuery(const Rect& bounds, uint32_t frame) {
        QueryKey key{bounds, frame};
        auto it = m_queryCache.find(key);
        if (it != m_queryCache.end()) {
            return it->second;
        }
        
        // Perform actual query and cache result
        auto result = performSpatialQuery(bounds);
        m_queryCache[key] = result;
        return result;
    }
};
```

## Implementation Guidelines

### Coding Standards and Conventions

**Naming Conventions**:
```cpp
// Classes: PascalCase
class WireConnectionManager { };

// Member variables: m_ prefix + camelCase
class Example {
private:
    int m_memberVariable;
    static constexpr int s_staticMember = 42;
};

// Functions: camelCase
void calculateWirePath() noexcept;

// Constants: UPPER_SNAKE_CASE
constexpr float MAX_WIRE_LENGTH = 1000.0f;

// Enums: PascalCase with strong typing
enum class WireState : uint8_t {
    Disconnected,
    Connected,
    Invalid
};
```

**Error Handling Patterns**:
```cpp
// Use Result<T> for fallible operations
[[nodiscard]] Result<WireId> createWire(const WireCreateRequest& request) noexcept {
    // Input validation
    if (!validateRequest(request)) {
        return {Constants::INVALID_WIRE_ID, ErrorCode::INVALID_POSITION};
    }
    
    // Resource allocation
    auto wireId = allocateWireId();
    if (wireId == Constants::INVALID_WIRE_ID) {
        return {Constants::INVALID_WIRE_ID, ErrorCode::OUT_OF_MEMORY};
    }
    
    // Success case
    return {wireId, ErrorCode::SUCCESS};
}

// Use ErrorCode for operations that don't return values
[[nodiscard]] ErrorCode deleteWire(WireId wireId) noexcept {
    if (!isValidWireId(wireId)) {
        return ErrorCode::INVALID_ID;
    }
    
    removeWireFromSpatialIndex(wireId);
    deallocateWire(wireId);
    
    return ErrorCode::SUCCESS;
}
```

**RAII Resource Management**:
```cpp
class WireResource {
private:
    WireId m_wireId{Constants::INVALID_WIRE_ID};
    WireSystemData* m_wireSystem{nullptr};
    
public:
    WireResource(WireSystemData* system) : m_wireSystem(system) {
        m_wireId = m_wireSystem->allocateWire();
    }
    
    ~WireResource() {
        if (m_wireId != Constants::INVALID_WIRE_ID && m_wireSystem) {
            m_wireSystem->deallocateWire(m_wireId);
        }
    }
    
    // Move-only semantics
    WireResource(const WireResource&) = delete;
    WireResource& operator=(const WireResource&) = delete;
    
    WireResource(WireResource&& other) noexcept 
        : m_wireId(other.m_wireId), m_wireSystem(other.m_wireSystem) {
        other.m_wireId = Constants::INVALID_WIRE_ID;
        other.m_wireSystem = nullptr;
    }
    
    WireId release() noexcept {
        auto id = m_wireId;
        m_wireId = Constants::INVALID_WIRE_ID;
        return id;
    }
};
```

### Logging and Monitoring Requirements

**Structured Logging**:
```cpp
enum class LogLevel : uint8_t {
    Trace = 0,
    Debug = 1, 
    Info = 2,
    Warning = 3,
    Error = 4,
    Fatal = 5
};

class WireSystemLogger {
private:
    LogLevel m_minLevel{LogLevel::Info};
    
public:
    template<typename... Args>
    void log(LogLevel level, const char* format, Args... args) noexcept {
        if (level < m_minLevel) return;
        
        // Thread-safe logging with minimal allocations
        char buffer[512];
        snprintf(buffer, sizeof(buffer), format, args...);
        
        writeLogEntry(level, buffer);
    }
    
    void logWireCreation(WireId wireId, GateId from, GateId to) noexcept {
        log(LogLevel::Debug, "Wire created: ID=%u, From=%u, To=%u", 
            wireId, from, to);
    }
    
    void logPerformanceMetric(const char* operation, float timeMs) noexcept {
        if (timeMs > 1.0f) { // Only log slow operations
            log(LogLevel::Warning, "Slow operation: %s took %.2fms", 
                operation, timeMs);
        }
    }
};
```

**Performance Monitoring**:
```cpp
class WireSystemProfiler {
private:
    struct ProfileData {
        uint64_t totalCalls{0};
        uint64_t totalTimeNs{0};
        uint64_t maxTimeNs{0};
        uint64_t minTimeNs{UINT64_MAX};
    };
    
    std::unordered_map<std::string, ProfileData> m_profiles;
    
public:
    class ScopedTimer {
        WireSystemProfiler* m_profiler;
        std::string m_name;
        std::chrono::high_resolution_clock::time_point m_start;
        
    public:
        ScopedTimer(WireSystemProfiler* profiler, std::string name)
            : m_profiler(profiler), m_name(std::move(name))
            , m_start(std::chrono::high_resolution_clock::now()) {}
            
        ~ScopedTimer() {
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - m_start);
            m_profiler->recordTime(m_name, duration.count());
        }
    };
    
    void recordTime(const std::string& name, uint64_t timeNs) {
        auto& data = m_profiles[name];
        data.totalCalls++;
        data.totalTimeNs += timeNs;
        data.maxTimeNs = std::max(data.maxTimeNs, timeNs);
        data.minTimeNs = std::min(data.minTimeNs, timeNs);
    }
    
    void printStats() const {
        for (const auto& [name, data] : m_profiles) {
            double avgMs = (data.totalTimeNs / 1000000.0) / data.totalCalls;
            double maxMs = data.maxTimeNs / 1000000.0;
            double minMs = data.minTimeNs / 1000000.0;
            
            printf("%s: calls=%llu, avg=%.3fms, max=%.3fms, min=%.3fms\n",
                   name.c_str(), data.totalCalls, avgMs, maxMs, minMs);
        }
    }
};

// Usage macro for easy profiling
#define PROFILE_SCOPE(profiler, name) \
    WireSystemProfiler::ScopedTimer timer(profiler, name)
```

### Testing Strategy

**Unit Tests**:
```cpp
// Test wire creation
TEST_F(WireConnectionTest, CreateValidWire) {
    // Arrange
    auto gateA = m_circuit->addGate({0, 0});
    auto gateB = m_circuit->addGate({2, 0});
    
    // Act
    auto result = m_wireSystem->createWire(*gateA, Constants::OUTPUT_PORT,
                                          *gateB, 0);
    
    // Assert
    EXPECT_TRUE(result.success());
    EXPECT_NE(*result, Constants::INVALID_WIRE_ID);
    
    auto wire = m_circuit->getWire(*result);
    ASSERT_NE(wire, nullptr);
    EXPECT_EQ(wire->fromGateId, *gateA);
    EXPECT_EQ(wire->toGateId, *gateB);
}

// Test connection validation
TEST_F(WireConnectionTest, RejectInvalidConnection) {
    // Arrange  
    auto gate = m_circuit->addGate({0, 0});
    
    // Act - try to connect gate to itself
    auto result = m_wireSystem->createWire(*gate, Constants::OUTPUT_PORT,
                                          *gate, 0);
    
    // Assert
    EXPECT_FALSE(result.success());
    EXPECT_EQ(result.error, ErrorCode::INVALID_POSITION);
}

// Performance test
TEST_F(WireConnectionTest, CreateManyWiresPerformance) {
    const size_t WIRE_COUNT = 10000;
    
    // Arrange - create a grid of gates
    std::vector<GateId> gates;
    for (int y = 0; y < 100; ++y) {
        for (int x = 0; x < 100; ++x) {
            auto gate = m_circuit->addGate({x, y});
            gates.push_back(*gate);
        }
    }
    
    // Act - measure wire creation time
    auto start = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < WIRE_COUNT; ++i) {
        GateId from = gates[i];
        GateId to = gates[(i + 1) % gates.size()];
        m_wireSystem->createWire(from, Constants::OUTPUT_PORT, to, 0);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Assert - should complete within performance budget
    EXPECT_LT(duration.count(), 100); // Less than 100ms for 10k wires
    
    double wiresPerSecond = WIRE_COUNT / (duration.count() / 1000.0);
    EXPECT_GT(wiresPerSecond, 1000); // At least 1000 wires/sec
}
```

**Integration Tests**:
```cpp
TEST_F(WireIntegrationTest, WireCreationWithInputManager) {
    // Arrange
    auto gateA = m_circuit->addGate({0, 0});
    auto gateB = m_circuit->addGate({2, 0});
    
    // Simulate mouse drag from gate A output to gate B input
    Vec2 startPos = m_circuit->getGate(*gateA)->getOutputPortPosition();
    Vec2 endPos = m_circuit->getGate(*gateB)->getInputPortPosition(0);
    
    // Act
    simulateMouseDown(startPos);
    simulateDrag(startPos, endPos);
    simulateMouseUp(endPos);
    
    // Process input events
    m_inputManager->update(0.016f);
    m_wireSystem->update(0.016f);
    
    // Assert
    EXPECT_EQ(m_circuit->getWireCount(), 1);
    
    auto wire = findWireBetweenGates(*gateA, *gateB);
    EXPECT_NE(wire, nullptr);
    EXPECT_TRUE(wire->isValid());
}
```

**End-to-End Tests**:
```cpp
TEST_F(WireE2ETest, CompleteUserWorkflow) {
    // Test complete user workflow: create gates, connect with wires, simulate
    
    // 1. Create NOT gate circuit: A -> NOT -> B -> NOT -> C
    auto inputGate = createInputGate({0, 0}, SignalState::HIGH);
    auto notGate1 = m_circuit->addGate({2, 0});
    auto notGate2 = m_circuit->addGate({4, 0});
    auto outputGate = createOutputGate({6, 0});
    
    // 2. Connect with wires through UI simulation
    connectGatesViaUI(*inputGate, Constants::OUTPUT_PORT, *notGate1, 0);
    connectGatesViaUI(*notGate1, Constants::OUTPUT_PORT, *notGate2, 0);
    connectGatesViaUI(*notGate2, Constants::OUTPUT_PORT, *outputGate, 0);
    
    // 3. Run simulation
    m_circuit->resume();
    for (int i = 0; i < 10; ++i) {
        m_circuit->update(0.1f); // Wait for signal propagation
    }
    
    // 4. Verify output (HIGH -> NOT -> LOW -> NOT -> HIGH)
    auto output = getOutputGateSignal(*outputGate);
    EXPECT_EQ(output, SignalState::HIGH);
}
```

This comprehensive technical specification provides all the necessary details for implementing the Wire Connection System, including concrete C++ class definitions, memory layouts, performance optimizations, and integration patterns that align with the existing codebase architecture.

Key files referenced in this implementation:
- C:\project\notgame3\src\core\Wire.h
- C:\project\notgame3\src\core\Gate.h  
- C:\project\notgame3\src\core\Circuit.h
- C:\project\notgame3\src\input\InputManager.h
- C:\project\notgame3\src\input\DragManager.h
- C:\project\notgame3\src\core\Types.h