/*
 * Named Data Network (NDN) Implementation
 * Computer Networks and Internet - 2024/2025
 * 
 * network.c - Implementation of network protocol functions
 */

#include "network.h"
#include "objects.h"

/*
 * Handle network events
 */
void handle_network_events() {
    /* Check if there's a new connection on the listening socket */
    if (FD_ISSET(node.listen_fd, &node.read_fds)) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        int new_fd = accept(node.listen_fd, (struct sockaddr *)&client_addr, &addr_len);
        
        if (new_fd == -1) {
            perror("accept");
        } else {
            /* A new connection has been established */
            char client_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
            char client_port[6];
            snprintf(client_port, 6, "%d", ntohs(client_addr.sin_port));
            
            printf("New connection from %s:%s\n", client_ip, client_port);
            
            /* Add the new connection to the list of neighbors */
            /* Initially, we don't know if it's external or internal */
            add_neighbor(client_ip, client_port, new_fd, 0);
            
            /* Update max_fd if needed */
            if (new_fd > node.max_fd) {
                node.max_fd = new_fd;
            }
        }
    }

    /* Check for activity on neighbor sockets */
    Neighbor *curr = node.neighbors;
    while (curr != NULL) {
        Neighbor *next = curr->next;  /* Save next pointer in case current node is removed */
        
        if (FD_ISSET(curr->fd, &node.read_fds)) {
            char buffer[MAX_BUFFER];
            int bytes_received = read(curr->fd, buffer, MAX_BUFFER - 1);
            
            if (bytes_received <= 0) {
                /* Connection closed or error */
                if (bytes_received == 0) {
                    printf("Connection closed by %s:%s\n", curr->ip, curr->port);
                } else {
                    perror("read");
                }
                
                /* Remove the neighbor */
                remove_neighbor(curr->fd);
            } else {
                /* Process received message */
                buffer[bytes_received] = '\0';
                
                /* Parse the message type and content */
                char *msg_type = strtok(buffer, " \n");
                
                if (msg_type != NULL) {
                    if (strcmp(msg_type, "ENTRY") == 0) {
                        char *ip = strtok(NULL, " \n");
                        char *port = strtok(NULL, " \n");
                        if (ip != NULL && port != NULL) {
                            handle_entry_message(curr->fd, ip, port);
                        }
                    } else if (strcmp(msg_type, "SAFE") == 0) {
                        char *ip = strtok(NULL, " \n");
                        char *port = strtok(NULL, " \n");
                        if (ip != NULL && port != NULL) {
                            handle_safe_message(curr->fd, ip, port);
                        }
                    } else if (strcmp(msg_type, "INTEREST") == 0) {
                        char *name = strtok(NULL, "\n");
                        if (name != NULL) {
                            handle_interest_message(curr->fd, name);
                        }
                    } else if (strcmp(msg_type, "OBJECT") == 0) {
                        char *name = strtok(NULL, "\n");
                        if (name != NULL) {
                            handle_object_message(curr->fd, name);
                        }
                    } else if (strcmp(msg_type, "NOOBJECT") == 0) {
                        char *name = strtok(NULL, "\n");
                        if (name != NULL) {
                            handle_noobject_message(curr->fd, name);
                        }
                    } else {
                        printf("Unknown message type: %s\n", msg_type);
                    }
                }
            }
        }
        
        curr = next;
    }
}

/*
 * Send a registration message to the registration server
 */
int send_reg_message(char *net, char *ip, char *port) {
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(59000);  /* Default registration server port */
    inet_pton(AF_INET, DEFAULT_REG_IP, &server_addr.sin_addr);
    
    char message[MAX_BUFFER];
    snprintf(message, MAX_BUFFER, "REG %s %s %s", net, ip, port);
    
    if (sendto(node.reg_fd, message, strlen(message), 0, 
               (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("sendto");
        return -1;
    }
    
    /* Receive and check the response */
    char buffer[MAX_BUFFER];
    socklen_t addr_len = sizeof(server_addr);
    
    int bytes_received = recvfrom(node.reg_fd, buffer, MAX_BUFFER - 1, 0, 
                                 (struct sockaddr *)&server_addr, &addr_len);
    
    if (bytes_received <= 0) {
        perror("recvfrom");
        return -1;
    }
    
    buffer[bytes_received] = '\0';
    
    if (strcmp(buffer, "OKREG") != 0) {
        printf("Unexpected response from registration server: %s\n", buffer);
        return -1;
    }
    
    return 0;
}

/*
 * Send an unregistration message to the registration server
 */
int send_unreg_message(char *net, char *ip, char *port) {
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(59000);  /* Default registration server port */
    inet_pton(AF_INET, DEFAULT_REG_IP, &server_addr.sin_addr);
    
    char message[MAX_BUFFER];
    snprintf(message, MAX_BUFFER, "UNREG %s %s %s", net, ip, port);
    
    if (sendto(node.reg_fd, message, strlen(message), 0, 
               (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("sendto");
        return -1;
    }
    
    /* Receive and check the response */
    char buffer[MAX_BUFFER];
    socklen_t addr_len = sizeof(server_addr);
    
    int bytes_received = recvfrom(node.reg_fd, buffer, MAX_BUFFER - 1, 0, 
                                 (struct sockaddr *)&server_addr, &addr_len);
    
    if (bytes_received <= 0) {
        perror("recvfrom");
        return -1;
    }
    
    buffer[bytes_received] = '\0';
    
    if (strcmp(buffer, "OKUNREG") != 0) {
        printf("Unexpected response from registration server: %s\n", buffer);
        return -1;
    }
    
    return 0;
}

/*
 * Send a nodes request to the registration server
 */
int send_nodes_request(char *net) {
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(59000);  /* Default registration server port */
    inet_pton(AF_INET, DEFAULT_REG_IP, &server_addr.sin_addr);
    
    char message[MAX_BUFFER];
    snprintf(message, MAX_BUFFER, "NODES %s", net);
    
    if (sendto(node.reg_fd, message, strlen(message), 0, 
               (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("sendto");
        return -1;
    }
    
    return 0;
}

/*
 * Process a NODESLIST response from the registration server
 */
int process_nodeslist_response(char *buffer) {
    /* Extract the network ID */
    char *line = strtok(buffer, "\n");
    if (line == NULL) {
        printf("Invalid NODESLIST response: empty\n");
        return -1;
    }
    
    char net[4];
    if (sscanf(line, "NODESLIST %3s", net) != 1) {
        printf("Invalid NODESLIST response: %s\n", line);
        return -1;
    }
    
    /* Extract the list of nodes */
    int node_count = 0;
    char node_ips[100][INET_ADDRSTRLEN];
    char node_ports[100][6];
    
    while ((line = strtok(NULL, "\n")) != NULL && node_count < 100) {
        /* Each line is of the form "IP PORT" */
        if (sscanf(line, "%s %s", node_ips[node_count], node_ports[node_count]) == 2) {
            node_count++;
        }
    }
    
    /* If there are no nodes, create a new network */
    if (node_count == 0) {
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
    
    /* Choose a random node from the list */
    int chosen_index = rand() % node_count;
    
    /* Connect to the chosen node */
    int fd = connect_to_node(node_ips[chosen_index], node_ports[chosen_index]);
    if (fd < 0) {
        printf("Failed to connect to %s:%s\n", node_ips[chosen_index], node_ports[chosen_index]);
        return -1;
    }
    
    /* Set the external neighbor */
    strcpy(node.ext_neighbor_ip, node_ips[chosen_index]);
    strcpy(node.ext_neighbor_port, node_ports[chosen_index]);
    
    /* Add the node as a neighbor */
    add_neighbor(node_ips[chosen_index], node_ports[chosen_index], fd, 1);
    
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
    
    printf("Joined network %s through %s:%s\n", net, node_ips[chosen_index], node_ports[chosen_index]);
    return 0;
}

/*
 * Send an ENTRY message to a node
 */
int send_entry_message(int fd, char *ip, char *port) {
    char message[MAX_BUFFER];
    snprintf(message, MAX_BUFFER, "ENTRY %s %s\n", ip, port);
    
    if (write(fd, message, strlen(message)) < 0) {
        perror("write");
        return -1;
    }
    
    return 0;
}

/*
 * Send a SAFE message to a node
 */
int send_safe_message(int fd, char *ip, char *port) {
    char message[MAX_BUFFER];
    snprintf(message, MAX_BUFFER, "SAFE %s %s\n", ip, port);
    
    if (write(fd, message, strlen(message)) < 0) {
        perror("write");
        return -1;
    }
    
    return 0;
}

/*
 * Handle an ENTRY message from a node
 */
int handle_entry_message(int fd, char *ip, char *port) {
    /* Add the node as an internal neighbor */
    add_neighbor(ip, port, fd, 0);
    
    /* Send SAFE message with external neighbor info */
    if (send_safe_message(fd, node.ext_neighbor_ip, node.ext_neighbor_port) < 0) {
        printf("Failed to send SAFE message.\n");
        return -1;
    }
    
    return 0;
}

/*
 * Handle a SAFE message from a node
 */
int handle_safe_message(int fd, char *ip, char *port) {
    /* Update the safety node */
    strcpy(node.safe_node_ip, ip);
    strcpy(node.safe_node_port, port);
    
    return 0;
}

/*
 * Send an interest message for an object
 */
int send_interest_message(char *name) {
    char message[MAX_BUFFER];
    snprintf(message, MAX_BUFFER, "INTEREST %s\n", name);
    
    /* Send to all neighbors */
    Neighbor *curr = node.neighbors;
    int sent_count = 0;
    
    while (curr != NULL) {
        if (write(curr->fd, message, strlen(message)) < 0) {
            perror("write");
            /* Continue with other neighbors */
        } else {
            /* Add to interest table */
            add_interest_entry(name, curr->interface_id, WAITING);
            sent_count++;
        }
        
        curr = curr->next;
    }
    
    if (sent_count == 0) {
        /* No neighbors to send to */
        printf("No neighbors to send interest message to.\n");
        return -1;
    }
    
    return 0;
}

/*
 * Send an object message to a node
 */
int send_object_message(int fd, char *name) {
    char message[MAX_BUFFER];
    snprintf(message, MAX_BUFFER, "OBJECT %s\n", name);
    
    if (write(fd, message, strlen(message)) < 0) {
        perror("write");
        return -1;
    }
    
    return 0;
}

/*
 * Send a no-object message to a node
 */
int send_noobject_message(int fd, char *name) {
    char message[MAX_BUFFER];
    snprintf(message, MAX_BUFFER, "NOOBJECT %s\n", name);
    
    if (write(fd, message, strlen(message)) < 0) {
        perror("write");
        return -1;
    }
    
    return 0;
}

/*
 * Handle an interest message from a node
 */
int handle_interest_message(int fd, char *name) {
    /* Check if we have the object */
    if (find_object(name) >= 0 || find_in_cache(name) >= 0) {
        /* We have the object, send it */
        return send_object_message(fd, name);
    }
    
    /* Get the interface ID for this file descriptor */
    int interface_id = -1;
    Neighbor *curr = node.neighbors;
    while (curr != NULL) {
        if (curr->fd == fd) {
            interface_id = curr->interface_id;
            break;
        }
        curr = curr->next;
    }
    
    if (interface_id < 0) {
        printf("Interface ID not found for fd %d\n", fd);
        return -1;
    }
    
    /* Check if we already have an interest entry for this object */
    InterestEntry *entry = node.interest_table;
    while (entry != NULL) {
        if (strcmp(entry->name, name) == 0) {
            /* Update the entry */
            entry->interface_states[interface_id] = RESPONSE;
            
            /* Check if there are any interfaces in WAITING state */
            int waiting_count = 0;
            for (int i = 0; i < MAX_INTERFACE; i++) {
                if (entry->interface_states[i] == WAITING) {
                    waiting_count++;
                }
            }
            
            if (waiting_count == 0) {
                /* No interfaces in WAITING state, send NOOBJECT message */
                for (int i = 0; i < MAX_INTERFACE; i++) {
                    if (entry->interface_states[i] == RESPONSE) {
                        /* Find the neighbor with this interface ID */
                        Neighbor *n = node.neighbors;
                        while (n != NULL) {
                            if (n->interface_id == i) {
                                send_noobject_message(n->fd, name);
                                break;
                            }
                            n = n->next;
                        }
                    }
                }
            }
            
            return 0;
        }
        entry = entry->next;
    }
    
    /* No existing entry, create one and forward the interest */
    curr = node.neighbors;
    int interest_sent = 0;
    
    while (curr != NULL) {
        if (curr->fd != fd) {
            /* Send interest message to other neighbors */
            char message[MAX_BUFFER];
            snprintf(message, MAX_BUFFER, "INTEREST %s\n", name);
            
            if (write(curr->fd, message, strlen(message)) < 0) {
                perror("write");
                /* Continue with other neighbors */
            } else {
                /* Add to interest table */
                add_interest_entry(name, curr->interface_id, WAITING);
                
                /* Mark the original interface as RESPONSE */
                update_interest_entry(name, interface_id, RESPONSE);
                
                interest_sent = 1;
            }
        }
        curr = curr->next;
    }
    
    if (!interest_sent) {
        /* No other neighbors to forward to, send NOOBJECT */
        return send_noobject_message(fd, name);
    }
    
    return 0;
}

/*
 * Handle an object message from a node
 */
int handle_object_message(int fd, char *name) {
    /* Add the object to the cache */
    add_to_cache(name);
    
    /* Forward the object to interfaces in RESPONSE state */
    InterestEntry *entry = node.interest_table;
    while (entry != NULL) {
        if (strcmp(entry->name, name) == 0) {
            for (int i = 0; i < MAX_INTERFACE; i++) {
                if (entry->interface_states[i] == RESPONSE) {
                    /* Find the neighbor with this interface ID */
                    Neighbor *n = node.neighbors;
                    while (n != NULL) {
                        if (n->interface_id == i) {
                            send_object_message(n->fd, name);
                            break;
                        }
                        n = n->next;
                    }
                }
            }
            
            /* Remove the interest entry */
            remove_interest_entry(name);
            
            return 0;
        }
        entry = entry->next;
    }
    
    /* No interest entry found, just cache the object */
    printf("Received object %s but no matching interest entry found\n", name);
    return 0;
}

/*
 * Handle a no-object message from a node
 */
int handle_noobject_message(int fd, char *name) {
    /* Get the interface ID for this file descriptor */
    int interface_id = -1;
    Neighbor *curr = node.neighbors;
    while (curr != NULL) {
        if (curr->fd == fd) {
            interface_id = curr->interface_id;
            break;
        }
        curr = curr->next;
    }
    
    if (interface_id < 0) {
        printf("Interface ID not found for fd %d\n", fd);
        return -1;
    }
    
    /* Update the interest entry */
    InterestEntry *entry = node.interest_table;
    while (entry != NULL) {
        if (strcmp(entry->name, name) == 0) {
            /* Update the entry */
            entry->interface_states[interface_id] = CLOSED;
            
            /* Check if there are any interfaces in WAITING state */
            int waiting_count = 0;
            for (int i = 0; i < MAX_INTERFACE; i++) {
                if (entry->interface_states[i] == WAITING) {
                    waiting_count++;
                }
            }
            
            if (waiting_count == 0) {
                /* No interfaces in WAITING state, send NOOBJECT message to RESPONSE interfaces */
                for (int i = 0; i < MAX_INTERFACE; i++) {
                    if (entry->interface_states[i] == RESPONSE) {
                        /* Find the neighbor with this interface ID */
                        Neighbor *n = node.neighbors;
                        while (n != NULL) {
                            if (n->interface_id == i) {
                                send_noobject_message(n->fd, name);
                                break;
                            }
                            n = n->next;
                        }
                    }
                }
                
                /* If no RESPONSE interfaces, remove the entry */
                int response_count = 0;
                for (int i = 0; i < MAX_INTERFACE; i++) {
                    if (entry->interface_states[i] == RESPONSE) {
                        response_count++;
                    }
                }
                
                if (response_count == 0) {
                    remove_interest_entry(name);
                }
            }
            
            return 0;
        }
        entry = entry->next;
    }
    
    printf("Received NOOBJECT message but no matching interest entry found\n");
    return 0;
}

/*
 * Connect to a node
 */
int connect_to_node(char *ip, char *port) {
    struct addrinfo hints, *res;
    int fd, errcode;
    
    /* Create socket */
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        perror("socket");
        return -1;
    }
    
    /* Connect to the specified node */
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    
    if ((errcode = getaddrinfo(ip, port, &hints, &res)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(errcode));
        close(fd);
        return -1;
    }
    
    if (connect(fd, res->ai_addr, res->ai_addrlen) == -1) {
        perror("connect");
        freeaddrinfo(res);
        close(fd);
        return -1;
    }
    
    freeaddrinfo(res);
    
    /* Update max_fd if needed */
    if (fd > node.max_fd) {
        node.max_fd = fd;
    }
    
    return fd;
}

/*
 * Add a neighbor
 */
int add_neighbor(char *ip, char *port, int fd, int is_external) {
    /* Create a new neighbor */
    Neighbor *new_neighbor = malloc(sizeof(Neighbor));
    if (new_neighbor == NULL) {
        perror("malloc");
        return -1;
    }
    
    strcpy(new_neighbor->ip, ip);
    strcpy(new_neighbor->port, port);
    new_neighbor->fd = fd;
    
    /* Assign an interface ID */
    static int next_interface_id = 0;
    new_neighbor->interface_id = next_interface_id++;
    
    /* Add to the list of neighbors */
    new_neighbor->next = node.neighbors;
    node.neighbors = new_neighbor;
    
    /* If it's not an external neighbor, add to the list of internal neighbors */
    if (!is_external) {
        Neighbor *internal_neighbor = malloc(sizeof(Neighbor));
        if (internal_neighbor == NULL) {
            perror("malloc");
            return -1;
        }
        
        strcpy(internal_neighbor->ip, ip);
        strcpy(internal_neighbor->port, port);
        internal_neighbor->fd = fd;
        internal_neighbor->interface_id = new_neighbor->interface_id;
        
        internal_neighbor->next = node.internal_neighbors;
        node.internal_neighbors = internal_neighbor;
    }
    
    return 0;
}

/*
 * Remove a neighbor
 */
int remove_neighbor(int fd) {
    /* Find the neighbor */
    Neighbor *prev = NULL;
    Neighbor *curr = node.neighbors;
    
    while (curr != NULL) {
        if (curr->fd == fd) {
            /* Check if it's the external neighbor */
            if (strcmp(curr->ip, node.ext_neighbor_ip) == 0 && 
                strcmp(curr->port, node.ext_neighbor_port) == 0) {
                /* External neighbor is gone, handle according to the protocol */
                if (strcmp(node.safe_node_ip, node.ip) != 0) {
                    /* Connect to the safety node */
                    int new_fd = connect_to_node(node.safe_node_ip, node.safe_node_port);
                    if (new_fd < 0) {
                        printf("Failed to connect to safety node %s:%s\n", 
                               node.safe_node_ip, node.safe_node_port);
                        return -1;
                    }
                    
                    /* Set as new external neighbor */
                    strcpy(node.ext_neighbor_ip, node.safe_node_ip);
                    strcpy(node.ext_neighbor_port, node.safe_node_port);
                    
                    /* Add as neighbor */
                    add_neighbor(node.safe_node_ip, node.safe_node_port, new_fd, 1);
                    
                    /* Send ENTRY message */
                    if (send_entry_message(new_fd, node.ip, node.port) < 0) {
                        printf("Failed to send ENTRY message to new external neighbor.\n");
                        return -1;
                    }
                    
                    /* Send SAFE message to all internal neighbors */
                    Neighbor *n = node.internal_neighbors;
                    while (n != NULL) {
                        send_safe_message(n->fd, node.ext_neighbor_ip, node.ext_neighbor_port);
                        n = n->next;
                    }
                } else if (node.internal_neighbors != NULL) {
                    /* Self is safety node but has internal neighbors */
                    /* Choose a random internal neighbor as new external neighbor */
                    Neighbor *n = node.internal_neighbors;
                    int count = 0;
                    Neighbor *chosen = NULL;
                    
                    /* Count internal neighbors */
                    while (n != NULL) {
                        count++;
                        n = n->next;
                    }
                    
                    /* Choose a random one */
                    int chosen_index = rand() % count;
                    n = node.internal_neighbors;
                    for (int i = 0; i < chosen_index; i++) {
                        n = n->next;
                    }
                    chosen = n;
                    
                    /* Set as new external neighbor */
                    strcpy(node.ext_neighbor_ip, chosen->ip);
                    strcpy(node.ext_neighbor_port, chosen->port);
                    
                    /* Send ENTRY message */
                    if (send_entry_message(chosen->fd, node.ip, node.port) < 0) {
                        printf("Failed to send ENTRY message to new external neighbor.\n");
                        return -1;
                    }
                    
                    /* Send SAFE message to all internal neighbors */
                    n = node.internal_neighbors;
                    while (n != NULL) {
                        if (n != chosen) {
                            send_safe_message(n->fd, node.ext_neighbor_ip, node.ext_neighbor_port);
                        }
                        n = n->next;
                    }
                } else {
                    /* Self is safety node and has no internal neighbors */
                    /* Become a standalone node */
                    strcpy(node.ext_neighbor_ip, node.ip);
                    strcpy(node.ext_neighbor_port, node.port);
                    strcpy(node.safe_node_ip, node.ip);
                    strcpy(node.safe_node_port, node.port);
                }
            }
            
            /* Remove from the list of neighbors */
            if (prev == NULL) {
                node.neighbors = curr->next;
            } else {
                prev->next = curr->next;
            }
            
            /* Also remove from the list of internal neighbors if it's there */
            Neighbor *prev_internal = NULL;
            Neighbor *curr_internal = node.internal_neighbors;
            
            while (curr_internal != NULL) {
                if (curr_internal->fd == fd) {
                    if (prev_internal == NULL) {
                        node.internal_neighbors = curr_internal->next;
                    } else {
                        prev_internal->next = curr_internal->next;
                    }
                    
                    free(curr_internal);
                    break;
                }
                
                prev_internal = curr_internal;
                curr_internal = curr_internal->next;
            }
            
            /* Close the socket and free the memory */
            close(curr->fd);
            free(curr);
            
            return 0;
        }
        
        prev = curr;
        curr = curr->next;
    }
    
    return -1;  /* Neighbor not found */
}