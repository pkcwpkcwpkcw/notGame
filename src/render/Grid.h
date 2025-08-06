#pragma once

namespace notgame {
namespace render {

class Grid {
public:
    Grid(int width, int height);
    ~Grid();
    
    // Grid properties
    int getWidth() const { return width_; }
    int getHeight() const { return height_; }
    
    // Grid rendering properties
    void setGridLineWidth(float width) { gridLineWidth_ = width; }
    float getGridLineWidth() const { return gridLineWidth_; }
    
    void setShowGrid(bool show) { showGrid_ = show; }
    bool isGridVisible() const { return showGrid_; }
    
    // Grid colors
    void setGridColor(float r, float g, float b, float a = 1.0f);
    void getGridColor(float& r, float& g, float& b, float& a) const;
    
private:
    int width_;
    int height_;
    float gridLineWidth_;
    bool showGrid_;
    
    float gridColorR_;
    float gridColorG_;
    float gridColorB_;
    float gridColorA_;
};

} // namespace render
} // namespace notgame