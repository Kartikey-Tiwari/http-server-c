#include "hashmap.h"
#include <stdint.h>
#include <stdlib.h>

hashmap *createHashmap(hashFn hashfn, equalFn equalfn,
                       hashmapDestroyData destroykey,
                       hashmapDestroyData destroyval) {
  if (!hashfn || !equalfn) {
    return NULL;
  }
  hashmap *map = (hashmap *)malloc(sizeof(hashmap));
  if (!map) {
    return NULL;
  }
  map->allocatedSize = 32;
  map->size = 0;
  map->hashfn = hashfn;
  map->equalfn = equalfn;
  map->buckets = (hashnode **)calloc(map->allocatedSize, sizeof(hashnode *));
  map->destroykey = destroykey;
  map->destroyval = destroyval;
  if (!map->buckets) {
    free(map);
    return NULL;
  }
  return map;
}

void *hashmap_lookup(hashmap *map, const void *key) {
  if (!map || !key) {
    return NULL;
  }
  uint32_t hash = map->hashfn(key);
  hash ^= hash >> 16;
  size_t index = hash & (map->allocatedSize - 1);

  hashnode *node = map->buckets[index];
  while (node) {
    if (map->equalfn(node->key, key) == 0) {
      return node->value;
    }
    node = node->next;
  }
  return NULL;
}

hashnode *createNode(void *key, void *value) {
  hashnode *newNode = (hashnode *)malloc(sizeof(hashnode));
  if (!newNode) {
    return NULL;
  }
  newNode->key = key;
  newNode->value = value;
  newNode->next = NULL;
  return newNode;
}

void hashmap_resize(hashmap *map) {
  size_t newSize = map->allocatedSize * 2;
  hashnode **bucket = (hashnode **)calloc(newSize, sizeof(hashnode *));
  if (!bucket) {
    return;
  }

  for (size_t i = 0; i < map->allocatedSize; i++) {
    hashnode *node = map->buckets[i];
    while (node) {
      hashnode *next = node->next;

      uint32_t hash = map->hashfn(node->key);
      hash ^= hash >> 16;
      size_t index = hash & (newSize - 1);

      node->next = bucket[index];
      bucket[index] = node;

      node = next;
    }
  }
  map->allocatedSize = newSize;
  free(map->buckets);
  map->buckets = bucket;
}

void hashmap_insert(hashmap *map, void *key, void *value) {
  if (!map || !key) {
    return;
  }
  if ((float)map->size / map->allocatedSize >= LOAD_FACTOR) {
    hashmap_resize(map);
  }

  uint32_t hash = map->hashfn(key);
  hash ^= hash >> 16;
  size_t index = hash & (map->allocatedSize - 1);

  hashnode *node = map->buckets[index];
  while (node) {
    if (map->equalfn(node->key, key) == 0) {
      if (map->destroykey) {
        map->destroykey(node->key);
      }
      if (map->destroyval) {
        map->destroyval(node->value);
      }
      node->key = key;
      node->value = value;
      return;
    }
    node = node->next;
  }

  node = createNode(key, value);
  if (!node) {
    return;
  }
  node->next = map->buckets[index];
  map->buckets[index] = node;
  map->size++;
}

void hashmap_foreach(hashmap *map, hashmapForEach forEach, void *data) {
  for (size_t i = 0; i < map->allocatedSize; i++) {
    hashnode *head = map->buckets[i];
    while (head) {
      forEach(head->key, head->value, data);
      head = head->next;
    }
  }
}

void hashmap_destroy(hashmap *map) {
  if (!map)
    return;

  for (size_t i = 0; i < map->allocatedSize; i++) {
    hashnode *node = map->buckets[i];
    hashnode *next = NULL;

    while (node) {
      next = node->next;

      if (map->destroykey) {
        map->destroykey(node->key);
      }
      if (map->destroyval) {
        map->destroyval(node->value);
      }
      free(node);
      node = next;
    }
  }

  free(map->buckets);
  free(map);
}
