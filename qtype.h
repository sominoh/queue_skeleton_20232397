#ifndef _QTYPE_H  
#define _QTYPE_H

#include <mutex>

typedef unsigned int Key;
typedef void* Value;

typedef struct {
    Key key;
    Value value;
    int value_size;
} Item;

typedef struct {
    bool success;
    Item item;
} Reply;

typedef struct node_t {
    Item item;
    struct node_t* next;
} Node;

typedef struct {
    Node* head;
    Node* tail;
    std::mutex head_lock;  // 락 분리 - head 락 추가
    std::mutex tail_lock;  // 락 분리 - tail 락 추가
} Queue;

#endif
