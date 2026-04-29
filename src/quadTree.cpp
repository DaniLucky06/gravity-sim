#include "quadTree.hpp"


Quadtree::Quadtree(float width, float height, int depth, int elementsPerLeaf) {
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

int Quadtree::allocate_children() {
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

void Quadtree::insert(QuadElt& ball) {
    int index = elts.insert(ball); // Element index in element array
    ball.id = index;

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

                int newChildrenIdx = allocate_children(); // Make new space for 4 children nodes, save the first index
                QuadNode& parentNode = nodes[nd.nodeIndex]; // get parentnode again, as allocation can move memory
                parentNode.first_child = newChildrenIdx;
                parentNode.count = -1; // Turn to branch

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
                            
                            eltNodes[targetNodeIndex].next = nodes[newChildrenIdx + 0].first_child; // Set the next ball index of the new reference to be the index to the first element in the quad it's being put in (chain)
                            nodes[newChildrenIdx + 0].first_child = targetNodeIndex; // set the first_child index of the quad to be the ball ref we're adding
                            nodes[newChildrenIdx + 0].count++; // raise the number of balls in the quad
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
                            
                            eltNodes[targetNodeIndex].next = nodes[newChildrenIdx + 1].first_child; // Set the next ball index of the new reference to be the index to the first element in the quad it's being put in (chain)
                            nodes[newChildrenIdx + 1].first_child = targetNodeIndex; // set the first_child index of the quad to be the ball ref we're adding
                            nodes[newChildrenIdx + 1].count++; // raise the number of balls in the quad
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
                            
                            eltNodes[targetNodeIndex].next = nodes[newChildrenIdx + 2].first_child; // Set the next ball index of the new reference to be the index to the first element in the quad it's being put in (chain)
                            nodes[newChildrenIdx + 2].first_child = targetNodeIndex; // set the first_child index of the quad to be the ball ref we're adding
                            nodes[newChildrenIdx + 2].count++; // raise the number of balls in the quad
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
                            
                            eltNodes[targetNodeIndex].next = nodes[newChildrenIdx + 3].first_child; // Set the next ball index of the new reference to be the index to the first element in the quad it's being put in (chain)
                            nodes[newChildrenIdx + 3].first_child = targetNodeIndex; // set the first_child index of the quad to be the ball ref we're adding
                            nodes[newChildrenIdx + 3].count++; // raise the number of balls in the quad
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

void Quadtree::remove(QuadElt& ball) {
    int index = ball.id; // index to the corresponding element in the QuadElt array

    std::vector<QuadNodeData> toProcess; // vector of QuadNodeData to process
    toProcess.push_back({0, 0, root_rect});

    while(toProcess.size() > 0) {
        QuadNodeData nd = toProcess.back(); // get the next node data
        toProcess.pop_back();

        QuadNode& node = nodes[nd.nodeIndex]; // get the node corresponding to the node data

        if (node.count != -1) {
            // leaf node
            int eltRefIndex = node.first_child; // index to the QuadEltNode element reference
            int prevEltIndex = -1; // index to the previous QuadEltNode element reference

            while (eltRefIndex != -1) {
                QuadEltNode currentEltRef = eltNodes[eltRefIndex]; // get the current element reference
                if (currentEltRef.element == index) {
                    // the analyzed element is the one we need to remove
                    if (prevEltIndex == -1) {
                        // the element is the first element in the list
                        node.first_child = currentEltRef.next; // set the first child to be the next element: forget the current one
                    } else {
                        //  the element is not in first place
                        eltNodes[prevEltIndex].next = currentEltRef.next; // skip current element: forget it
                    }
                    eltNodes.erase(eltRefIndex);

                    node.count--;
                    break;
                }

                prevEltIndex = eltRefIndex;
                eltRefIndex = currentEltRef.next;
            }
        } else {
            // not a leaf node
            const float mx = nd.rect.mx;
            const float my = nd.rect.my;
            const float hx = nd.rect.hx * 0.5f; // New half-width
            const float hy = nd.rect.hy * 0.5f; // New half-height
            const float l = mx-hx, t = my-hy, r = mx+hx, b = my+hy;

            const int firstChildIndex = node.first_child;
            const int nextDepth = nd.depth + 1;

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

    elts.erase(index);
}

void Quadtree::cleanup () {
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

std::vector<Quadtree::CollisionData> Quadtree::getCollisions() {
    std::vector<Quadtree::CollisionData> collisions;
    std::vector<int> toProcess;

    if (nodes[0].count != 0) {
        toProcess.push_back(0);
    }

    while (toProcess.size() > 0) {

        int nodeIndex = toProcess.back();
        toProcess.pop_back();
        QuadNode& node = nodes[nodeIndex];

        if (node.count != -1) {
            // leaf node
            int firstRefIndex = node.first_child;

            // check if there at least 2 elements
            if (node.count >= 2) {
                for (int ref1 = firstRefIndex; ref1 != -1; ref1 = eltNodes[ref1].next) {
                    QuadElt& ball1 = elts[eltNodes[ref1].element];

                    for (int ref2 = eltNodes[ref1].next; ref2 != -1; ref2 = eltNodes[ref2].next) {
                        QuadElt& ball2 = elts[eltNodes[ref2].element];

                        if (ball1.minX <= ball2.maxX && ball1.minY <= ball2.maxY && 
                            ball1.maxX >= ball2.minX && ball1.maxY >= ball2.minY) {
                                collisions.push_back({ball1.objId, ball2.objId});
                        }
                    }
                }
            }
        } else {
            // not a leaf node
            for (int i = 0; i < 4; i++) {
                toProcess.push_back(node.first_child + i);
            }
        }
    }

    return collisions;
};