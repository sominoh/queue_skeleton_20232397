#include <iostream>
#include "queue.h"

Queue* init(void) {
    Queue* q = new Queue;
    q->head = nullptr;
    return q;
}

void release(Queue* queue) {
    std::lock_guard<std::mutex> guard(queue->lock);
    Node* curr = queue->head;
    while (curr) {
        Node* next = curr->next;
        delete curr;
        curr = next;
    }
    delete queue;
}

Node* nalloc(Item item) {
    Node* node = new Node;
    node->item = item;
    node->next = nullptr;
    return node;
}

void nfree(Node* node) {
    delete node;
}

Node* nclone(Node* node) {
    return nalloc(node->item);
}

Reply enqueue(Queue* queue, Item item) {
    std::lock_guard<std::mutex> guard(queue->lock);
    Node* new_node = nalloc(item);

    if (!queue->head || item.key > queue->head->item.key) {
        new_node->next = queue->head;
        queue->head = new_node;
    } else {
        Node* curr = queue->head;
        while (curr->next && curr->next->item.key >= item.key) {
            curr = curr->next;
        }
        new_node->next = curr->next;
        curr->next = new_node;
    }

    return { true, item };
}

Reply dequeue(Queue* queue) {
    std::lock_guard<std::mutex> guard(queue->lock);
    if (!queue->head) return { false, {} };

    Node* node = queue->head;
    queue->head = node->next;
    Item item = node->item;
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
