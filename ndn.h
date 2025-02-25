/*
 * Named Data Network (NDN) Implementation
 * Computer Networks and Internet - 2024/2025
 * 
 * ndn.h - Main header file with common definitions
 */

#ifndef NDN_H
#define NDN_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
#include <ctype.h>

#define MAX_INTERFACE 10       /* Maximum number of interfaces a node can have */
#define MAX_OBJECT_NAME 100    /* Maximum length of object names */
#define MAX_CACHE_SIZE 100     /* Default maximum cache size */
#define MAX_BUFFER 1024        /* Maximum buffer size for messages */
#define MAX_CMD_SIZE 128       /* Maximum command size */
#define DEFAULT_REG_IP "193.136.138.142"
#define DEFAULT_REG_UDP 59000

/* State for each interface in the interest table */
enum interface_state {
    RESPONSE,   /* Interface where a response should be sent */
    WAITING,    /* Interface where an interest was sent, awaiting response */
    CLOSED      /* Interface where a NOOBJECT message was received */
};

/* Object structure */
typedef struct object {
    char name[MAX_OBJECT_NAME + 1];
    struct object *next;
} Object;

/* Interest table entry structure */
typedef struct interest_entry {
    char name[MAX_OBJECT_NAME + 1];
    int interface_states[MAX_INTERFACE];
    struct interest_entry *next;
} InterestEntry;

/* Neighbor structure */
typedef struct neighbor {
    char ip[INET_ADDRSTRLEN];
    char port[6];
    int fd;                    /* File descriptor for the connection */
    int interface_id;          /* Interface ID */
    struct neighbor *next;
} Neighbor;

/* Node structure - main data structure for the application */
typedef struct node {
    char ip[INET_ADDRSTRLEN];
    char port[6];
    char ext_neighbor_ip[INET_ADDRSTRLEN];
    char ext_neighbor_port[6];
    char safe_node_ip[INET_ADDRSTRLEN];
    char safe_node_port[6];
    int listen_fd;             /* File descriptor for the listening socket */
    int reg_fd;                /* File descriptor for the registration server socket */
    int max_fd;                /* Maximum file descriptor for select() */
    int cache_size;            /* Maximum cache size */
    int current_cache_size;    /* Current cache size */
    int in_network;            /* 1 if in a network, 0 otherwise */
    int network_id;            /* ID of the network */
    fd_set read_fds;           /* Set of file descriptors for select() */
    Neighbor *neighbors;       /* List of neighbors */
    Neighbor *internal_neighbors; /* List of internal neighbors */
    Object *objects;           /* List of objects */
    Object *cache;             /* List of cached objects */
    InterestEntry *interest_table; /* Interest table */
} Node;

/* Global node variable */
extern Node node;

/* Initialization and cleanup functions */
void initialize_node(int cache_size, char *ip, char *port, char *reg_ip, int reg_udp);
void cleanup_and_exit();
void handle_sigint(int sig);

/* Event handlers */
void handle_user_input();
void handle_network_events();

/* Command processing */
int process_command(char *cmd);
void print_help();

#endif /* NDN_H */