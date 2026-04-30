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

