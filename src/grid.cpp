#include "grid.hpp"
#include <stdexcept>
#include <iostream>

Grid::Grid(float _width, float _height, uint32_t _xNum, uint32_t _yNum):
	width(_width), height(_height),
	xNum(_xNum), yNum(_yNum),
	
	xCellSize(width / xNum), yCellSize(height / yNum),

	invXCellSize(1.f / xCellSize), invYCellSize(1.f / yCellSize),

	cells(xNum * yNum, static_cast<uint32_t>(-1)),
	
	rowElements(yNum)
{};

int Grid::addElement(Element element) {
	int eltId = elements.insert(element);
};

void Grid::insert(int eltId) {
    // index of element in elements list
	Element& element = elements[eltId];

	float r = element.radius;
	float left   = element.cx - r;
    float right  = element.cx + r;
    float top    = element.cy - r;
    float bottom = element.cy + r;
    
    int minRow = static_cast<int>(   top * invYCellSize);
    int maxRow = static_cast<int>(bottom * invYCellSize);

    int minCol = static_cast<int>(  left * invXCellSize);
    int maxCol = static_cast<int>( right * invXCellSize);

    // insert in necessary cells
    for (int row = minRow; row < maxRow; row++) { // rows
        for (int col = minCol; col < maxCol; col++) { // columns
            // check if there are already elements in that cell
            if (cells[row] == -1) {
                // no elements, add the current one to the references in this cell
                int eltRefId = rowElements[row].insert({eltId, -1});

                // set the cell to index the element reference
                cells[row * xNum + col] = eltRefId;
            } else {
                // get first reference id
				int cellRefId = cells[row * xNum + col];

				// insert the element reference into the cell list
				int eltRefId = rowElements[row].insert({eltId, cellRefId});

				// change the cell to index the current elt reference
				cells[row * xNum + col] = cellRefId;
            }
        }
    }
};

void Grid::remove(int eltId) {
	// index of element in elements list
    Element& element = elements[eltId];

	float r = element.radius;
	float left   = element.cx - r;
    float right  = element.cx + r;
    float top    = element.cy - r;
    float bottom = element.cy + r;
    
    int minRow = static_cast<int>(   top * invYCellSize);
    int maxRow = static_cast<int>(bottom * invYCellSize);

    int minCol = static_cast<int>(  left * invXCellSize);
    int maxCol = static_cast<int>( right * invXCellSize);

    // remove from necessary cells
    for (int row = minRow; row < maxRow; row++) { // rows
        for (int col = minCol; col < maxCol; col++) { // columns
            // check if there are no elements in this cell (commented becaouse the element has to be in it)
            // if (cells[row] == -1) continue;

			int cellRefId = cells[row * xNum + col]; // current element reference
			int prevRefId = -1; // reference to the previous object

			// else iterate until it finds the element in the cell's reference list
			while (true) {
				ElementRef& eltRef = rowElements[row][cellRefId];
				
				// check if there isn't a next element (should NOT be possible)
				if (cellRefId == -1) throw std::runtime_error("Tried to get a non-existing element in a cell!");
				
				if (cellRefId != eltId) {
					// this is not the element. pass to the next one
					prevRefId = cellRefId;
					cellRefId = eltRef.nextInCell;
					continue; // step to next iteration
				}
				
				// --- if this runs, THIS (eltRef) is the element reference we're searching for ---
				
				
				// close gap in element reference links (if not first element)
				if (prevRefId != -1) {
					// NOT FIRST
					rowElements[row][prevRefId].nextInCell = eltRef.nextInCell;
				} else {
					// FIRST -> set cell to contain the next element reference
					cells[row * yNum + col] = eltRef.nextInCell;
				}

				rowElements[row].erase(cellRefId);
				break;
			}
        }
    }
	
	// remove element from elements (COMMENTED OUT BECAUSE OF OPTIMISATION TRY)
	// elements.erase(eltId);
}

void Grid::eraseElement(int eltId) {
	elements.erase(eltId);
};

void Grid::cleanup() {
	// de-allocate empty rows
	for (int i = 0; i < yNum; i++) {
		if (rowElements[i].length == 0 && !rowElements[i].cleared) rowElements[i].clear();
	}
};
