//
//  main.c
//  HashMap
//
//  Created by Francisco on 7/10/16.
//  Copyright © 2016 Francisco. All rights reserved.
//

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "hash_map.h"

int main(int argc, const char * argv[]) {
    // insert code here...
    printf("Hello, World!\n");
    
    HashMapPtr map = HashMap_New(NULL, NULL, NULL);
    assert(map);
    
    char * key1 = "Francisco";
    HashMap_Add(map, key1, "Ortega");
    HashMap_Add(map, "Javier", "Ortega");
    HashMap_Add(map, "Programming", "III");
    HashMap_Add(map, "Summer", "2016");
    HashMap_Add(map, key1, "Ortega Melo");
    HashMap_Add(map, "Summer", "2016 at FIU");
    HashMap_Add(map, "Hello", "World");
    HashMap_Add(map, "Hell", "Heaven");
    HashMap_Add(map, "Hi", "Goodbye");
    HashMap_Add(map, "Summer", "2016");
    
    void * val = HashMap_GetValue(map,"Francisco");
    printf("The value is %s\n", (char *)val);
    val = HashMap_GetValue(map,"Javier");
    printf("The value is %s\n", (char *)val);
    val = HashMap_GetValue(map,"Programming");
    printf("The value is %s\n", (char *)val);
    val = HashMap_GetValue(map,"Summer");
    printf("The value is %s\n", (char *)val);
    val = HashMap_GetValue(map,"Hello");
    printf("The value is %s\n", (char *)val);
    val = HashMap_GetValue(map,"Hell");
    printf("The value is %s\n", (char *)val);
    val = HashMap_GetValue(map,"Hi");
    printf("The value is %s\n", (char *)val);
    val = HashMap_GetValue(map,"NotFound");
    if (!val) printf("The value is Null\n");
    
    HashMap_Free(map);
    
    return 0;
}
