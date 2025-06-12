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
    std::lock_guard<std::mutex> guard(queue->lock);
    Node* curr = queue->head;
    while (curr) {
        Node* next = curr->next;
        delete (int*)curr->item.value; 
        delete curr;
        curr = next;
    }
    delete queue;
}

Reply enqueue(Queue* queue, Item item) {
    std::lock_guard<std::mutex> guard(queue->lock);

    Node* curr = queue->head;
    Node* prev = nullptr;

    while (curr) {
        if (curr->item.key == item.key) {
            delete (int*)curr->item.value; 
            int* val_copy = new int(*(int*)item.value); 
            curr->item.value = val_copy; 
            curr->item.value_size = item.value_size;
            return { true, curr->item };
        }
        if (item.key > curr->item.key) break;
        prev = curr;
        curr = curr->next;
    }

    int* val_copy = new int(*(int*)item.value); 
    Node* new_node = new Node;
    new_node->item.key = item.key;
    new_node->item.value = val_copy; 
    new_node->item.value_size = item.value_size;
    new_node->next = curr;

    if (!prev) {
        queue->head = new_node;
        if (!queue->tail) queue->tail = new_node;
    }
    else {
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

    delete node;
    return { true, item };
}

Queue* range(Queue* queue, Key start, Key end) {
    std::lock_guard<std::mutex> guard(queue->lock);

    Queue* new_queue = init();
    Node* curr = queue->head;

    while (curr) {
        Key key = curr->item.key;
        if (key >= start && key <= end) {
            int* val_copy = new int(*(int*)curr->item.value); 
            Item copied_item = { key, val_copy, curr->item.value_size };
            enqueue(new_queue, copied_item);
        }
        curr = curr->next;
    }

    return new_queue;
}
