#include "grid.hpp"
#include <stdexcept>
#include <iostream>

Grid::Grid(float _width, float _height, uint32_t _xNum, uint32_t _yNum):
	width(_width), height(_height),
	xNum(_xNum), yNum(_yNum),
	
	xCellSize(_width / _xNum), yCellSize(_height / _yNum),

	invXCellSize(_xNum / _width), invYCellSize(_yNum / _height),

	cells(_xNum * _yNum, INVALID_REF),
	
	rowElements(_yNum)
{};

uint32_t Grid::addElement(Element element) {
	return elements.insert(element);
};

void Grid::insert(uint32_t eltId) {
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

    // insert in necessary cells
    for (int row = minRow; row <= maxRow; row++) { // rows
        for (int col = minCol; col <= maxCol; col++) { // columns
            // check if there are already elements in that cell
            if (cells[row * xNum + col] == INVALID_REF) {
                // no elements, add the current one to the references in this cell
                uint32_t eltRefId = rowElements[row].insert({eltId, INVALID_REF});

                // set the cell to index the element reference
                cells[row * xNum + col] = eltRefId;
            } else {
                // get first reference id
				uint32_t cellRefId = cells[row * xNum + col];

				// insert the element reference into the cell list
				uint32_t eltRefId = rowElements[row].insert({eltId, cellRefId});

				// change the cell to index the current elt reference
				cells[row * xNum + col] = eltRefId;
            }
        }
    }
};

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

void Grid::eraseElement(uint32_t eltId) {
	elements.erase(eltId);
};

void Grid::cleanup() {
	// de-allocate empty rows
	for (int i = 0; i < yNum; i++) {
		if (rowElements[i].length == 0 && !rowElements[i].cleared) rowElements[i].clear();
	}
};

void Grid::clear() {
	std::fill(cells.begin(), cells.end(), INVALID_REF);
	for (int i = 0; i < yNum; i++) rowElements[i].clear();
};
