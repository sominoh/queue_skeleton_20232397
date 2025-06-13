#include <iostream>
#include <cstring>
#include <cstdlib>
#include <new>
#include <cstdint>
#include <cassert>
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

Node* nalloc(Item item) {
    Node* node = new(std::nothrow) Node;
    if (!node) return nullptr;

    node->item.key = item.key;
    int sz = (item.value_size > 0 && item.value_size <= 1024) ? item.value_size : DEFAULT_VALUE_SIZE;
    node->item.value_size = sz;

    node->item.value = malloc(sz);
    if (node->item.value && item.value && item.value_size > 0 && item.value_size <= 1024) {
        memcpy(node->item.value, item.value, sz);
    } else if (node->item.value) {
        memset(node->item.value, 0, sz);
    }

    node->next = nullptr;
    return node;
}

void nfree(Node* node) {
    if (node->item.value)
        free(node->item.value);
    delete node;
}

Node* nclone(Node* node) {
    return nalloc(node->item);
}

Reply enqueue(Queue* queue, Item item) {
    Node* new_node = alloc_node(queue);
    if (!new_node) return { false, {} };

    int sz = (item.value_size > 0 && item.value_size <= 1024) ? item.value_size : DEFAULT_VALUE_SIZE;
    new_node->item.key = item.key;
    new_node->item.value_size = sz;

    new_node->item.value = malloc(sz);
    if (new_node->item.value && item.value && item.value_size > 0 && item.value_size <= 1024) {
        memcpy(new_node->item.value, item.value, sz);
    } else if (new_node->item.value) {
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
        if (curr->item.value && item.value && item.value_size > 0 && item.value_size <= 1024) {
            memcpy(curr->item.value, item.value, sz);
        } else if (curr->item.value) {
            memset(curr->item.value, 0, sz);
        }
        curr->item.value_size = sz;

        assert(queue->size >= 0);
        return { true, curr->item };
    }

    if (!prev) {
        new_node->next = queue->head;
        queue->head = new_node;
        if (!queue->tail) queue->tail = new_node;
    } else {
        new_node->next = prev->next;
        prev->next = new_node;
        if (!new_node->next) queue->tail = new_node;
    }

    queue->size++;
    assert(queue->size >= 0);
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
    assert(queue->size >= 0);

    return { true, item };
}

Queue* range(Queue* queue, Key start, Key end) {
    Queue* new_queue = init();

    Node* curr;
    {
        std::lock_guard<std::mutex> guard(queue->head_lock);
        curr = queue->head;
    }

    while (curr) {
        if (curr->item.key >= start && curr->item.key <= end) {
            enqueue(new_queue, curr->item);
        }
        curr = curr->next;
    }

    return new_queue;
}
