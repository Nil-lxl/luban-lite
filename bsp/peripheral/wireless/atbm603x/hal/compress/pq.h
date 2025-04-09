/*
*File: pq.h
*purpose: declaration of priority queue in C
*Author:puresky
*Date:2011/04/27
*/
#ifndef _PRIORITY_QUEUE_H
#define _PRIORITY_QUEUE_H

// =============KeyValue Struct==================================
typedef struct key_value_struct KeyValue;
struct key_value_struct
{
      int _key;
      void *_value;
};
KeyValue *key_value_new(int key, void *value);
void key_value_free(KeyValue *kv, void (*freevalue)(void *));

// =============PriorityQueue Struct==============================
#define PRIORITY_MAX 1
#define PRIORITY_MIN 2
typedef struct priority_queue_struct PriorityQueue;
struct priority_queue_struct
{
      KeyValue **_nodes;
      int _size;
      int _capacity;
     
      int _priority;
};
PriorityQueue *priority_queue_new(int priority);
void priority_queue_free(PriorityQueue *pq, void (*freevalue)(void *));
const KeyValue *priority_queue_top(PriorityQueue *pq);
KeyValue *priority_queue_dequeue(PriorityQueue *pq);
void priority_queue_enqueue(PriorityQueue *pq, KeyValue *kv);
int priority_queue_size(PriorityQueue *pq);
int priority_queue_empty(PriorityQueue *pq);
void priority_queue_print(PriorityQueue *pq);

#endif