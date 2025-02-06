#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include "label_map.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void          free_entry(Entry *e);
static void          free_entries(Entry *e);
static unsigned long hash_function(char *s);
static Entry        *entry_init(char *id, Command *command);

bool label_map_init(LabelMap *map, int capacity) {
    // STUDENT TODO: Initialize the fields of the label map
    map->capacity = capacity;
    map->entries = calloc(capacity, sizeof(Entry*));
    return true;
}

/**
 * @brief Frees the resources for a singular entry.
 *
 * @param e The pointer to the entry to free.
 */
static void free_entry(Entry *e) {
    // STUDENT TODO: Free a *singular* entry
    // Do not free children; see below
    free(e->id);
    free(e);
}

/**
 * @brief Frees the given list of entries.
 *
 * @param e A pointer to the first entry to free.
 */
static void free_entries(Entry *e) {
    // STUDENT TODO: Free an entry and its children
    if (e == NULL)
        return;
    Entry* n = e->next;
    free_entry(e);
    free_entries(n);
}

void label_map_free(LabelMap *map) {
    // STUDENT TODO: Free the entries associated with this hashmap
    // Do not free the pointer itself, as you do not know whether it was allocated on the heap
    for (int i = 0; i < map->capacity; i++) {
        free_entries(map->entries[i]);
    }
    free(map->entries);
}

/**
 * @brief Returns a hash of the specified id.
 *
 * @param s The string to hash.
 * @return The hash of s
 */
static unsigned long hash_function(char *s) {
    unsigned long i = 0;
    for (int j = 0; s[j]; j++) {
        i += s[j];
    }

    return i;
}

/**
 * @brief Initializes the given entry's state.
 *
 * @param id The id associated with this entry.
 * @param command The command associated with this entry.
 * @return True if the entry was initialized successfully, false otherwise.
 */
static Entry *entry_init(char *id, Command *command) {
    // STUDENT TODO: Initialize the entry and its fields
    Entry* temp = malloc(sizeof(Entry));
    temp->id = id;
    temp->command = command;
    temp->next = NULL;
    return temp;
}

bool put_label(LabelMap *map, char *id, Command *command) {
    // STUDENT TODO: Put the given command into the bucket associated with this label
    // It is okay for the command to be null
    Entry* e = entry_init(id, command);
    uint64_t hashcode = hash_function(id) % map->capacity;
    
    Entry* curr = map->entries[hashcode];
    Entry* prev = NULL;
    // maybe need to check for duplicate ids
    while (curr != NULL) {
        if (strcmp(curr->id, id) == 0) {
            e->next = curr->next;
            if (prev == NULL) {
                map->entries[hashcode] = e;
            } else {
                prev->next = e;
            }
            free_entry(curr);
            return true;
        }
        prev = curr;
        curr = curr->next;
    }

    e->next = map->entries[hashcode];
    map->entries[hashcode] = e;
    return true;
}

Entry *get_label(LabelMap *map, char *id) {
    // STUDENT TODO: Retrieve the value associated with this label
    uint64_t hashcode = hash_function(id) % map->capacity;
    if (map->entries[hashcode] == 0) {
        return NULL;
    } else {
        Entry* curr = map->entries[hashcode];
        if (strcmp(curr->id, id) == 0)
            return curr;
        curr = curr->next;
        while (curr != NULL) {
            if (strcmp(curr->id, id) == 0)
                return curr;
            curr = curr->next;
        }
    }
    return NULL;
}