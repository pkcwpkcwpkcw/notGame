#include "GatePaletteUI.h"
#include "../game/PlacementManager.h"
#include "../game/SelectionManager.h"
#include <imgui.h>
#include <SDL.h>
#include <cstring>

void GatePaletteUI::render() noexcept {
    if (!isVisible) {
        return;
    }
    
    // Removed repetitive logging
    
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoFocusOnAppearing;
    
    if (isDocked) {
        ImGui::SetNextWindowPos(ImVec2(0, 20), ImGuiCond_Once);
        ImGui::SetNextWindowSize(ImVec2(paletteWidth, ImGui::GetIO().DisplaySize.y - 20), ImGuiCond_Always);
        windowFlags |= ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;
    } else {
        ImGui::SetNextWindowSize(ImVec2(paletteWidth, 400), ImGuiCond_FirstUseEver);
    }
    
    if (ImGui::Begin("Gate Palette", &isVisible, windowFlags)) {
        renderGatePalette();
        
        ImGui::Separator();
        renderSelectionInfo();
        
        ImGui::Separator();
        renderPlacementMode();
        
        ImGui::Separator();
        renderShortcutHints();
    }
    ImGui::End();
}

void GatePaletteUI::renderGatePalette() noexcept {
    ImGui::Text("Gates");
    ImGui::Spacing();
    
    ImVec2 buttonSize(64, 64);
    
    renderGateButton(GateType::NOT, "NOT", "Inverts the input signal (0→1, 1→0)\nShortcut: N");
    
    ImGui::Spacing();
    
    if (selectionManager && selectionManager->hasSelection()) {
        if (ImGui::Button("Delete Selected", ImVec2(-1, 30))) {
            if (onDeleteSelected) {
                onDeleteSelected();
            } else if (selectionManager) {
                selectionManager->deleteSelected();
            }
        }
        
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Delete selected gates\nShortcut: Delete");
        }
    }
}

void GatePaletteUI::renderGateButton(GateType type, const char* label, const char* tooltip) noexcept {
    ImVec2 buttonSize(64, 64);
    
    bool isActive = false;
    if (placementManager) {
        isActive = placementManager->isInPlacementMode() && 
                  placementManager->getSelectedGateType() == type;
    }
    
    if (isActive) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.8f, 0.3f, 1.0f));
    }
    
    if (ImGui::Button(label, buttonSize)) {
        SDL_Log("[GatePaletteUI] Gate button clicked - type: %d, isActive: %d", (int)type, isActive);
        if (onGateSelected) {
            onGateSelected(type);
        } else if (placementManager) {
            if (isActive) {
                SDL_Log("[GatePaletteUI] Exiting placement mode");
                placementManager->exitPlacementMode();
            } else {
                SDL_Log("[GatePaletteUI] Entering placement mode for gate type: %d", (int)type);
                placementManager->enterPlacementMode(type);
            }
        } else {
            SDL_Log("[GatePaletteUI] ERROR: PlacementManager is null!");
        }
    }
    
    if (isActive) {
        ImGui::PopStyleColor(2);
    }
    
    if (ImGui::IsItemHovered() && showTooltips && tooltip) {
        ImGui::SetTooltip("%s", tooltip);
    }
}

void GatePaletteUI::renderSelectionInfo() noexcept {
    ImGui::Text("Selection");
    ImGui::Spacing();
    
    if (selectionManager) {
        size_t selCount = selectionManager->getSelectionCount();
        if (selCount > 0) {
            ImGui::Text("Selected: %zu gate%s", selCount, selCount > 1 ? "s" : "");
            
            GateId lastSelected = selectionManager->getLastSelected();
            if (lastSelected != Constants::INVALID_GATE_ID) {
                ImGui::Text("Last ID: %u", static_cast<uint32_t>(lastSelected));
            }
        } else {
            ImGui::TextDisabled("No selection");
        }
    } else {
        ImGui::TextDisabled("Selection not available");
    }
}

void GatePaletteUI::renderPlacementMode() noexcept {
    ImGui::Text("Placement Mode");
    ImGui::Spacing();
    
    if (placementManager) {
        if (placementManager->isInPlacementMode()) {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "ACTIVE");
            ImGui::Text("Type: %s", getGateTypeString(placementManager->getSelectedGateType()).c_str());
            
            if (placementManager->isPreviewPositionValid()) {
                Vec2i pos = placementManager->getPreviewPosition();
                ImGui::Text("Position: (%d, %d)", pos.x, pos.y);
            } else {
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Invalid position");
            }
            
            bool continuous = placementManager->isContinuousPlacement();
            if (ImGui::Checkbox("Continuous", &continuous)) {
                placementManager->setContinuousPlacement(continuous);
            }
            
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Hold Shift for continuous placement");
            }
            
            if (ImGui::Button("Cancel (ESC)", ImVec2(-1, 0))) {
                placementManager->exitPlacementMode();
            }
        } else {
            ImGui::TextDisabled("Inactive");
            ImGui::TextDisabled("Select a gate to place");
        }
    } else {
        ImGui::TextDisabled("Placement not available");
    }
}

void GatePaletteUI::renderShortcutHints() noexcept {
    ImGui::Text("Shortcuts");
    ImGui::Spacing();
    
    ImGui::TextDisabled("N - NOT Gate");
    ImGui::TextDisabled("Delete - Delete Selected");
    ImGui::TextDisabled("ESC - Cancel Placement");
    ImGui::TextDisabled("Shift - Continuous Place");
    ImGui::TextDisabled("Ctrl+Click - Multi-select");
    ImGui::TextDisabled("D - Delete Mode");
}

std::string GatePaletteUI::getGateTypeString(GateType type) const noexcept {
    switch (type) {
        case GateType::NOT: return "NOT";
        default: return "Unknown";
    }
}