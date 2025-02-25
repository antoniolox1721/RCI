/*
 * Named Data Network (NDN) Implementation
 * Computer Networks and Internet - 2024/2025
 * 
 * objects.c - Implementation of object handling functions
 */

#include "objects.h"

/*
 * Add an object
 */
int add_object(char *name) {
    /* Check if the object already exists */
    Object *curr = node.objects;
    while (curr != NULL) {
        if (strcmp(curr->name, name) == 0) {
            return 0;  /* Object already exists */
        }
        curr = curr->next;
    }
    
    /* Create a new object */
    Object *new_object = malloc(sizeof(Object));
    if (new_object == NULL) {
        perror("malloc");
        return -1;
    }
    
    strcpy(new_object->name, name);
    
    /* Add to the list of objects */
    new_object->next = node.objects;
    node.objects = new_object;
    
    return 0;
}

/*
 * Remove an object
 */
int remove_object(char *name) {
    /* Find the object */
    Object *prev = NULL;
    Object *curr = node.objects;
    
    while (curr != NULL) {
        if (strcmp(curr->name, name) == 0) {
            /* Remove from the list */
            if (prev == NULL) {
                node.objects = curr->next;
            } else {
                prev->next = curr->next;
            }
            
            free(curr);
            return 0;
        }
        
        prev = curr;
        curr = curr->next;
    }
    
    return -1;  /* Object not found */
}

/*
 * Add an object to the cache
 */
int add_to_cache(char *name) {
    /* Check if the object already exists in the cache */
    Object *curr = node.cache;
    while (curr != NULL) {
        if (strcmp(curr->name, name) == 0) {
            return 0;  /* Object already in cache */
        }
        curr = curr->next;
    }
    
    /* Check if the cache is full */
    if (node.current_cache_size >= node.cache_size) {
        /* Remove the oldest object (first in the list) */
        if (node.cache != NULL) {
            Object *oldest = node.cache;
            node.cache = oldest->next;
            free(oldest);
            node.current_cache_size--;
        }
    }
    
    /* Create a new cache entry */
    Object *new_object = malloc(sizeof(Object));
    if (new_object == NULL) {
        perror("malloc");
        return -1;
    }
    
    strcpy(new_object->name, name);
    
    /* Add to the list of cached objects */
    new_object->next = node.cache;
    node.cache = new_object;
    node.current_cache_size++;
    
    return 0;
}

/*
 * Find an object by name
 */
int find_object(char *name) {
    /* Search in the list of objects */
    Object *curr = node.objects;
    while (curr != NULL) {
        if (strcmp(curr->name, name) == 0) {
            return 0;  /* Object found */
        }
        curr = curr->next;
    }
    
    return -1;  /* Object not found */
}

/*
 * Find an object in the cache
 */
int find_in_cache(char *name) {
    /* Search in the cache */
    Object *curr = node.cache;
    while (curr != NULL) {
        if (strcmp(curr->name, name) == 0) {
            return 0;  /* Object found in cache */
        }
        curr = curr->next;
    }
    
    return -1;  /* Object not found in cache */
}

/*
 * Add an interest entry
 */
int add_interest_entry(char *name, int interface_id, enum interface_state state) {
    /* Check if the entry already exists */
    InterestEntry *curr = node.interest_table;
    while (curr != NULL) {
        if (strcmp(curr->name, name) == 0) {
            /* Entry exists, update it */
            curr->interface_states[interface_id] = state;
            return 0;
        }
        curr = curr->next;
    }
    
    /* Create a new entry */
    InterestEntry *new_entry = malloc(sizeof(InterestEntry));
    if (new_entry == NULL) {
        perror("malloc");
        return -1;
    }
    
    strcpy(new_entry->name, name);
    
    /* Initialize all interfaces to 0 (no state) */
    for (int i = 0; i < MAX_INTERFACE; i++) {
        new_entry->interface_states[i] = 0;
    }
    
    /* Set the state for the specified interface */
    new_entry->interface_states[interface_id] = state;
    
    /* Add to the list of interest entries */
    new_entry->next = node.interest_table;
    node.interest_table = new_entry;
    
    return 0;
}

/*
 * Update an interest entry
 */
int update_interest_entry(char *name, int interface_id, enum interface_state state) {
    /* Find the entry */
    InterestEntry *curr = node.interest_table;
    while (curr != NULL) {
        if (strcmp(curr->name, name) == 0) {
            /* Update the state for the specified interface */
            curr->interface_states[interface_id] = state;
            return 0;
        }
        curr = curr->next;
    }
    
    return -1;  /* Entry not found */
}

/*
 * Remove an interest entry
 */
int remove_interest_entry(char *name) {
    /* Find the entry */
    InterestEntry *prev = NULL;
    InterestEntry *curr = node.interest_table;
    
    while (curr != NULL) {
        if (strcmp(curr->name, name) == 0) {
            /* Remove from the list */
            if (prev == NULL) {
                node.interest_table = curr->next;
            } else {
                prev->next = curr->next;
            }
            
            free(curr);
            return 0;
        }
        
        prev = curr;
        curr = curr->next;
    }
    
    return -1;  /* Entry not found */
}

/*
 * Trim leading and trailing whitespace from a string
 */
void trim(char *str) {
    char *start = str;
    char *end;
    
    /* Trim leading whitespace */
    while (isspace(*start)) {
        start++;
    }
    
    if (*start == '\0') {
        /* String is all whitespace */
        *str = '\0';
        return;
    }
    
    /* Trim trailing whitespace */
    end = start + strlen(start) - 1;
    while (end > start && isspace(*end)) {
        end--;
    }
    
    /* Null-terminate the trimmed string */
    *(end + 1) = '\0';
    
    /* Move the trimmed string to the beginning of the input string */
    if (start != str) {
        memmove(str, start, (end - start) + 2);
    }
}

/*
 * Check if a name is valid (alphanumeric and up to MAX_OBJECT_NAME characters)
 */
int is_valid_name(char *name) {
    if (name == NULL || strlen(name) == 0 || strlen(name) > MAX_OBJECT_NAME) {
        return 0;
    }
    
    /* Check if all characters are alphanumeric */
    for (int i = 0; name[i]; i++) {
        if (!isalnum(name[i])) {
            return 0;
        }
    }
    
    return 1;
}