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
    } // lock 해제 이후 안전하게 delete
    delete queue;
}

Node* nalloc(Item item) {
    Node* node = new Node;
    node->item.key = item.key;
    node->item.value_size = item.value_size;
    if (item.value_size > 0 && item.value != nullptr) {
        node->item.value = malloc(item.value_size);
        memcpy(node->item.value, item.value, item.value_size);
    }
    else {
        node->item.value = nullptr;
    }
    node->next = nullptr;
    return node;
}

void nfree(Node* node) {
    if (node->item.value != nullptr) {
        free(node->item.value);
    }
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
            if (curr->item.value != nullptr) {
                free(curr->item.value);
            }
            curr->item.value_size = item.value_size;
            curr->item.value = malloc(item.value_size);
            memcpy(curr->item.value, item.value, item.value_size);
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
    if (item.value_size > 0 && node->item.value != nullptr) {
        item.value = malloc(item.value_size);
        memcpy(item.value, node->item.value, item.value_size);
    }
    else {
        item.value = nullptr;
    }

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

