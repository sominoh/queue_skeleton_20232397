#include <iostream>
#include <cstring>
#include <cstdlib>
#include <new>
#include <cstdint>
#include "queue.h"

const int DEFAULT_VALUE_SIZE = sizeof(int);

Queue* init(void) {
    Queue* q = new Queue;
    q->head = nullptr;
    q->tail = nullptr;
    q->size = 0;                
    return q;
}

void release(Queue* queue) {
    {
        std::lock_guard<std::mutex> head_guard(queue->head_lock);
        std::lock_guard<std::mutex> tail_guard(queue->tail_lock);
        Node* curr = queue->head;
        while (curr) {
            Node* next = curr->next;
            if (curr->item.value) {
                free(curr->item.value);   
                curr->item.value = nullptr;
            }
            delete curr;
            curr = next;
        }

        while (!queue->freelist.empty()) {  
            Node* node = queue->freelist.top();
            queue->freelist.pop();
            delete node;
        }
    }
    delete queue;
}

Node* alloc_node(Queue* queue) {
    std::lock_guard<std::mutex> guard(queue->freelist_lock);  
    if (!queue->freelist.empty()) {
        Node* node = queue->freelist.top();
        queue->freelist.pop();
        node->next = nullptr;
        return node;
    }
    return new(std::nothrow) Node;  
}

void free_node(Queue* queue, Node* node) {
    if (node->item.value) {
        free(node->item.value);   
        node->item.value = nullptr;
    }
    node->item.value_size = 0;
    node->item.key = 0;
    node->next = nullptr;

    std::lock_guard<std::mutex> guard(queue->freelist_lock);
    queue->freelist.push(node);     
}

Reply enqueue(Queue* queue, Item item) {
    Node* new_node = alloc_node(queue);
    if (!new_node) return { false, {} };

    int sz = (item.value_size > 0 && item.value_size <= 1024) ? item.value_size : DEFAULT_VALUE_SIZE;
    new_node->item.key = item.key;
    new_node->item.value_size = sz;

    new_node->item.value = malloc(sz);    
    if (new_node->item.value && item.value) {
        int val = static_cast<int>(reinterpret_cast<intptr_t>(item.value));
        memcpy(new_node->item.value, &val, sizeof(int));
    }
    else if (new_node->item.value) {
        memset(new_node->item.value, 0, sz);
    }

    std::lock_guard<std::mutex> guard(queue->tail_lock);

    Node* curr = queue->head;
    Node* prev = nullptr;

    while (curr && curr->item.key > item.key) {
        prev = curr;
        curr = curr->next;
    }

    if (curr && curr->item.key == item.key) {
        if (new_node->item.value)
            free(new_node->item.value);
        free_node(queue, new_node);

        if (curr->item.value)
            free(curr->item.value);
        curr->item.value = malloc(sz);
        if (curr->item.value && item.value) {
            int val = static_cast<int>(reinterpret_cast<intptr_t>(item.value));
            memcpy(curr->item.value, &val, sizeof(int));
        }
        else if (curr->item.value) {
            memset(curr->item.value, 0, sz);
        }
        curr->item.value_size = sz;
        return { true, curr->item };
    }

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

    queue->size++;    
    return { true, new_node->item };
}

Reply dequeue(Queue* queue) {
    std::lock_guard<std::mutex> guard(queue->head_lock);
    if (!queue->head) return { false, {} };

    Node* node = queue->head;
    queue->head = node->next;
    if (!queue->head)
        queue->tail = nullptr;

    int sz = (node->item.value_size > 0 && node->item.value_size <= 1024) ? node->item.value_size : DEFAULT_VALUE_SIZE;

    Item item;
    item.key = node->item.key;
    item.value_size = sz;
    item.value = malloc(sz);

    if (item.value && node->item.value)
        memcpy(item.value, node->item.value, sz);
    else if (item.value)
        memset(item.value, 0, sz);

    free_node(queue, node);   
    queue->size--;            

    return { true, item };
}

Queue* range(Queue* queue, Key start, Key end) {
    Queue* new_queue = init();

    Node* curr;
    {
        std::lock_guard<std::mutex> guard(queue->head_lock);
        curr = queue->head;
        while (curr) {
            Key key = curr->item.key;
            if (key >= start && key <= end) {
                Item copied_item;
                copied_item.key = key;
                copied_item.value_size = curr->item.value_size;
                copied_item.value = malloc(copied_item.value_size);
                if (copied_item.value && curr->item.value) {
                    memcpy(copied_item.value, curr->item.value, copied_item.value_size);
                }
                enqueue(new_queue, copied_item);
            }
            curr = curr->next;
        }
    }

    return new_queue;
}
