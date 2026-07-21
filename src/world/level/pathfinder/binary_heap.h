
#ifndef MCPSP_WORLD_LEVEL_PATHFINDER_BINARY_HEAP_H
#define MCPSP_WORLD_LEVEL_PATHFINDER_BINARY_HEAP_H

#include "world/level/pathfinder/node.h"
#include <cfloat>

class BinaryHeap {
public:
    BinaryHeap() : _size(0), _maxSize(1024) { _heap = new Node*[_maxSize]; }
    ~BinaryHeap() { delete[] _heap; }

    Node* insert(Node* node) {
        if (_size == _maxSize) {
            Node** nh = new Node*[_maxSize = _size << 1];
            for (int i = 0; i < _size; ++i) nh[i] = _heap[i];
            delete[] _heap; _heap = nh;
        }
        _heap[_size] = node;
        node->heapIdx = _size;
        upHeap(_size++);
        return node;
    }

    void clear() { _size = 0; }

    Node* pop() {
        Node* popped = _heap[0];
        _heap[0] = _heap[--_size];
        _heap[_size] = 0;
        if (_size > 0) downHeap(0);
        popped->heapIdx = -1;
        return popped;
    }

    void changeCost(Node* node, float newCost) {
        float oldCost = node->f;
        node->f = newCost;
        if (newCost < oldCost) upHeap(node->heapIdx);
        else                   downHeap(node->heapIdx);
    }

    int  size()    { return _size; }
    bool isEmpty() { return _size == 0; }

private:
    void upHeap(int idx) {
        Node* node = _heap[idx];
        float cost = node->f;
        while (idx > 0) {
            int parentIdx = (idx - 1) >> 1;
            Node* parent = _heap[parentIdx];
            if (cost < parent->f) { _heap[idx] = parent; parent->heapIdx = idx; idx = parentIdx; }
            else break;
        }
        _heap[idx] = node; node->heapIdx = idx;
    }

    void downHeap(int idx) {
        Node* node = _heap[idx];
        float cost = node->f;
        while (true) {
            int leftIdx = 1 + (idx << 1);
            int rightIdx = leftIdx + 1;
            if (leftIdx >= _size) break;
            Node* leftNode = _heap[leftIdx];
            float leftCost = leftNode->f;
            Node* rightNode; float rightCost;
            if (rightIdx >= _size) { rightNode = 0; rightCost = FLT_MAX; }
            else { rightNode = _heap[rightIdx]; rightCost = rightNode->f; }
            if (leftCost < rightCost) {
                if (leftCost < cost) { _heap[idx] = leftNode; leftNode->heapIdx = idx; idx = leftIdx; }
                else break;
            } else {
                if (rightCost < cost) { _heap[idx] = rightNode; rightNode->heapIdx = idx; idx = rightIdx; }
                else break;
            }
        }
        _heap[idx] = node; node->heapIdx = idx;
    }

    Node** _heap;
    int _size, _maxSize;
};

#endif
