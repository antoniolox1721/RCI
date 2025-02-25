/*
 * Named Data Network (NDN) Implementation
 * Computer Networks and Internet - 2024/2025
 * 
 * commands.h - Functions for command handling
 */

#ifndef COMMANDS_H
#define COMMANDS_H

#include "ndn.h"

/* Command handlers */
int cmd_join(char *net);
int cmd_direct_join(char *net, char *connect_ip, char *connect_tcp);
int cmd_create(char *name);
int cmd_delete(char *name);
int cmd_retrieve(char *name);
int cmd_show_topology();
int cmd_show_names();
int cmd_show_interest_table();
int cmd_leave();
int cmd_exit();

#endif /* COMMANDS_H */