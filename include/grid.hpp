#pragma once

#include <vector>
#include <cstdint>
#include <SFML/Graphics.hpp>



template <class T>
class FreeList {
private:
    union FreeElement {
        T element;
        int next;
    };
    
    std::vector<FreeElement> data;
    int first_free; // The "head" of our invisible linked list
public:
    int length = -1;
    int cleared = true;

    FreeList() {
        first_free = -1;
    };

    /**
     * @brief Insert element to list
     * 
     * @param element Element to insert
     * @return Index of the element you just inserted
     */
    int insert(const T& element) {
        length++;
        cleared = false;
        if (first_free != -1) {
            // save free index
            const int index = first_free;
            // get next free index (stored in the data at current free index)
            first_free = data[first_free].next;
            // save the input element to index
            data[index].element = element;

            return index;
        } else {
            // crete new element
            FreeElement fe;
            // set it's element
            fe.element = element;
            // add new element to data list
            data.push_back(fe);

            // return the index to the last element
            return static_cast<int>(data.size() - 1);
        };
    };

    /**
     * @brief Erase item from list
     * 
     * @param index Index of element to remove
     */
    void erase (int index) {
        data[index].element.mass = 0.f;
        
        data[index].next = first_free;
        first_free = index;
        length--;
    };

    /**
     * @brief Empty list
     */
    void clear () {
        data.clear();
        first_free = -1;
        length = 0;
        cleared = true;
    };

    /**
     * @brief Get the list range in memory
     * 
     * @return Number of elements as int
     */
    int range() const {
        return static_cast<int>(data.size());
    };

    T& operator[](int n) {
        return data[n].element;
    };

    const T& operator[](int n) const {
        return data[n].element;
    };
};

struct ElementIndex {
    uint32_t cellId;
    uint32_t ballId;
};

// The ball
struct Element {
    float radius;
    float mass;
    float rest;
    float vx, vy;
    uint32_t color;
    

    // for grid
    float cx, cy; // center pos
};  

constexpr uint32_t INVALID_REF = 0xFFFFFFFF;

/**
 * Grid class to efficiently manage elements stored in a grid.
 */
class Grid {
private:
    float xCellSize, yCellSize;
    float invXCellSize, invYCellSize;
    float width, height;
    
    public:
    uint32_t xNum, yNum, numCells;
    uint32_t activeParticleCount;
    std::vector<uint32_t> cellStart;
    std::vector<ElementIndex> indices;
    std::vector<ElementIndex> indicesTemp;

    /**
     * @brief List of elements in the grid
     */
    FreeList<Element> elements;

    Grid() = default;
    Grid(float _width, float _height, uint32_t _xNum, uint32_t _yNum, uint32_t _nBalls):
        width(_width), height(_height),
        xNum(_xNum), yNum(_yNum),
        
        xCellSize(_width / _xNum), yCellSize(_height / _yNum),

        invXCellSize(_xNum / _width), invYCellSize(_yNum / _height)
    {
        numCells = _xNum * _yNum;
        cellStart.resize(numCells);
    };

    /**
     * @brief Add `element` to the grid's list (you have to call `insert()`, too)
     * 
     * @param element The element to add
     * @return Index to the element in the grid's list
     */
    uint32_t addElement(Element element);

    /**
     * @brief Insert an element into the grid structure
     * 
     * @param eltId The index of the element to insert
     */
    void insert(uint32_t eltId);

    /**
     * @brief Remove element from the grid cells (for moving the element)
     * 
     * @param eltId Index of the element to remove
     *
    void remove(uint32_t eltId);
     */

    /**
     * @brief Remove an element from the `elements` list. RUN `remove()` FIRST!
     * 
     * @param eltId Index of the element to remove
     */
    void eraseElement(uint32_t eltId);

    /**
     * @brief Check all possible collision pairs (BROAD PHASE)
     * 
     * @param[out] collisionPairs Array to store the collision pairs in
     */
    // void calculateCollisions(std::vector<uint64_t>& collisionPairs);

    /**
     * @brief Clear the grid (not the elements list)
     */
    void clear();

    void build();
};
