#include "grid.hpp"
#include <stdexcept>
#include <iostream>
#include <algorithm>

Grid::Grid(float _width, float _height, uint32_t _xNum, uint32_t _yNum, uint32_t _nBalls):
    width(_width), height(_height),
    xNum(_xNum), yNum(_yNum),
    xCellSize(_width / _xNum), yCellSize(_height / _yNum),
    invXCellSize(_xNum / _width), invYCellSize(_yNum / _height),
    nBalls(_nBalls)
{
    numCells = xNum * yNum;
    
    // Pre-allocate all memory strictly once
    cellStart.resize(numCells);
    indices.resize(nBalls);
    indicesTemp.resize(nBalls);
    elements.resize(nBalls);
}

void Grid::build() 
{
    // 1. Populate the proxy array (O(N))
    for (uint32_t i = 0; i < nBalls; i++) {
        const Element& el = elements[i];

        int col = std::max(0, std::min(static_cast<int>(xNum - 1), static_cast<int>(el.cx * invXCellSize)));
        int row = std::max(0, std::min(static_cast<int>(yNum - 1), static_cast<int>(el.cy * invYCellSize)));
        
        indices[i] = { static_cast<uint32_t>(row * xNum + col), i };
    }

    // 2. 8-bit LSD Radix Sort (O(N) - 4 passes)
    for (int pass = 0; pass < 4; pass++) {
        uint32_t counts[256] = {0};
        uint32_t shift = pass * 8;

        for (uint32_t i = 0; i < nBalls; i++) {
            uint8_t byte = (indices[i].cellId >> shift) & 0xFF;
            counts[byte]++;
        }

        uint32_t offsets[256];
        offsets[0] = 0;
        for (int i = 1; i < 256; i++) {
            offsets[i] = offsets[i - 1] + counts[i - 1];
        }

        for (uint32_t i = 0; i < nBalls; i++) {
            uint8_t byte = (indices[i].cellId >> shift) & 0xFF;
            indicesTemp[offsets[byte]++] = indices[i];
        }

        indices.swap(indicesTemp);
    }

    // 3. Build Cell Start Offsets (O(N))
    std::fill(cellStart.begin(), cellStart.end(), nBalls); 
    
    for (uint32_t i = 0; i < nBalls; i++) {
        uint32_t cell = indices[i].cellId;
        if (i == 0 || cell != indices[i - 1].cellId) {
            cellStart[cell] = i;
        }
    }
}