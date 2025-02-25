/*
 * Named Data Network (NDN) Implementation
 * Computer Networks and Internet - 2024/2025
 * 
 * network.h - Functions for network protocols
 */

#ifndef NETWORK_H
#define NETWORK_H

#include "ndn.h"

/* Registration protocol */
int send_reg_message(char *net, char *ip, char *port);
int send_unreg_message(char *net, char *ip, char *port);
int send_nodes_request(char *net);
int process_nodeslist_response(char *buffer);

/* Topology protocol */
int send_entry_message(int fd, char *ip, char *port);
int send_safe_message(int fd, char *ip, char *port);
int handle_entry_message(int fd, char *ip, char *port);
int handle_safe_message(int fd, char *ip, char *port);

/* NDN protocol */
int send_interest_message(char *name);
int send_object_message(int fd, char *name);
int send_noobject_message(int fd, char *name);
int handle_interest_message(int fd, char *name);
int handle_object_message(int fd, char *name);
int handle_noobject_message(int fd, char *name);

/* Network utilities */
int connect_to_node(char *ip, char *port);
int add_neighbor(char *ip, char *port, int fd, int is_external);
int remove_neighbor(int fd);

#endif /* NETWORK_H */