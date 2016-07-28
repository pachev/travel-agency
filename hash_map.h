///  @file hashmap.h
///  HW1Lecture2016
///
///  Created by Francisco on 7/5/16.
///  Copyright © 2016 Francisco. All rights reserved.

#ifndef hashmap_h
#define hashmap_h

#include <stdint.h>
#include <string.h>

//call back for hashcode function
typedef size_t (*HashFunction)(void *);
typedef size_t (*KeyCompare)(void *, void *);

typedef struct Node * NodePtr;
struct Node
{
    void * key;
    void * value;
    NodePtr nextNode;
    size_t hashCode;
};

typedef struct BaseNode * BaseNodePtr;
struct BaseNode
{
    //this could be called firstNode
    NodePtr firstNode;
    size_t collisions;
};

//just to keep it simple
typedef struct HashMap * HashMapPtr;
struct HashMap
{
    size_t size;
    size_t capacity;
    BaseNodePtr hashTable;
    HashFunction hashFunction;
    KeyCompare keyCompare;
    
    // a few functions
};

static const int DEFAULT_HASHMAP_SIZE = 16;

/// @brief Allocates memory and initializes new HashMap
/// @returns NULL if error
HashMapPtr HashMap_New(size_t * capacity,
                       HashFunction hashFunction,
                       KeyCompare keyCompare);

/// @brief Deallocates memory and destroys hashmap internally
///  
void HashMap_Free(HashMapPtr map);

/// @brief Initializes map's hash table.
/// @returns 1 if allocation is successful, else 0.
/// Hash table's capacity, hash, and key compare are determined
/// by parameters, otherwise default values are assigned.
int HashMap_Init(HashMapPtr map,
                size_t * capacity,
                HashFunction hashFunction,
                KeyCompare keyCompare);

/// @brief Resets destroys hashmap internals
///
void HashMap_Destroy(HashMapPtr map);

/// @brief Adds new node to hashmap table
/// If collision occurs, appends node to list at index
/// If node with key is found, updates value
void HashMap_Add(HashMapPtr map,
                 void * key,
                 void * value);

/// @brief Searches hashmap for node with key
/// @returns NULL if not found, else returns node's value
/// Calls keyCompare to determine equivalence
void * HashMap_GetValue(HashMapPtr map,
                        void * key);

#endif /* hashmap_h */