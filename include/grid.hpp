#pragma once
#include <vector>
#include <cstdint>
#include <SFML/Graphics.hpp>

struct ElementIndex {
    uint32_t cellId;
    uint32_t ballId;
};

struct Element {
    float radius, mass, rest;
    float vx, vy;
    uint32_t color;
    float cx, cy;
};

constexpr uint32_t INVALID_REF = 0xFFFFFFFF;

class Grid {
private:
    float xCellSize, yCellSize;
    float invXCellSize, invYCellSize;
    float width, height;
    
public:
    uint32_t xNum, yNum, numCells;
    uint32_t nBalls;

    std::vector<uint32_t> cellStart;
    std::vector<ElementIndex> indices;
    std::vector<ElementIndex> indicesTemp;
    std::vector<Element> elements;

    Grid() = default;
    
    Grid(float _width, float _height, uint32_t _xNum, uint32_t _yNum, uint32_t _nBalls);

    void build();
};