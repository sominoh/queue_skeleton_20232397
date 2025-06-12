#ifndef _QTYPE_H
#define _QTYPE_H

#include <mutex> // mutex 추가

typedef unsigned int Key;  
typedef void* Value;

typedef struct {
    Key key;
    Value value;
    int value_size; // byte
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
    std::mutex lock; // thread-safe를 위한 뮤텍스 추가
} Queue;

#endif
