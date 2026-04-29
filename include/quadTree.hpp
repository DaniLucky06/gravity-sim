#pragma once

#include <vector>
#include <cstdint>

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
        data[index].next = first_free;
        first_free = index;
    };

    /**
     * @brief Empty list
     */
    void clear () {
        data.clear();
        first_free = -1;
    };

    /**
     * @brief Get the list size
     * 
     * @return List size as int
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

// 8-byte node (has first_child and count)
struct QuadNode {
    int32_t first_child; // Index to first of 4 contiguous children, or first element if leaf
    int32_t count;       // -1 if branch, >= 0 if leaf
};

// Represents the actual 2D ball
struct QuadElt {
    int id;
    int objId;
    float cx, cy; // Center
    float radius;
    // Bounding box for fast quadtree logic
    float minX, minY, maxX, maxY; 
    
    QuadElt() = default;

    QuadElt(float _radius, float _cx, float _cy) {
        radius = _radius;
        cx = _cx;
        cy = _cy;

        minX = cx - radius;
        maxX = cx + radius;
        minY = cy - radius;
        maxY = cy + radius;
    }
};

// Singly-linked list node for elements in leaves
struct QuadEltNode {
    int next;    // Index of next element node, -1 for end
    int element; // Index to the corresponding element in the QuadElt array
};

// Represents the boundaries of a node computed on the fly
struct QuadCRect {
    float mx, my; // Center X, Y
    float hx, hy; // Half-width, Half-height
};


// helper struct for traversing
struct QuadNodeData {
    int nodeIndex; // index of node in the quadNode array (nodes)
    int depth; // depth of the node
    QuadCRect rect; // location info
};

class Quadtree {
    FreeList<QuadElt> elts; // Array of elements (linked internally)
    FreeList<QuadEltNode> eltNodes; // Array of element references (linked internally)

    std::vector<QuadNode> nodes; // Array of nodes (quad/leaf)

    QuadCRect root_rect; // for eventual quadtree size

    // First free node in memory, the eventual next ones are stored recursively in the nodes
    int freeNode;

    // maximum tree depth
    int maxDepth;
    int maxElementsPerLeaf;

    int allocate_children();

public:
    // Store collision data
    struct CollisionData
    {
        int id1;
        int id2;
    };

    /**
     * @brief Initialize Quadtree
     * 
     * @param width Width of the bounding box
     * @param height Hidth of the bounding box
     * 
     * @param depth Max depth of tree
     * @param elementsPerLeaf Max number of elements per leaf node
     */
    Quadtree(float width, float height, int depth = 8, int elementsPerLeaf = 2);

    /**
     * @brief Allocate memory for 4 children nodes
     * 
     * @return Index to the first allocation
     */
    void insert(QuadElt& ball);

    /**
     * @brief Insert element in tree
     * 
     * @param ball Element to insert
     */
    void remove(QuadElt& ball);

    /**
     * @brief Remove element from tree
     * 
     * @param Element to remove
     */
    void cleanup();

    /**
     * @brief Get all possible collisions
     * 
     * @return Vector of `CollisionData`: pairs of ids of colliding balls
     */
    std::vector<CollisionData> getCollisions();
};