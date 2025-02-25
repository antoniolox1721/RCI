/*
 * Named Data Network (NDN) Implementation
 * Computer Networks and Internet - 2024/2025
 * 
 * main.c - Main program file
 */

#include "ndn.h"
#include "commands.h"
#include "network.h"
#include "objects.h"

/* Global node variable */
Node node;

/*
 * Main function
 */
int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Usage: %s cache IP TCP [regIP regUDP]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    /* Extract command-line arguments */
    int cache_size = atoi(argv[1]);
    char *ip = argv[2];
    char *port = argv[3];
    char *reg_ip = (argc > 4) ? argv[4] : DEFAULT_REG_IP;
    int reg_udp = (argc > 5) ? atoi(argv[5]) : DEFAULT_REG_UDP;

    /* Set up signal handler for graceful termination */
    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_handler = handle_sigint;
    if (sigaction(SIGINT, &act, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    /* Ignore SIGPIPE to prevent the program from terminating when writing to closed sockets */
    memset(&act, 0, sizeof(act));
    act.sa_handler = SIG_IGN;
    if (sigaction(SIGPIPE, &act, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    /* Initialize the node */
    initialize_node(cache_size, ip, port, reg_ip, reg_udp);

    /* Main loop */
    while (1) {
        /* Prepare the file descriptor set for select() */
        FD_ZERO(&node.read_fds);
        FD_SET(STDIN_FILENO, &node.read_fds);  /* Add stdin for user input */
        FD_SET(node.listen_fd, &node.read_fds); /* Add listening socket */
        
        /* Add all neighbor sockets */
        Neighbor *curr = node.neighbors;
        while (curr != NULL) {
            FD_SET(curr->fd, &node.read_fds);
            curr = curr->next;
        }

        /* Wait for activity on one of the file descriptors */
        if (select(node.max_fd + 1, &node.read_fds, NULL, NULL, NULL) == -1) {
            if (errno == EINTR) {
                /* Interrupted by signal, continue the loop */
                continue;
            } else {
                perror("select");
                break;
            }
        }

        /* Check for user input */
        if (FD_ISSET(STDIN_FILENO, &node.read_fds)) {
            handle_user_input();
        }

        /* Check for network events */
        handle_network_events();
    }

    /* Cleanup and exit */
    cleanup_and_exit();
    return EXIT_SUCCESS;
}

/*
 * Signal handler for SIGINT
 */
void handle_sigint(int sig) {
    printf("\nReceived SIGINT, cleaning up and exiting...\n");
    cleanup_and_exit();
    exit(EXIT_SUCCESS);
}

/*
 * Handle user input
 */
void handle_user_input() {
    char cmd_buffer[MAX_CMD_SIZE];
    
    if (fgets(cmd_buffer, MAX_CMD_SIZE, stdin) == NULL) {
        if (feof(stdin)) {
            /* End of file, exit gracefully */
            cleanup_and_exit();
            exit(EXIT_SUCCESS);
        } else {
            perror("fgets");
            return;
        }
    }

    /* Remove trailing newline */
    cmd_buffer[strcspn(cmd_buffer, "\n")] = '\0';
    
    /* Process the command */
    if (process_command(cmd_buffer) < 0) {
        printf("Error processing command: %s\n", cmd_buffer);
    }
}

/*
 * Initialize the node
 */
void initialize_node(int cache_size, char *ip, char *port, char *reg_ip, int reg_udp) {
    struct addrinfo hints, *res;
    int errcode;

    /* Initialize node structure */
    memset(&node, 0, sizeof(Node));
    node.cache_size = cache_size;
    strcpy(node.ip, ip);
    strcpy(node.port, port);
    strcpy(node.ext_neighbor_ip, ip);      /* Initially, external neighbor is self */
    strcpy(node.ext_neighbor_port, port);
    node.in_network = 0;                  /* Not in a network initially */
    node.max_fd = 0;

    /* Create TCP listening socket */
    node.listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (node.listen_fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    /* Allow address reuse */
    int reuse = 1;
    if (setsockopt(node.listen_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == -1) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    /* Bind the socket to the specified IP and port */
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if ((errcode = getaddrinfo(NULL, port, &hints, &res)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(errcode));
        exit(EXIT_FAILURE);
    }

    if (bind(node.listen_fd, res->ai_addr, res->ai_addrlen) == -1) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    /* Start listening for connections */
    if (listen(node.listen_fd, 5) == -1) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    /* Update max_fd */
    node.max_fd = node.listen_fd;

    /* Create UDP socket for registration server communication */
    node.reg_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (node.reg_fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    /* Update max_fd if needed */
    if (node.reg_fd > node.max_fd) {
        node.max_fd = node.reg_fd;
    }

    freeaddrinfo(res);
    printf("Node initialized with IP %s and port %s\n", ip, port);
    printf("Enter 'help' for a list of commands\n");
}

/*
 * Clean up resources and exit
 */
void cleanup_and_exit() {
    /* If in a network, leave it first */
    if (node.in_network) {
        cmd_leave();
    }
    
    /* Close all sockets */
    if (node.listen_fd > 0) {
        close(node.listen_fd);
    }
    
    if (node.reg_fd > 0) {
        close(node.reg_fd);
    }
    
    /* Close and free all neighbor connections */
    Neighbor *curr = node.neighbors;
    while (curr != NULL) {
        Neighbor *next = curr->next;
        close(curr->fd);
        free(curr);
        curr = next;
    }
    
    /* Free all objects */
    Object *obj = node.objects;
    while (obj != NULL) {
        Object *next = obj->next;
        free(obj);
        obj = next;
    }
    
    /* Free all cached objects */
    obj = node.cache;
    while (obj != NULL) {
        Object *next = obj->next;
        free(obj);
        obj = next;
    }
    
    /* Free all interest table entries */
    InterestEntry *entry = node.interest_table;
    while (entry != NULL) {
        InterestEntry *next = entry->next;
        free(entry);
        entry = next;
    }
}