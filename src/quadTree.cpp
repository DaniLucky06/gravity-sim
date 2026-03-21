#include "quadTree.hpp"

#include <vector>
#include <cstdint>


template <class T>
class FreeList {
public:
    FreeList() {
        first_free = -1;
    };

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

    void erase (int n) {
        data[n].next = first_free;
        first_free = n;
    };

    void clear () {
        data.clear();
        first_free = -1;
    };

    int range() const {
        return static_cast<int>(data.size());
    };

    T& operator[](int n) {
        return data[n].element;
    };

    const T& operator[](int n) const {
        return data[n].element;
    };

private:
    union FreeElement {
        T element;
        int next;
    };
    
    std::vector<FreeElement> data;
    int first_free; // The "head" of our invisible linked list
};

// 8-byte node (has first_child and count)
struct QuadNode {
    int32_t first_child; // Index to first of 4 contiguous children, or first element if leaf
    int32_t count;       // -1 if branch, >= 0 if leaf
};

// Represents the actual 2D ball
struct QuadElt {
    int id;
    float cx, cy; // Center
    float radius;
    // Bounding box for fast quadtree logic
    float minX, minY, maxX, maxY; 
    /* 
    QuadElt() = default;

    QuadElt(int _radius, int _cx, int _cy) {
        radius = _radius;
        cx = _cx;
        cy = _cy;

        minX = cx - radius;
        maxX = cx + radius;
        minY = cy - radius;
        maxY = cy + radius;
    } */
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
public:
    Quadtree(float width, float height, int depth = 8, int elementsPerLeaf = 2) {
        // 1. Initialize your max_depth and max_elements_per_leaf.
        maxDepth = depth;
        maxElementsPerLeaf = elementsPerLeaf;

        // 2. Set your free_node to -1 (no recycled nodes yet).
        freeNode = -1;

        // Init the root rect
        root_rect.hx = width * .5f;
        root_rect.hy = height * .5f;
        root_rect.mx = root_rect.hx;
        root_rect.my = root_rect.hy;

        // 4. THE TRICKY PART: Initialize the root node.
        nodes.push_back({-1, 0});
    }

    int allocate_children() {
        // check if there is a 4-space free
        if (freeNode != -1) {
            int index = freeNode; // save the index
            
            // point to the next free space
            freeNode = nodes[index].first_child; 
            
            // save the 4 empty leaves
            for(int i = 0; i < 4; ++i) {
                nodes[index + i].first_child = -1;
                nodes[index + i].count = 0;
            }
            return index; // Return the start index
        } 
        // no 4-space available
        else {
            int index = nodes.size(); // new index
            
            // add 4 new leaves
            for(int i = 0; i < 4; ++i) {
                nodes.push_back({-1, 0});
            }
            return index;
        }
    }

    void insert(const QuadElt& ball) {
        int index = elts.insert(ball); // Element index in element array

        std::vector<QuadNodeData> toProcess; // Array of QuadNodeData left to process
        toProcess.push_back({0, 0, root_rect}); // Add the root to the process stack

        while (toProcess.size() > 0) {
            QuadNodeData nd = toProcess.back(); // The nodeData to process in the while loop
            toProcess.pop_back();

            QuadNode& node = nodes[nd.nodeIndex]; // Reference to the QuadNode node

            if (node.count != -1) {
                // This is a leaf node
                QuadEltNode eltRef; // New element reference
                eltRef.element = index; // Set it to point to the element
                eltRef.next = node.first_child; // Set it to link the current first element in the leaf

                int nodeIndex = eltNodes.insert(eltRef); // Get the index when adding the element reference
                node.first_child = nodeIndex; // Set the node to link to this element reference
                node.count++; // Increment the element count on the leaf by 1

                // Only if there are too many elements, and we're not at max depth yet
                if (node.count > maxElementsPerLeaf && nd.depth < maxDepth) {

                    int currentEltNodeIndex = node.first_child; // Save the index of the first element reference
                    node.first_child = allocate_children(); // Make new space for 4 children nodes, save the first index
                    
                    node.count = -1; // Turn to branch

                    const float mx = nd.rect.mx, my = nd.rect.my; // Get the middle of the quad we're splitting

                    // Element assignment: currentEltNodeIntex gets updated element through element until they're sorted and it's -1
                    while (currentEltNodeIndex != -1) {
                        QuadEltNode& currentEltNode = eltNodes[currentEltNodeIndex]; // Get eltNode reference from its index
                        QuadElt& ball = elts[currentEltNode.element]; // Get element reference from the index in its eltNode

                        int nextEltNodeIndex = currentEltNode.next; // Save the index of the next element to process

                        // Check which child the element has to go into
                        bool recycled = false;
                        if (ball.minY <= my) { // Top
                            if (ball.minX <= mx) { // Top left
                                int targetNodeIndex; // Index of the eltNode we're moving to the quad
                                if (!recycled) {
                                    // if it's the first reallocation
                                    targetNodeIndex = currentEltNodeIndex; // recycle the already existing eltNode
                                    recycled = true;
                                } else {
                                    // new allocations
                                    QuadEltNode newEltNode; // Create new element reference
                                    newEltNode.element = currentEltNode.element; // Link to the same element
                                    targetNodeIndex = eltNodes.insert(newEltNode); // Add the new reference to the list, and save the index
                                }
                                
                                eltNodes[targetNodeIndex].next = nodes[node.first_child + 0].first_child; // Set the next ball index of the new reference to be the index to the first element in the quad it's being put in (chain)
                                nodes[node.first_child + 0].first_child = targetNodeIndex; // set the first_child index of the quad to be the ball ref we're adding
                                nodes[node.first_child + 0].count++; // raise the number of balls in the quad
                            }
                            if (ball.maxX > mx) { // Top right
                                int targetNodeIndex; // Index of the eltNode we're moving to the quad
                                if (!recycled) {
                                    // if it's the first reallocation
                                    targetNodeIndex = currentEltNodeIndex; // recycle the already existing eltNode
                                    recycled = true;
                                } else {
                                    // new allocations
                                    QuadEltNode newEltNode; // Create new element reference
                                    newEltNode.element = currentEltNode.element; // link to the same element
                                    targetNodeIndex = eltNodes.insert(newEltNode); // add the new reference to the list, and save the index
                                }
                                
                                eltNodes[targetNodeIndex].next = nodes[node.first_child + 1].first_child; // Set the next ball index of the new reference to be the index to the first element in the quad it's being put in (chain)
                                nodes[node.first_child + 1].first_child = targetNodeIndex; // set the first_child index of the quad to be the ball ref we're adding
                                nodes[node.first_child + 1].count++; // raise the number of balls in the quad
                            }
                        }
                        if (ball.maxY > my) { // Bottom
                            if (ball.minX <= mx) { // Bottom left
                                int targetNodeIndex; // Index of the eltNode we're moving to the quad
                                if (!recycled) {
                                    // if it's the first reallocation
                                    targetNodeIndex = currentEltNodeIndex; // recycle the already existing eltNode
                                    recycled = true;
                                } else {
                                    // new allocations
                                    QuadEltNode newEltNode; // Create new element reference
                                    newEltNode.element = currentEltNode.element; // link to the same element
                                    targetNodeIndex = eltNodes.insert(newEltNode); // add the new reference to the list, and save the index
                                }
                                
                                eltNodes[targetNodeIndex].next = nodes[node.first_child + 2].first_child; // Set the next ball index of the new reference to be the index to the first element in the quad it's being put in (chain)
                                nodes[node.first_child + 2].first_child = targetNodeIndex; // set the first_child index of the quad to be the ball ref we're adding
                                nodes[node.first_child + 2].count++; // raise the number of balls in the quad
                            }
                            if (ball.maxX > mx) { // Bottom right
                                int targetNodeIndex; // Index of the eltNode we're moving to the quad
                                if (!recycled) {
                                    // if it's the first reallocation
                                    targetNodeIndex = currentEltNodeIndex; // recycle the already existing eltNode
                                    recycled = true;
                                } else {
                                    // new allocations
                                    QuadEltNode newEltNode; // Create new element reference
                                    newEltNode.element = currentEltNode.element; // link to the same element
                                    targetNodeIndex = eltNodes.insert(newEltNode); // add the new reference to the list, and save the index
                                }
                                
                                eltNodes[targetNodeIndex].next = nodes[node.first_child + 3].first_child; // Set the next ball index of the new reference to be the index to the first element in the quad it's being put in (chain)
                                nodes[node.first_child + 3].first_child = targetNodeIndex; // set the first_child index of the quad to be the ball ref we're adding
                                nodes[node.first_child + 3].count++; // raise the number of balls in the quad
                            }
                        }

                        currentEltNodeIndex = nextEltNodeIndex; // Now that the elements are put into the sub-nodes, get the index of the next element (at the end of the elements in the quad i'ts -1)
                    }
                }
            } else {
                // Not a leaf: add subnodes the element would go to to the processing stack
                const float mx = nd.rect.mx;
                const float my = nd.rect.my;
                const float hx = nd.rect.hx * 0.5f; // New half-width
                const float hy = nd.rect.hy * 0.5f; // New half-height
                const float l = mx-hx, t = my-hy, r = mx+hx, b = my+hy;

                const int firstChildIndex = nodes[nd.nodeIndex].first_child; // Get the index of the first of the four children
                const int nextDepth = nd.depth + 1; // Raise the depth of the subnodes


                if (ball.minY <= my) { // Top
                    if (ball.minX <= mx) // Top left
                        toProcess.push_back(QuadNodeData{firstChildIndex + 0, nextDepth, QuadCRect{l, t, hx, hy}});
                    if (ball.maxX > mx)  // Top right
                        toProcess.push_back(QuadNodeData{firstChildIndex + 1, nextDepth, QuadCRect{r, t, hx, hy}});
                }
                if (ball.maxY > my) { // Bottom
                    if (ball.minX <= mx) // Bottom left
                        toProcess.push_back(QuadNodeData{firstChildIndex + 2, nextDepth, QuadCRect{l, b, hx, hy}});
                    if (ball.maxX > mx) // Bottom right
                        toProcess.push_back(QuadNodeData{firstChildIndex + 3, nextDepth, QuadCRect{r, b, hx, hy}});
                }
            }
        }
    }

    void remove(const QuadElt& ball) {
        
    }

    void cleanup () {
        std::vector<int> toProcess; // Array of nodes left to process

        if (nodes[0].count == -1) {
            // Only add the root to the process if it's not a leaf
            toProcess.push_back(0);
        }

        while (toProcess.size() > 0) {
            const int nodeIndex = toProcess.back(); // The index of the node to process
            toProcess.pop_back();
            QuadNode& node = nodes[nodeIndex]; // Node to process

            // Loop through children
            int numEmptyLeafs = 0;
            for (int j=0; j<4; j++) {
                const int childIndex = node.first_child + j; // Get index of the child node in analysis
                QuadNode& childNode = nodes[childIndex]; // Child node in analysis

                // Check child
                if (childNode.count==0) {
                    numEmptyLeafs++; // Child is an empty leaf
                } else if (childNode.count==-1) {
                    toProcess.push_back(childIndex); // Child is a node, add to processing stack
                }
            }

            // If all children were empty leafs, delete them
            if (numEmptyLeafs==4) {
                nodes[node.first_child].first_child = freeNode; // Set the first child's first child index to link to the next free memory location
                freeNode = node.first_child; // Set freeNode to be the first child we just cleared

                // Make this node the empty leaf
                node.first_child = -1;
                node.count = 0;
            }
        }
    }
};