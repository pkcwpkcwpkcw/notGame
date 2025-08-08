#include <iostream>
#include <memory>
#include "../core/GatePool.h"
#include "../core/GridMap.h"
#include "../game/PlacementManager.h"
#include "../game/SelectionManager.h"
#include "../core/Circuit.h"
#include "../core/Grid.h"

int main() {
    std::cout << "Testing Gate Placement System" << std::endl;
    
    // Create components
    auto circuit = std::make_unique<Circuit>();
    auto gridMap = std::make_unique<GridMap>();
    auto grid = std::make_unique<Grid>();
    auto placementManager = std::make_unique<PlacementManager>();
    auto selectionManager = std::make_unique<SelectionManager>();
    
    // Initialize managers
    placementManager->initialize(circuit.get(), gridMap.get(), grid.get());
    selectionManager->initialize(circuit.get(), gridMap.get(), grid.get());
    
    // Test placement
    placementManager->enterPlacementMode(GateType::NOT);
    
    Vec2i testPos(5, 5);
    if (placementManager->validatePosition(testPos)) {
        auto result = placementManager->placeGate(testPos);
        if (result.success()) {
            std::cout << "Gate placed successfully at (5,5) with ID: " 
                     << static_cast<uint32_t>(result.value) << std::endl;
            
            // Test selection
            GateId gateId = selectionManager->getGateAt(testPos);
            if (gateId != Constants::INVALID_GATE_ID) {
                selectionManager->selectGate(gateId);
                std::cout << "Gate selected successfully" << std::endl;
                
                if (selectionManager->isSelected(gateId)) {
                    std::cout << "Selection verified" << std::endl;
                }
            }
        } else {
            std::cout << "Failed to place gate" << std::endl;
        }
    } else {
        std::cout << "Invalid position for gate placement" << std::endl;
    }
    
    // Test multiple placements
    placementManager->setContinuousPlacement(true);
    for (int i = 0; i < 5; ++i) {
        Vec2i pos(i * 2, 0);
        auto result = placementManager->placeGate(pos);
        if (result.success()) {
            std::cout << "Gate " << i << " placed at (" << pos.x << "," << pos.y << ")" << std::endl;
        }
    }
    
    std::cout << "Total gates in circuit: " << circuit->getGateCount() << std::endl;
    std::cout << "Selected gates: " << selectionManager->getSelectionCount() << std::endl;
    
    // Test deletion
    if (selectionManager->hasSelection()) {
        selectionManager->deleteSelected();
        std::cout << "Selected gates deleted" << std::endl;
        std::cout << "Remaining gates: " << circuit->getGateCount() << std::endl;
    }
    
    std::cout << "Gate Placement System Test Complete!" << std::endl;
    
    return 0;
}