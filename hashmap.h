#ifndef HASHMAP_H
#define HASHMAP_H

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#define LOAD_FACTOR 0.75

typedef uint32_t (*hashFn)(const void *key);
typedef int (*equalFn)(const void *key1, const void *key2);
typedef void (*hashmapForEach)(void *key1, void *key2, void *data);
typedef void (*hashmapDestroyData)(void *item);

typedef struct hashnode {
  void *key;
  void *value;
  struct hashnode *next;
} hashnode;

typedef struct hashmap {
  hashnode **buckets;
  size_t allocatedSize;
  size_t size;
  hashFn hashfn;
  equalFn equalfn;
  hashmapDestroyData destroykey;
  hashmapDestroyData destroyval;
} hashmap;

hashmap *createHashmap(hashFn hashfn, equalFn equalfn,
                       hashmapDestroyData destroykey,
                       hashmapDestroyData destroyvalue);
void *hashmap_lookup(hashmap *map, const void *key);
void hashmap_insert(hashmap *map, void *key, void *value);
void hashmap_foreach(hashmap *map, hashmapForEach forEach, void *data);
void hashmap_destroy(hashmap *map);

#endif
