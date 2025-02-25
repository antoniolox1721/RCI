/*
 * Named Data Network (NDN) Implementation
 * Computer Networks and Internet - 2024/2025
 * 
 * commands.c - Implementation of command functions
 */

#include "commands.h"
#include "network.h"
#include "objects.h"

/*
 * Process a user command
 */
int process_command(char *cmd) {
    char *cmd_name = strtok(cmd, " ");
    
    if (cmd_name == NULL) {
        return 0;  /* Empty command */
    }
    
    /* Convert to lowercase for case-insensitive comparison */
    for (int i = 0; cmd_name[i]; i++) {
        cmd_name[i] = tolower(cmd_name[i]);
    }
    
    if (strcmp(cmd_name, "join") == 0 || strcmp(cmd_name, "j") == 0) {
        char *net = strtok(NULL, " ");
        if (net != NULL) {
            return cmd_join(net);
        } else {
            printf("Usage: join (j) <net>\n");
        }
    } else if (strcmp(cmd_name, "direct") == 0 || strcmp(cmd_name, "dj") == 0) {
        char *net = strtok(NULL, " ");
        char *connect_ip = strtok(NULL, " ");
        char *connect_tcp = strtok(NULL, " ");
        if (net != NULL && connect_ip != NULL && connect_tcp != NULL) {
            return cmd_direct_join(net, connect_ip, connect_tcp);
        } else {
            printf("Usage: direct join (dj) <net> <connectIP> <connectTCP>\n");
        }
    } else if (strcmp(cmd_name, "create") == 0 || strcmp(cmd_name, "c") == 0) {
        char *name = strtok(NULL, " ");
        if (name != NULL) {
            return cmd_create(name);
        } else {
            printf("Usage: create (c) <name>\n");
        }
    } else if (strcmp(cmd_name, "delete") == 0 || strcmp(cmd_name, "dl") == 0) {
        char *name = strtok(NULL, " ");
        if (name != NULL) {
            return cmd_delete(name);
        } else {
            printf("Usage: delete (dl) <name>\n");
        }
    } else if (strcmp(cmd_name, "retrieve") == 0 || strcmp(cmd_name, "r") == 0) {
        char *name = strtok(NULL, " ");
        if (name != NULL) {
            return cmd_retrieve(name);
        } else {
            printf("Usage: retrieve (r) <name>\n");
        }
    } else if (strcmp(cmd_name, "show") == 0 || strcmp(cmd_name, "s") == 0) {
        char *what = strtok(NULL, " ");
        if (what != NULL) {
            for (int i = 0; what[i]; i++) {
                what[i] = tolower(what[i]);
            }
            
            if (strcmp(what, "topology") == 0 || strcmp(what, "st") == 0) {
                return cmd_show_topology();
            } else if (strcmp(what, "names") == 0 || strcmp(what, "sn") == 0) {
                return cmd_show_names();
            } else if (strcmp(what, "interest") == 0 || strcmp(what, "si") == 0) {
                return cmd_show_interest_table();
            } else {
                printf("Unknown show command: %s\n", what);
            }
        } else {
            printf("Usage: show <topology|names|interest>\n");
        }
    } else if (strcmp(cmd_name, "leave") == 0 || strcmp(cmd_name, "l") == 0) {
        return cmd_leave();
    } else if (strcmp(cmd_name, "exit") == 0 || strcmp(cmd_name, "x") == 0) {
        return cmd_exit();
    } else if (strcmp(cmd_name, "help") == 0 || strcmp(cmd_name, "h") == 0) {
        print_help();
        return 0;
    } else {
        printf("Unknown command: %s\n", cmd_name);
    }
    
    return -1;
}

/*
 * Print help information
 */
void print_help() {
    printf("Available commands:\n");
    printf("  join (j) <net>                        - Join network <net>\n");
    printf("  direct join (dj) <net> <IP> <TCP>     - Join network <net> directly through node <IP>:<TCP>\n");
    printf("  create (c) <name>                     - Create object with name <name>\n");
    printf("  delete (dl) <name>                    - Delete object with name <name>\n");
    printf("  retrieve (r) <name>                   - Retrieve object with name <name>\n");
    printf("  show topology (st)                    - Show network topology\n");
    printf("  show names (sn)                       - Show objects stored in this node\n");
    printf("  show interest table (si)              - Show interest table\n");
    printf("  leave (l)                             - Leave the network\n");
    printf("  exit (x)                              - Exit the application\n");
    printf("  help (h)                              - Show this help message\n");
}

/*
 * Join a network
 */
int cmd_join(char *net) {
    if (node.in_network) {
        printf("Already in a network. Leave first.\n");
        return -1;
    }
    
    /* Check if the network ID is valid (3 digits) */
    if (strlen(net) != 3 || !isdigit(net[0]) || !isdigit(net[1]) || !isdigit(net[2])) {
        printf("Invalid network ID. Must be 3 digits.\n");
        return -1;
    }
    
    /* Request the list of nodes in the network */
    if (send_nodes_request(net) < 0) {
        printf("Failed to send NODES request.\n");
        return -1;
    }
    
    /* Receive and process the NODESLIST response */
    char buffer[MAX_BUFFER];
    struct sockaddr_in server_addr;
    socklen_t addr_len = sizeof(server_addr);
    
    int bytes_received = recvfrom(node.reg_fd, buffer, MAX_BUFFER - 1, 0, 
                                 (struct sockaddr *)&server_addr, &addr_len);
    
    if (bytes_received <= 0) {
        printf("Failed to receive NODESLIST response.\n");
        return -1;
    }
    
    buffer[bytes_received] = '\0';
    
    /* Process the NODESLIST response */
    if (process_nodeslist_response(buffer) < 0) {
        printf("Failed to process NODESLIST response.\n");
        return -1;
    }
    
    /* Register with the network */
    node.network_id = atoi(net);
    node.in_network = 1;
    printf("Joined network %s\n", net);
    
    return 0;
}

/*
 * Join a network directly
 */
int cmd_direct_join(char *net, char *connect_ip, char *connect_tcp) {
    if (node.in_network) {
        printf("Already in a network. Leave first.\n");
        return -1;
    }
    
    /* Check if the network ID is valid (3 digits) */
    if (strlen(net) != 3 || !isdigit(net[0]) || !isdigit(net[1]) || !isdigit(net[2])) {
        printf("Invalid network ID. Must be 3 digits.\n");
        return -1;
    }
    
    /* If connect_ip is 0.0.0.0, create a new network */
    if (strcmp(connect_ip, "0.0.0.0") == 0) {
        /* Register with the network */
        if (send_reg_message(net, node.ip, node.port) < 0) {
            printf("Failed to register with the network.\n");
            return -1;
        }
        
        /* Set the network ID and mark as in_network */
        node.network_id = atoi(net);
        node.in_network = 1;
        
        /* This node is its own external neighbor and safety node */
        strcpy(node.ext_neighbor_ip, node.ip);
        strcpy(node.ext_neighbor_port, node.port);
        strcpy(node.safe_node_ip, node.ip);
        strcpy(node.safe_node_port, node.port);
        
        printf("Created and joined network %s\n", net);
        return 0;
    }
    
    /* Connect to the specified node */
    int fd = connect_to_node(connect_ip, connect_tcp);
    if (fd < 0) {
        printf("Failed to connect to %s:%s\n", connect_ip, connect_tcp);
        return -1;
    }
    
    /* Set the external neighbor */
    strcpy(node.ext_neighbor_ip, connect_ip);
    strcpy(node.ext_neighbor_port, connect_tcp);
    
    /* Add the node as a neighbor */
    add_neighbor(connect_ip, connect_tcp, fd, 1);
    
    /* Send ENTRY message */
    if (send_entry_message(fd, node.ip, node.port) < 0) {
        printf("Failed to send ENTRY message.\n");
        close(fd);
        return -1;
    }
    
    /* Register with the network */
    if (send_reg_message(net, node.ip, node.port) < 0) {
        printf("Failed to register with the network.\n");
        close(fd);
        return -1;
    }
    
    /* Set the network ID and mark as in_network */
    node.network_id = atoi(net);
    node.in_network = 1;
    
    printf("Joined network %s through %s:%s\n", net, connect_ip, connect_tcp);
    return 0;
}

/*
 * Create an object
 */
int cmd_create(char *name) {
    if (!is_valid_name(name)) {
        printf("Invalid object name. Must be alphanumeric and up to %d characters.\n", MAX_OBJECT_NAME);
        return -1;
    }
    
    if (add_object(name) < 0) {
        printf("Failed to create object %s\n", name);
        return -1;
    }
    
    printf("Created object %s\n", name);
    return 0;
}

/*
 * Delete an object
 */
int cmd_delete(char *name) {
    if (!is_valid_name(name)) {
        printf("Invalid object name. Must be alphanumeric and up to %d characters.\n", MAX_OBJECT_NAME);
        return -1;
    }
    
    if (remove_object(name) < 0) {
        printf("Object %s not found\n", name);
        return -1;
    }
    
    printf("Deleted object %s\n", name);
    return 0;
}

/*
 * Retrieve an object
 */
int cmd_retrieve(char *name) {
    if (!is_valid_name(name)) {
        printf("Invalid object name. Must be alphanumeric and up to %d characters.\n", MAX_OBJECT_NAME);
        return -1;
    }
    
    /* Check if the object exists locally */
    if (find_object(name) >= 0) {
        printf("Object %s found locally\n", name);
        return 0;
    }
    
    /* Check if the object exists in the cache */
    if (find_in_cache(name) >= 0) {
        printf("Object %s found in cache\n", name);
        return 0;
    }
    
    /* Object not found locally, send interest message to all neighbors */
    if (send_interest_message(name) < 0) {
        printf("Failed to send interest message for %s\n", name);
        return -1;
    }
    
    printf("Sent interest for object %s\n", name);
    return 0;
}

/*
 * Show the network topology
 */
int cmd_show_topology() {
    printf("Node: %s:%s\n", node.ip, node.port);
    printf("External neighbor: %s:%s\n", node.ext_neighbor_ip, node.ext_neighbor_port);
    printf("Safety node: %s:%s\n", node.safe_node_ip, node.safe_node_port);
    
    printf("Internal neighbors:\n");
    Neighbor *curr = node.internal_neighbors;
    while (curr != NULL) {
        printf("  %s:%s\n", curr->ip, curr->port);
        curr = curr->next;
    }
    
    return 0;
}

/*
 * Show stored object names
 */
int cmd_show_names() {
    printf("Objects:\n");
    Object *obj = node.objects;
    while (obj != NULL) {
        printf("  %s\n", obj->name);
        obj = obj->next;
    }
    
    printf("Cache:\n");
    obj = node.cache;
    while (obj != NULL) {
        printf("  %s\n", obj->name);
        obj = obj->next;
    }
    
    return 0;
}

/*
 * Show the interest table
 */
int cmd_show_interest_table() {
    printf("Interest table:\n");
    InterestEntry *entry = node.interest_table;
    while (entry != NULL) {
        printf("  %s:", entry->name);
        for (int i = 0; i < MAX_INTERFACE; i++) {
            if (entry->interface_states[i] == RESPONSE) {
                printf(" %d:response", i);
            } else if (entry->interface_states[i] == WAITING) {
                printf(" %d:waiting", i);
            } else if (entry->interface_states[i] == CLOSED) {
                printf(" %d:closed", i);
            }
        }
        printf("\n");
        entry = entry->next;
    }
    
    return 0;
}

/*
 * Leave the network
 */
int cmd_leave() {
    if (!node.in_network) {
        printf("Not in a network.\n");
        return -1;
    }
    
    /* Unregister from the network */
    char net_str[4];
    snprintf(net_str, 4, "%03d", node.network_id);
    
    if (send_unreg_message(net_str, node.ip, node.port) < 0) {
        printf("Failed to unregister from the network.\n");
        return -1;
    }
    
    /* Close all neighbor connections */
    Neighbor *curr = node.neighbors;
    while (curr != NULL) {
        Neighbor *next = curr->next;
        close(curr->fd);
        free(curr);
        curr = next;
    }
    
    /* Reset node state */
    node.neighbors = NULL;
    node.internal_neighbors = NULL;
    strcpy(node.ext_neighbor_ip, node.ip);
    strcpy(node.ext_neighbor_port, node.port);
    memset(node.safe_node_ip, 0, INET_ADDRSTRLEN);
    memset(node.safe_node_port, 0, 6);
    node.in_network = 0;
    
    printf("Left network %03d\n", node.network_id);
    return 0;
}

/*
 * Exit the application
 */
int cmd_exit() {
    /* If in a network, leave it first */
    if (node.in_network) {
        cmd_leave();
    }
    
    /* Clean up and exit */
    cleanup_and_exit();
    exit(EXIT_SUCCESS);
    
    return 0;  /* Never reached */
}