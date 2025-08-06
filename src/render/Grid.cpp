#include "render/Grid.h"

namespace notgame {
namespace render {

Grid::Grid(int width, int height)
    : width_(width)
    , height_(height)
    , gridLineWidth_(1.0f)
    , showGrid_(true)
    , gridColorR_(0.3f)
    , gridColorG_(0.3f)
    , gridColorB_(0.3f)
    , gridColorA_(1.0f) {
}

Grid::~Grid() {
}

void Grid::setGridColor(float r, float g, float b, float a) {
    gridColorR_ = r;
    gridColorG_ = g;
    gridColorB_ = b;
    gridColorA_ = a;
}

void Grid::getGridColor(float& r, float& g, float& b, float& a) const {
    r = gridColorR_;
    g = gridColorG_;
    b = gridColorB_;
    a = gridColorA_;
}

} // namespace render
} // namespace notgame