//
//  hashmap.c
//  HW1Lecture2016
//
//  Created by Francisco on 7/5/16.
//  Copyright © 2016 Francisco. All rights reserved.
//

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hash_map.h"

// Default hash function, implements the FNV-1a hash algorithm
static size_t Default_HashFunction(void * key);

// Allocates memory, and initializes new node pointer
static NodePtr createNode(HashMapPtr map, void * key, void * value);

// Default compare function, casts keys to char * and
// uses strcmp internally to compare keys
static size_t Default_KeyCompare(void * key, void * otherKey);

HashMapPtr HashMap_New(size_t * capacity,
                       HashFunction hashFunction,
                       KeyCompare keyCompare)
{
    // allocate memory
    HashMapPtr map = malloc(sizeof(struct HashMap));
    
    // return map if initialized correctly, else null
    return HashMap_Init(map, capacity, hashFunction, keyCompare)? map : NULL;
}

void HashMap_Free(HashMapPtr map)
{
    HashMap_Destroy(map);
    free(map);
}

int HashMap_Init(HashMapPtr map,
                 size_t * capacity,
                 HashFunction hashFunction,
                 KeyCompare keyCompare)
{
    assert(map);
    
    map->size = 0;
    map->capacity = (capacity) ? *capacity : DEFAULT_HASHMAP_SIZE;
    map->hashFunction = (hashFunction) ? hashFunction : Default_HashFunction;
    map->keyCompare = (keyCompare)? keyCompare : Default_KeyCompare;
    
    map->hashTable = calloc(map->capacity, sizeof(struct BaseNode));
    
    if (map->hashTable == NULL)
        return 0;
    
    return 1;
}

void HashMap_Add(HashMapPtr map, void * key, void * value)
{
    assert(map);
    assert(key);
    
    //let's get the key
    int hashCode = map->hashFunction(key);
    int index = hashCode % map->capacity;
    
    BaseNodePtr baseNode = & map->hashTable[index];
    NodePtr currentNode = baseNode->firstNode;
    //check if first bucket is unused
    if (currentNode == NULL) {
        // but we need to allocated baseNode
        baseNode->firstNode = createNode(map, key, value);
        map->size++;
        
        return;
    } else if (map->keyCompare(key, currentNode->key) == 0) {
       //0 means they are both equal.
       //see: http://www.cplusplus.com/reference/cstring/strcmp/
       //
       //0 because is a comparison function.
       //update is enough
        currentNode->value = value;
        return;
    }
    
    // bucket is used, so collision may happen if key is different.
    while (currentNode->nextNode) {
        if (map->keyCompare(key, currentNode->key) == 0) {
           currentNode->value = value;
           return;
        }
        
        //iterates
        currentNode = currentNode->nextNode;
    }

    // at this point, it wasn't found
    // so we must add it at the end of the list.
    currentNode->nextNode = createNode(map, key, value);
    // we need to update collision
    
    baseNode->collisions++;
    
    return;
}

void HashMap_Destroy(HashMapPtr map)
{
    assert(map);
    
    int index = 0;
    for (; index < map->capacity; ++index) {
        NodePtr currentNode = map->hashTable[index].firstNode;
        while (currentNode != NULL) {
            NodePtr nextNode = currentNode->nextNode;
            free(currentNode);
            currentNode = nextNode;
        }
    }
    
    free(map->hashTable);
}


NodePtr HashMap_GetNode(HashMapPtr map, void * key)
{
    assert(map);
    assert(key);
    
    int hashCode = map->hashFunction(key);
    int index = hashCode % map->capacity;
    
    NodePtr currentNode = map->hashTable[index].firstNode;
    
    // search list for node with key
    while (currentNode) {
        if (map->keyCompare(key, currentNode->key) == 0) {
            //found
            return currentNode;
        }
        
        // iterate
        currentNode = currentNode->nextNode;
    }
    
    return NULL;
}

void * HashMap_GetValue(HashMapPtr map, void * key)
{
    NodePtr node  = HashMap_GetNode(map, key);
    if (node)
        return node->value;
    
    return NULL;
}

NodePtr createNode(HashMapPtr map, void * key, void * value)
{
    int hashCode = map->hashFunction(key);
    //Node is short of struct Node
    NodePtr node = malloc(sizeof(struct Node));
    node->nextNode = NULL;
    node->key = key;
    node->hashCode = hashCode;
    node->value = value;
    
    return node;
}

size_t Default_HashFunction(void * key)
{
    //not that it works for char * or short int
    char * temp = (char *) key;
    
    int const FNV_PRIME = 16777619;
    int const FNV_OFFSET_BASIS = 2166136261;
    int hash = FNV_OFFSET_BASIS;
    
    int i = 0;
    for (i = 0; i < sizeof(temp) / sizeof(char *); i++)
    {
        hash ^= temp[i];
        hash *= FNV_PRIME;
    }
    
    return hash;
}


size_t Default_KeyCompare(void * key, void * otherKey)
{
    char * K1 = (char *)key;
    char * K2 = (char *)otherKey;
    
    //remember that this returns 0 if they are equal.
    return strcmp(K1, K2);
}