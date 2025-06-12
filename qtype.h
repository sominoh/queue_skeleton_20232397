#ifndef _QTYPE_H  
#define _QTYPE_H

#include <mutex>
#include <stack>      // <--- freelist용 스택 추가
#include <atomic>     // <--- atomic 변수 사용 추가

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

struct Node {
    Item item = { 0, nullptr, 0 };
    Node* next = nullptr;
};

struct Queue {
    Node* head = nullptr;
    Node* tail = nullptr;

    std::mutex head_lock;
    std::mutex tail_lock;

    std::stack<Node*> freelist;   
    std::mutex freelist_lock;     

    std::atomic<int> size = 0;    
};

#endif
