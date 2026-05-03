#include "grid.hpp"
#include <stdexcept>
#include <iostream>


uint32_t Grid::addElement(Element element) {
	return elements.insert(element);
};

void Grid::insert(uint32_t ballId) {
    Element& element = elements[ballId];
    
    int col = std::max(0, std::min(static_cast<int>(xNum - 1), static_cast<int>(element.cx * invXCellSize)));
    int row = std::max(0, std::min(static_cast<int>(yNum - 1), static_cast<int>(element.cy * invYCellSize)));
    
    uint32_t cellIndex = row * xNum + col;

    // Push to the front of the implicit linked list
    cellStart[ballId] = cellStart[cellIndex];
    cellStart[cellIndex] = ballId;
};

/*
void Grid::remove(uint32_t eltId) {
	// index of element in elements list
    Element& element = elements[eltId];

	float r = element.radius;
	float left   = element.cx - r;
    float right  = element.cx + r;
    float top    = element.cy - r;
    float bottom = element.cy + r;
    
	int minRow = std::max(0, static_cast<int>(top * invYCellSize));
	int maxRow = std::min(static_cast<int>(yNum - 1), static_cast<int>(bottom * invYCellSize));

	int minCol = std::max(0, static_cast<int>(left * invXCellSize));
	int maxCol = std::min(static_cast<int>(xNum - 1), static_cast<int>(right * invXCellSize));

    // remove from necessary cells
    for (int row = minRow; row <= maxRow; row++) { // rows
        for (int col = minCol; col <= maxCol; col++) { // columns
            // check if there are no elements in this cell (commented becaouse the element has to be in it)
            // if (cells[row] == INVALID_REF) continue;

			uint32_t cellRefId = cells[row * xNum + col]; // current element reference
			uint32_t prevRefId = INVALID_REF; // reference to the previous object

			// else iterate until it finds the element in the cell's reference list
			while (true) {
				if (cellRefId == INVALID_REF) throw std::runtime_error("Tried to get a non-existing element in a cell!");
				
				ElementRef& eltRef = rowElements[row][cellRefId];
				
				// check if there isn't a next element (should NOT be possible)
				
				if (eltRef.ref != eltId) {
					// this is not the element. pass to the next one
					prevRefId = cellRefId;
					cellRefId = eltRef.nextInCell;
					continue; // step to next iteration
				}
				
				// --- if this runs, THIS (eltRef) is the element reference we're searching for ---
				
				
				// close gap in element reference links (if not first element)
				if (prevRefId != INVALID_REF) {
					// NOT FIRST
					rowElements[row][prevRefId].nextInCell = eltRef.nextInCell;
				} else {
					// FIRST -> set cell to contain the next element reference
					cells[row * xNum + col] = eltRef.nextInCell;
				}

				rowElements[row].erase(cellRefId);
				break;
			}
        }
    }
	
	// remove element from elements (COMMENTED OUT BECAUSE OF OPTIMISATION TRY)
	// elements.erase(eltId);
}
*/

void Grid::eraseElement(uint32_t eltId) {
	elements.erase(eltId);
};

void Grid::clear() {
	std::fill(cellStart.begin(), cellStart.end(), INVALID_REF);
};

void Grid::build() 
{
    uint32_t activeRange = elements.range(); // Highest index in the FreeList
    
    // 0. Dynamically scale the proxy buffers if new elements were inserted
    if (indices.size() < activeRange) {
        indices.resize(activeRange);
        indicesTemp.resize(activeRange);
    }

    // 1. Populate the proxy array and filter out deleted elements (O(N))
    uint32_t validCount = 0;
    for (uint32_t i = 0; i < activeRange; i++) {
        const Element& el = elements[i];
        
        // Skip elements you have removed from the FreeList.
        // Assuming you set mass to 0 or radius to 0 upon deletion:
        if (el.mass <= 0.00001f) continue; 

        int col = std::max(0, std::min(static_cast<int>(xNum - 1), static_cast<int>(el.cx * invXCellSize)));
        int row = std::max(0, std::min(static_cast<int>(yNum - 1), static_cast<int>(el.cy * invYCellSize)));
        
        // Compacts valid elements into a dense front-loaded array
        indices[validCount++] = { static_cast<uint32_t>(row * xNum + col), i };
    }

    activeParticleCount = validCount;

    // 2. 8-bit LSD Radix Sort (O(N) - 4 passes)
    for (int pass = 0; pass < 4; pass++) {
        uint32_t counts[256] = {0};
        uint32_t shift = pass * 8;

        for (uint32_t i = 0; i < validCount; i++) {
            uint8_t byte = (indices[i].cellId >> shift) & 0xFF;
            counts[byte]++;
        }

        uint32_t offsets[256];
        offsets[0] = 0;
        for (int i = 1; i < 256; i++) {
            offsets[i] = offsets[i - 1] + counts[i - 1];
        }

        for (uint32_t i = 0; i < validCount; i++) {
            uint8_t byte = (indices[i].cellId >> shift) & 0xFF;
            indicesTemp[offsets[byte]++] = indices[i];
        }

        indices.swap(indicesTemp);
    }

    // 3. Build Cell Start Offsets (O(N))
    std::fill(cellStart.begin(), cellStart.end(), validCount); 
    
    for (uint32_t i = 0; i < validCount; i++) {
        uint32_t cell = indices[i].cellId;
        if (i == 0 || cell != indices[i - 1].cellId) {
            cellStart[cell] = i;
        }
    }
}