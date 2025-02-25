/*
 * Named Data Network (NDN) Implementation
 * Computer Networks and Internet - 2024/2025
 * 
 * objects.h - Functions for object management
 */

#ifndef OBJECTS_H
#define OBJECTS_H

#include "ndn.h"

/* Object management */
int add_object(char *name);
int remove_object(char *name);
int add_to_cache(char *name);
int find_object(char *name);
int find_in_cache(char *name);

/* Interest table management */
int add_interest_entry(char *name, int interface_id, enum interface_state state);
int update_interest_entry(char *name, int interface_id, enum interface_state state);
int remove_interest_entry(char *name);

/* Utility functions */
void trim(char *str);
int is_valid_name(char *name);

#endif /* OBJECTS_H */