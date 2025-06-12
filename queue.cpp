#include <iostream>
#include <cstring>
#include <cstdlib>
#include "queue.h"

Queue* init(void) {
    Queue* q = new Queue;
    q->head = nullptr;
    q->tail = nullptr;
    return q;
}

void release(Queue* queue) {
    {
        std::lock_guard<std::mutex> guard(queue->lock);
        Node* curr = queue->head;
        while (curr) {
            Node* next = curr->next;
            nfree(curr);
            curr = next;
        }
    }
    delete queue;
}

Node* nalloc(Item item) {
    Node* node = new Node;
    node->item.key = item.key;
    node->item.value_size = item.value_size;
    node->item.value = item.value;  // 그냥 포인터 저장
    node->next = nullptr;
    return node;
}

void nfree(Node* node) {
    // node->item.value는 malloc으로 생성되지 않음
    delete node;
}

Node* nclone(Node* node) {
    return nalloc(node->item);
}

Reply enqueue(Queue* queue, Item item) {
    std::lock_guard<std::mutex> guard(queue->lock);

    Node* curr = queue->head;
    Node* prev = nullptr;

    while (curr) {
        if (curr->item.key == item.key) {
            curr->item.value = item.value;
            curr->item.value_size = item.value_size;
            return { true, curr->item };
        }
        if (item.key > curr->item.key) break;
        prev = curr;
        curr = curr->next;
    }

    Node* new_node = nalloc(item);
    if (!prev) {
        new_node->next = queue->head;
        queue->head = new_node;
        if (!queue->tail) queue->tail = new_node;
    }
    else {
        new_node->next = prev->next;
        prev->next = new_node;
        if (!new_node->next) queue->tail = new_node;
    }

    return { true, new_node->item };
}

Reply dequeue(Queue* queue) {
    std::lock_guard<std::mutex> guard(queue->lock);
    if (!queue->head) return { false, {} };

    Node* node = queue->head;
    queue->head = node->next;
    if (!queue->head) queue->tail = nullptr;

    Item item;
    item.key = node->item.key;
    item.value_size = node->item.value_size;
    item.value = node->item.value;

    nfree(node);
    return { true, item };
}

Queue* range(Queue* queue, Key start, Key end) {
    std::lock_guard<std::mutex> guard(queue->lock);

    Queue* new_queue = init();
    Node* curr = queue->head;

    while (curr) {
        Key key = curr->item.key;
        if (key >= start && key <= end) {
            enqueue(new_queue, curr->item);
        }
        curr = curr->next;
    }

    return new_queue;
}
