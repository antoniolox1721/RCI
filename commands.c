/*
 * Implementação de Rede de Dados Identificados por Nome (NDN)
 * Redes de Computadores e Internet - 2024/2025
 *
 * commands.c - Implementação das funções de comandos
 */

#include "commands.h"
#include "network.h"
#include "objects.h"
#include "debug_utils.h"
#include "ndn.h"
/**
 * Processa um comando do utilizador.
 *
 * @param cmd String contendo o comando a processar
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int process_command(char *cmd) {
    char original_cmd[MAX_CMD_SIZE];
    strncpy(original_cmd, cmd, MAX_CMD_SIZE - 1);
    original_cmd[MAX_CMD_SIZE - 1] = '\0';

    /* Skip leading whitespace */
    while (*cmd && isspace(*cmd)) {
        cmd++;
    }

    if (*cmd == '\0') {
        return 0; /* Empty command */
    }

    /* Extract command name (first word) */
    char cmd_name[MAX_CMD_SIZE];
    int i = 0;
    while (cmd[i] && !isspace(cmd[i]) && i < MAX_CMD_SIZE - 1) {
        cmd_name[i] = tolower(cmd[i]); /* Convert to lowercase */
        i++;
    }
    cmd_name[i] = '\0';

    /* Advance to parameters (after the command and spaces) */
    char *params = cmd + i;
    while (*params && isspace(*params)) {
        params++;
    }

    /* Check for commands that need special handling for multi-word arguments */
    if (strcmp(cmd_name, "retrieve") == 0 || strcmp(cmd_name, "r") == 0 ||
        strcmp(cmd_name, "create") == 0 || strcmp(cmd_name, "c") == 0 ||
        strcmp(cmd_name, "delete") == 0 || strcmp(cmd_name, "dl") == 0) {
        
        /* For these commands, we need to ensure the arguments don't contain spaces */
        char object_name[MAX_OBJECT_NAME + 1] = {0};
        
        /* Extract the first word as the object name */
        i = 0;
        while (params[i] && !isspace(params[i]) && i < MAX_OBJECT_NAME) {
            object_name[i] = params[i];
            i++;
        }
        object_name[i] = '\0';
        
        /* Check if there are more words after the object name */
        char *next_param = params + i;
        while (*next_param && isspace(*next_param)) {
            next_param++;
        }
        
        /* If there are more words, display an error */
        if (*next_param) {
            printf("%sError: Object name cannot contain spaces. Found: \"%s %s\"%s\n", 
                   COLOR_RED, object_name, next_param, COLOR_RESET);
            return -1;
        }
        
        /* Process the command with the validated object name */
        if (strcmp(cmd_name, "retrieve") == 0 || strcmp(cmd_name, "r") == 0) {
            if (*object_name) {
                return cmd_retrieve(object_name);
            } else {
                printf("%sUsage: retrieve (r) <name>%s\n", COLOR_RED, COLOR_RESET);
                return -1;
            }
        } else if (strcmp(cmd_name, "create") == 0 || strcmp(cmd_name, "c") == 0) {
            if (*object_name) {
                return cmd_create(object_name);
            } else {
                printf("%sUsage: create (c) <name>%s\n", COLOR_RED, COLOR_RESET);
                return -1;
            }
        } else if (strcmp(cmd_name, "delete") == 0 || strcmp(cmd_name, "dl") == 0) {
            if (*object_name) {
                return cmd_delete(object_name);
            } else {
                printf("%sUsage: delete (dl) <name>%s\n", COLOR_RED, COLOR_RESET);
                return -1;
            }
        }
    }

    /* Handle other commands using strtok for parsing */
    char *token = strtok(cmd, " \n");
    /* Skip the command name token, already processed above */
    token = strtok(NULL, " \n");

    if (strcmp(cmd_name, "join") == 0 || strcmp(cmd_name, "j") == 0) {
        if (token != NULL) {
            return cmd_join(token);
        } else {
            printf("%sUsage: join (j) <net>%s\n", COLOR_RED, COLOR_RESET);
            return -1;
        }
    } else if (strcmp(cmd_name, "direct") == 0 || strcmp(cmd_name, "dj") == 0) {
        char *connect_ip = token;                /* First token after command is IP */
        char *connect_tcp = strtok(NULL, " \n"); /* Second token is TCP port */

        if (connect_ip != NULL && connect_tcp != NULL) {
            return cmd_direct_join(connect_ip, connect_tcp);
        } else {
            printf("%sUsage: direct join (dj) <connectIP> <connectTCP>%s\n", COLOR_RED, COLOR_RESET);
            return -1;
        }
    } else if (strcmp(cmd_name, "show") == 0 || strcmp(cmd_name, "s") == 0) {
        if (token != NULL) {
            char what[MAX_CMD_SIZE];
            strncpy(what, token, MAX_CMD_SIZE - 1);
            what[MAX_CMD_SIZE - 1] = '\0';

            /* Convert to lowercase */
            for (int i = 0; what[i]; i++) {
                what[i] = tolower(what[i]);
            }

            if (strcmp(what, "topology") == 0) {
                return cmd_show_topology();
            } else if (strcmp(what, "names") == 0) {
                return cmd_show_names();
            } else if (strcmp(what, "interest") == 0 || strcmp(what, "table") == 0) {
                return cmd_show_interest_table();
            } else {
                printf("%sUnknown show command: %s%s\n", COLOR_RED, what, COLOR_RESET);
                return -1;
            }
        } else {
            printf("%sUsage: show <topology|names|interest>%s\n", COLOR_RED, COLOR_RESET);
            return -1;
        }
    }
    /* Handle direct abbreviations */
    else if (strcmp(cmd_name, "st") == 0) {
        return cmd_show_topology();
    } else if (strcmp(cmd_name, "sn") == 0) {
        return cmd_show_names();
    } else if (strcmp(cmd_name, "si") == 0) {
        return cmd_show_interest_table();
    } else if (strcmp(cmd_name, "leave") == 0 || strcmp(cmd_name, "l") == 0) {
        return cmd_leave();
    } else if (strcmp(cmd_name, "exit") == 0 || strcmp(cmd_name, "x") == 0) {
        return cmd_exit();
    } else if (strcmp(cmd_name, "help") == 0 || strcmp(cmd_name, "h") == 0) {
        print_help();
        return 0;
    } else {
        printf("%sUnknown command: %s%s\n", COLOR_RED, cmd_name, COLOR_RESET);
        return -1;
    }

    return -1;
}

/**
 * Imprime informações de ajuda.
 * Mostra todos os comandos disponíveis e suas descrições.
 */
void print_help()
{
    printf("Available commands:\n");
    printf("  join (j) <net>                        - Join network <net>\n");
    printf("  direct join (dj) <IP> <TCP>           - Join network directly through node <IP>:<TCP>\n");
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

/**
 * Aderir a uma rede através do servidor de registo.
 * Modificado para validar corretamente o ID da rede e processar a resposta.
 *
 * @param net ID da rede (três dígitos)
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int cmd_join(char *net)
{
    if (node.in_network)
    {
        printf("Already in a network. Leave first.\n");
        return -1;
    }

    /* Verifica se o ID da rede é válido (3 dígitos) */
    if (strlen(net) != 3 || !isdigit(net[0]) || !isdigit(net[1]) || !isdigit(net[2]))
    {
        printf("Invalid network ID. Must be exactly 3 digits.\n");
        return -1;
    }

    printf("Attempting to join network %s through registration server %s:%s\n",
           net, node.reg_server_ip, node.reg_server_port);

    /* Solicita a lista de nós na rede */
    if (send_nodes_request(net) < 0)
    {
        printf("Failed to send NODES request.\n");
        return -1;
    }

    /* Recebe e processa a resposta NODESLIST */
    char buffer[MAX_BUFFER];
    struct sockaddr_in server_addr;
    socklen_t addr_len = sizeof(server_addr);

    // Define um timeout para a operação de receção
    struct timeval timeout;
    timeout.tv_sec = 5; // 5 segundos timeout
    timeout.tv_usec = 0;
    if (setsockopt(node.reg_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0)
    {
        perror("setsockopt receive timeout");
        // Continua de qualquer forma, apenas sem timeout
    }

    int bytes_received = recvfrom(node.reg_fd, buffer, MAX_BUFFER - 1, 0,
                                  (struct sockaddr *)&server_addr, &addr_len);

    if (bytes_received <= 0)
    {
        if (bytes_received < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                printf("Timeout waiting for response from registration server\n");
            }
            else
            {
                perror("recvfrom");
            }
        }
        else
        {
            printf("Empty response from registration server\n");
        }
        return -1;
    }

    buffer[bytes_received] = '\0';
    printf("Received response from server: %s\n", buffer);

    /* Verifica se esta é uma resposta NODESLIST para a rede correta */
    char response_net[4] = {0};
    if (sscanf(buffer, "NODESLIST %3s", response_net) != 1 || strcmp(response_net, net) != 0)
    {
        printf("Invalid or mismatched NODESLIST response: %s\n", buffer);
        return -1;
    }

    /* Processa a resposta NODESLIST */
    if (process_nodeslist_response(buffer) < 0)
    {
        printf("Failed to process NODESLIST response.\n");
        return -1;
    }

    return 0;
}
// /**
//  * Directly join a network or create a new one without registering with the server.
//  *
//  * @param connect_ip IP address of the node to connect to, or 0.0.0.0 to create a new network
//  * @param connect_port TCP port of the node to connect to
//  * @return 0 on success, -1 on error
//  */
// int cmd_direct_join(char *connect_ip, char *connect_port)
// {
//     /* Check if already in a network */
//     if (node.in_network)
//     {
//         printf("Error: Already in network %03d. Leave first.\n", node.network_id);
//         return -1;
//     }

//     /* Use a default network ID (076) */
//     char net[4] = "076";

//     /* Special case - creating a new network (0.0.0.0) */
//     if (strcmp(connect_ip, "0.0.0.0") == 0)
//     {
//         printf("Creating new network %s as standalone node\n", net);

//         /* Register with the registration server for this new network */
//         if (send_reg_message(net, node.ip, node.port) < 0)
//         {
//             printf("Warning: Failed to register with the registration server.\n");
//             printf("Other nodes won't be able to discover this network via the join command.\n");
//             /* Continue anyway since this is direct join */
//         }
//         else
//         {
//             printf("Successfully registered with the registration server for network %s\n", net);
//         }

//         /* Set network ID */
//         node.network_id = atoi(net);
//         node.in_network = 1;

//         /* Initially, standalone node has no external neighbor */
//         strcpy(node.ext_neighbor_ip, "");
//         strcpy(node.ext_neighbor_port, "");

//         /* Node is its own safety node */
//         strcpy(node.safe_node_ip, node.ip);
//         strcpy(node.safe_node_port, node.port);

//         printf("Standalone node created for network %s - waiting for connections\n", net);
//         return 0;
//     }

//     /* Regular connection to an existing node */
//     printf("Connecting to node %s:%s in network %s\n", connect_ip, connect_port, net);

//     /* Connect to the specified node */
//     int fd = connect_to_node(connect_ip, connect_port);
//     if (fd < 0)
//     {
//         printf("Failed to connect to %s:%s\n", connect_ip, connect_port);
//         return -1;
//     }

//     /* Set as external neighbor */
//     strcpy(node.ext_neighbor_ip, connect_ip);
//     strcpy(node.ext_neighbor_port, connect_port);

//     /* Add as external neighbor */
//     add_neighbor(connect_ip, connect_port, fd, 1);

//     /* Send ENTRY message */
//     if (send_entry_message(fd, node.ip, node.port) < 0)
//     {
//         printf("Failed to send ENTRY message.\n");
//         close(fd);
//         return -1;
//     }

//     /* Set network ID */
//     node.network_id = atoi(net);
//     node.in_network = 1;

//     printf("Joined network %s through %s:%s\n", net, connect_ip, connect_port);
//     return 0;
// }
/**
 * Directly join a network or create a new one without registering with the server.
 *
 * @param connect_ip IP address of the node to connect to, or 0.0.0.0 to create a new network
 * @param connect_port TCP port of the node to connect to
 * @return 0 on success, -1 on error
 */
int cmd_direct_join(char *connect_ip, char *connect_port)
{
    /* Check if already in a network */
    if (node.in_network)
    {
        printf("Error: Already in network %03d. Leave first.\n", node.network_id);
        return -1;
    }

    /* Use a default network ID (076) */
    char net[4] = "076";

    /* Special case - creating a new network (0.0.0.0) */
    if (strcmp(connect_ip, "0.0.0.0") == 0)
    {
        printf("Creating new network %s as standalone node\n", net);

        /* Set network ID */
        node.network_id = atoi(net);
        node.in_network = 1;

        /* Initially, standalone node has no external neighbor */
        strcpy(node.ext_neighbor_ip, "");
        strcpy(node.ext_neighbor_port, "");

        /* Initially, safety node is left empty until second node joins */
        strcpy(node.safe_node_ip, "");
        strcpy(node.safe_node_port, "");

        printf("Standalone node created for network %s - waiting for connections\n", net);
        return 0;
    }

    /* Regular connection to an existing node */
    printf("Connecting to node %s:%s in network %s\n", connect_ip, connect_port, net);

    /* Connect to the specified node */
    int fd = connect_to_node(connect_ip, connect_port);
    if (fd < 0)
    {
        printf("Failed to connect to %s:%s\n", connect_ip, connect_port);
        return -1;
    }

    /* Set as external neighbor */
    strcpy(node.ext_neighbor_ip, connect_ip);
    strcpy(node.ext_neighbor_port, connect_port);

    /* Add as external neighbor */
    add_neighbor(connect_ip, connect_port, fd, 1);

    /* Send ENTRY message */
    if (send_entry_message(fd, node.ip, node.port) < 0)
    {
        printf("Failed to send ENTRY message.\n");
        close(fd);
        return -1;
    }

    /* Set network ID */
    node.network_id = atoi(net);
    node.in_network = 1;

    printf("Joined network %s through %s:%s\n", net, connect_ip, connect_port);
    return 0;
}

/**
 * Criar um objeto.
 *
 * @param name Nome do objeto a criar
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int cmd_create(char *name)
{
    /* Verifica se o nome contém espaços */
    if (strchr(name, ' ') != NULL)
    {
        printf("Invalid object name. Object names cannot contain spaces.\n");
        return -1;
    }

    if (!is_valid_name(name))
    {
        printf("Invalid object name. Must be alphanumeric and up to %d characters.\n", MAX_OBJECT_NAME);
        return -1;
    }

    if (add_object(name) < 0)
    {
        printf("Failed to create object %s\n", name);
        return -1;
    }

    printf("Created object %s\n", name);
    return 0;
}

/**
 * Eliminar um objeto.
 *
 * @param name Nome do objeto a eliminar
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int cmd_delete(char *name)
{
    if (!is_valid_name(name))
    {
        printf("Invalid object name. Must be alphanumeric and up to %d characters.\n", MAX_OBJECT_NAME);
        return -1;
    }

    if (remove_object(name) < 0)
    {
        printf("Object %s not found\n", name);
        return -1;
    }

    printf("Deleted object %s\n", name);
    return 0;
}

/**
 * Obter um objeto.
 * Procura localmente e, se não encontrar, envia interesse pela rede.
 *
 * @param name Nome do objeto a obter
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int cmd_retrieve(char *name)
{
    if (name == NULL || strlen(name) == 0)
    {
        printf("%sError: Object name is required%s\n", COLOR_RED, COLOR_RESET);
        return -1;
    }

    /* Check for spaces in name */
    if (strchr(name, ' ') != NULL)
    {
        printf("%sError: Object name cannot contain spaces%s\n", COLOR_RED, COLOR_RESET);
        return -1;
    }
    if (!is_valid_name(name))
    {
        printf("Invalid object name. Must be alphanumeric and up to %d characters.\n", MAX_OBJECT_NAME);
        return -1;
    }

    /* Verifica se o objeto existe localmente */
    if (find_object(name) >= 0)
    {
        printf("Object %s found locally\n", name);
        return 0;
    }

    /* Verifica se o objeto existe na cache */
    if (find_in_cache(name) >= 0)
    {
        printf("Object %s found in cache\n", name);
        return 0;
    }

    /* Verifica se está numa rede */
    if (!node.in_network)
    {
        printf("Not in a network, can't retrieve remote objects\n");
        return -1;
    }

    /* Verifica se tem vizinhos */
    if (node.neighbors == NULL)
    {
        printf("No neighbors to send interest message to\n");
        return -1;
    }

    /* Cria uma entrada de interesse para este pedido com a nossa interface "local" marcada como RESPONSE */
    InterestEntry *entry = find_or_create_interest_entry(name);
    if (entry == NULL)
    {
        printf("Failed to create interest entry\n");
        return -1;
    }

    /* Marca um ID de interface especial para a interface local como RESPONSE */
    entry->interface_states[MAX_INTERFACE - 1] = RESPONSE;
    printf("Marked local interface as RESPONSE for %s\n", name);

    /* Envia interesse para vizinhos com IDs de interface válidos */
    char message[MAX_BUFFER];
    snprintf(message, MAX_BUFFER, "INTEREST %s\n", name);

    int sent_count = 0;
    for (Neighbor *curr = node.neighbors; curr != NULL; curr = curr->next)
    {
        /* Envia apenas para vizinhos com IDs de interface válidos (maiores que 0) */
        if (curr->interface_id > 0)
        {
            if (write(curr->fd, message, strlen(message)) > 0)
            {
                entry->interface_states[curr->interface_id] = WAITING;
                sent_count++;
                printf("Sent interest for %s to neighbor at interface %d (marked WAITING)\n",
                       name, curr->interface_id);
            }
            else
            {
                perror("write");
            }
        }
    }

    if (sent_count == 0)
    {
        printf("No neighbors to send interest message to.\n");
        return -1;
    }

    printf("Sent interest for object %s to %d interfaces\n", name, sent_count);
    return 0;
}

/**
 * Mostrar a topologia da rede.
 *
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int cmd_show_topology()
{
    printf("\n%s%s┌───────────────────────────────────────────────────┐%s\n", COLOR_BOLD, COLOR_BLUE, COLOR_RESET);
    printf("%s%s│               NETWORK TOPOLOGY                     │%s\n", COLOR_BOLD, COLOR_BLUE, COLOR_RESET);
    printf("%s%s└───────────────────────────────────────────────────┘%s\n", COLOR_BOLD, COLOR_BLUE, COLOR_RESET);

    printf("%s%sNODE IDENTITY:%s\n", COLOR_BOLD, COLOR_MAGENTA, COLOR_RESET);
    printf("  %-15s: %s%s:%s%s\n", "This Node", COLOR_CYAN, node.ip, node.port, COLOR_RESET);

    if (node.in_network)
    {
        printf("  %-15s: %s%03d%s\n", "Network ID", COLOR_CYAN, node.network_id, COLOR_RESET);
    }
    else
    {
        printf("  %-15s: %sNot in a network%s\n", "Network ID", COLOR_RED, COLOR_RESET);
    }

    printf("\n%s%sCONNECTIONS:%s\n", COLOR_BOLD, COLOR_MAGENTA, COLOR_RESET);

    // External neighbor
    if (strlen(node.ext_neighbor_ip) > 0)
    {
        printf("  %-15s: %s%s:%s%s", "External", COLOR_CYAN, node.ext_neighbor_ip, node.ext_neighbor_port, COLOR_RESET);

        // Check if external neighbor is self
        if (strcmp(node.ext_neighbor_ip, node.ip) == 0 &&
            strcmp(node.ext_neighbor_port, node.port) == 0)
        {
            printf(" %s(self - standalone node)%s", COLOR_YELLOW, COLOR_RESET);
        }
        printf("\n");
    }
    else
    {
        printf("  %-15s: %sNone%s\n", "External", COLOR_RED, COLOR_RESET);
    }

    // Safety node
    if (strlen(node.safe_node_ip) > 0)
    {
        printf("  %-15s: %s%s:%s%s", "Safety", COLOR_CYAN, node.safe_node_ip, node.safe_node_port, COLOR_RESET);

        // Check if safety node is self
        if (strcmp(node.safe_node_ip, node.ip) == 0 &&
            strcmp(node.safe_node_port, node.port) == 0)
        {
            printf(" %s(self)%s", COLOR_YELLOW, COLOR_RESET);
        }
        printf("\n");
    }
    else
    {
        printf("  %-15s: %sNot set%s\n", "Safety", COLOR_RED, COLOR_RESET);
    }

    // Internal neighbors
    printf("\n%s%sINTERNAL NEIGHBORS:%s\n", COLOR_BOLD, COLOR_MAGENTA, COLOR_RESET);
    Neighbor *curr = node.internal_neighbors;

    if (curr == NULL)
    {
        printf("  %sNone%s\n", COLOR_RED, COLOR_RESET);
    }
    else
    {
        int count = 0;
        while (curr != NULL)
        {
            // Make sure to display both IP and port for each internal neighbor
            printf("  %s%d.%s %s%s:%s%s (interface: %s%d%s, fd: %d)\n",
                   COLOR_GREEN, ++count, COLOR_RESET,
                   COLOR_CYAN, curr->ip, curr->port, COLOR_RESET,
                   COLOR_YELLOW, curr->interface_id, COLOR_RESET,
                   curr->fd);
            curr = curr->next;
        }
    }

    printf("\n");
    return 0;
}

/**
 * Mostrar nomes de objetos armazenados.
 *
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int cmd_show_names()
{
    int local_count = 0;
    int cache_count = 0;
    Object *obj;

    // Count objects first
    obj = node.objects;
    while (obj != NULL)
    {
        local_count++;
        obj = obj->next;
    }

    obj = node.cache;
    while (obj != NULL)
    {
        cache_count++;
        obj = obj->next;
    }

    // Print header
    printf("\n%s%s┌───────────────────────────────────────────────────┐%s\n", COLOR_BOLD, COLOR_CYAN, COLOR_RESET);
    printf("%s%s│               STORED OBJECTS                       │%s\n", COLOR_BOLD, COLOR_CYAN, COLOR_RESET);
    printf("%s%s└───────────────────────────────────────────────────┘%s\n", COLOR_BOLD, COLOR_CYAN, COLOR_RESET);

    // Print local objects
    printf("%s%sLOCAL OBJECTS (%d):%s\n", COLOR_BOLD, COLOR_GREEN, local_count, COLOR_RESET);
    if (local_count == 0)
    {
        printf("  No objects stored locally\n");
    }
    else
    {
        int col = 0;
        obj = node.objects;
        while (obj != NULL)
        {
            printf("  %s%-24s%s", COLOR_GREEN, obj->name, COLOR_RESET);
            col++;
            if (col == 3)
            {
                printf("\n");
                col = 0;
            }
            obj = obj->next;
        }
        if (col != 0)
            printf("\n"); // End the line if not already done
    }

    // Print cache objects
    printf("\n%s%sCACHED OBJECTS (%d/%d):%s\n", COLOR_BOLD, COLOR_YELLOW, cache_count, node.cache_size, COLOR_RESET);
    if (cache_count == 0)
    {
        printf("  Cache is empty\n");
    }
    else
    {
        int col = 0;
        obj = node.cache;
        while (obj != NULL)
        {
            printf("  %s%-24s%s", COLOR_YELLOW, obj->name, COLOR_RESET);
            col++;
            if (col == 3)
            {
                printf("\n");
                col = 0;
            }
            obj = obj->next;
        }
        if (col != 0)
            printf("\n"); // End the line if not already done
    }

    printf("\n");
    return 0;
}

/**
 * Mostrar a tabela de interesses.
 *
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int cmd_show_interest_table()
{
    InterestEntry *entry = node.interest_table;
    int entry_count = 0;

    printf("\n%s%s┌───────────────────────────────────────────────────┐%s\n", COLOR_BOLD, COLOR_MAGENTA, COLOR_RESET);
    printf("%s%s│               INTEREST TABLE                       │%s\n", COLOR_BOLD, COLOR_MAGENTA, COLOR_RESET);
    printf("%s%s└───────────────────────────────────────────────────┘%s\n", COLOR_BOLD, COLOR_MAGENTA, COLOR_RESET);

    if (entry == NULL)
    {
        printf("%sNo active interests%s\n\n", COLOR_YELLOW, COLOR_RESET);
        return 0;
    }

    // First, collect information about valid interfaces (connected nodes)
    int valid_interfaces[MAX_INTERFACE] = {0};
    Neighbor *n;

    // Mark the local interface as valid
    valid_interfaces[MAX_INTERFACE - 1] = 1; // The local "interface"

    // Mark interfaces corresponding to actual neighbors as valid
    for (n = node.neighbors; n != NULL; n = n->next)
    {
        if (n->interface_id > 0 && n->interface_id < MAX_INTERFACE)
        {
            valid_interfaces[n->interface_id] = 1;
        }
    }

    while (entry != NULL)
    {
        entry_count++;
        printf("%s%sINTEREST:%s \"%s%s%s\"\n",
               COLOR_BOLD, COLOR_BLUE, COLOR_RESET,
               COLOR_CYAN, entry->name, COLOR_RESET);

        int response_count = 0;
        int waiting_count = 0;
        int closed_count = 0;

        // Count states but only for valid interfaces
        for (int i = 0; i < MAX_INTERFACE; i++)
        {
            if (valid_interfaces[i])
            {
                if (entry->interface_states[i] == RESPONSE)
                    response_count++;
                else if (entry->interface_states[i] == WAITING)
                    waiting_count++;
                else if (entry->interface_states[i] == CLOSED)
                    closed_count++;
            }
        }

        printf("  %sSummary:%s %s%d response%s, %s%d waiting%s, %s%d closed%s\n",
               COLOR_BOLD, COLOR_RESET,
               COLOR_GREEN, response_count, COLOR_RESET,
               COLOR_YELLOW, waiting_count, COLOR_RESET,
               COLOR_RED, closed_count, COLOR_RESET);

        // Then list each interface with a state, but only valid ones
        printf("  %sInterfaces:%s\n", COLOR_BOLD, COLOR_RESET);

        // First RESPONSE interfaces
        if (response_count > 0)
        {
            printf("    %s%sRESPONSE:%s ", COLOR_BOLD, COLOR_GREEN, COLOR_RESET);
            int first = 1;
            for (int i = 0; i < MAX_INTERFACE; i++)
            {
                // Only show if it's a valid interface and has RESPONSE state
                if (valid_interfaces[i] && entry->interface_states[i] == RESPONSE)
                {
                    if (!first)
                        printf(", ");

                    if (i == MAX_INTERFACE - 1)
                    {
                        printf("%sLOCAL%s", COLOR_CYAN, COLOR_RESET);
                    }
                    else
                    {
                        // Find neighbor name for this interface
                        char neighbor_info[50] = "";
                        for (n = node.neighbors; n != NULL; n = n->next)
                        {
                            if (n->interface_id == i)
                            {
                                snprintf(neighbor_info, sizeof(neighbor_info), "%s:%s", n->ip, n->port);
                                break;
                            }
                        }

                        if (strlen(neighbor_info) > 0)
                        {
                            printf("%s%d%s (%s)", COLOR_GREEN, i, COLOR_RESET, neighbor_info);
                        }
                        else
                        {
                            printf("%s%d%s", COLOR_GREEN, i, COLOR_RESET);
                        }
                    }

                    first = 0;
                }
            }
            printf("\n");
        }

        // Then WAITING interfaces
        if (waiting_count > 0)
        {
            printf("    %s%sWAITING: %s ", COLOR_BOLD, COLOR_YELLOW, COLOR_RESET);
            int first = 1;
            for (int i = 0; i < MAX_INTERFACE; i++)
            {
                // Only show if it's a valid interface and has WAITING state
                if (valid_interfaces[i] && entry->interface_states[i] == WAITING)
                {
                    if (!first)
                        printf(", ");

                    // Find neighbor name for this interface
                    char neighbor_info[50] = "";
                    for (n = node.neighbors; n != NULL; n = n->next)
                    {
                        if (n->interface_id == i)
                        {
                            snprintf(neighbor_info, sizeof(neighbor_info), "%s:%s", n->ip, n->port);
                            break;
                        }
                    }

                    if (strlen(neighbor_info) > 0)
                    {
                        printf("%s%d%s (%s)", COLOR_YELLOW, i, COLOR_RESET, neighbor_info);
                    }
                    else
                    {
                        printf("%s%d%s", COLOR_YELLOW, i, COLOR_RESET);
                    }

                    first = 0;
                }
            }
            printf("\n");
        }

        // Then CLOSED interfaces
        if (closed_count > 0)
        {
            printf("    %s%sCLOSED:  %s ", COLOR_BOLD, COLOR_RED, COLOR_RESET);
            int first = 1;
            for (int i = 0; i < MAX_INTERFACE; i++)
            {
                // Only show if it's a valid interface and has CLOSED state
                if (valid_interfaces[i] && entry->interface_states[i] == CLOSED)
                {
                    if (!first)
                        printf(", ");

                    // Find neighbor name for this interface
                    char neighbor_info[50] = "";
                    for (n = node.neighbors; n != NULL; n = n->next)
                    {
                        if (n->interface_id == i)
                        {
                            snprintf(neighbor_info, sizeof(neighbor_info), "%s:%s", n->ip, n->port);
                            break;
                        }
                    }

                    if (strlen(neighbor_info) > 0)
                    {
                        printf("%s%d%s (%s)", COLOR_RED, i, COLOR_RESET, neighbor_info);
                    }
                    else
                    {
                        printf("%s%d%s", COLOR_RED, i, COLOR_RESET);
                    }

                    first = 0;
                }
            }
            printf("\n");
        }

        // Show age of interest
        time_t now = time(NULL);
        int age = (int)difftime(now, entry->timestamp);
        printf("  %sAge:%s %s%d seconds%s\n",
               COLOR_BOLD, COLOR_RESET,
               (age > 5) ? COLOR_YELLOW : COLOR_GREEN, age, COLOR_RESET);

        printf("\n");
        entry = entry->next;
    }

    printf("%s%sTotal entries: %d%s\n\n", COLOR_BOLD, COLOR_BLUE, entry_count, COLOR_RESET);
    return 0;
}

/**
 * Sair da rede.
 *
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int cmd_leave()
{
    if (!node.in_network)
    {
        printf("Not in a network.\n");
        return -1;
    }

    /* Cancela o registo na rede */
    char net_str[4];
    snprintf(net_str, 4, "%03d", node.network_id);

    /* Faz cópias locais dos vizinhos antes de cancelar o registo */
    Neighbor *neighbor_copy = NULL;
    Neighbor *curr = node.neighbors;

    while (curr != NULL)
    {
        Neighbor *next = curr->next;
        Neighbor *new_copy = malloc(sizeof(Neighbor));
        if (new_copy != NULL)
        {
            memcpy(new_copy, curr, sizeof(Neighbor));
            new_copy->next = neighbor_copy;
            neighbor_copy = new_copy;
        }
        curr = next;
    }

    /* Envia mensagem de cancelamento de registo ao servidor */
    if (send_unreg_message(net_str, node.ip, node.port) < 0)
    {
        printf("Failed to unregister from the network.\n");

        /* Liberta as cópias já que não as usámos */
        while (neighbor_copy != NULL)
        {
            Neighbor *next = neighbor_copy->next;
            free(neighbor_copy);
            neighbor_copy = next;
        }

        return -1;
    }

    /* Fecha todas as ligações de vizinhos com segurança */
    while (neighbor_copy != NULL)
    {
        Neighbor *next = neighbor_copy->next;

        /* Encontra o vizinho real na lista */
        Neighbor *actual_neighbor = node.neighbors;
        while (actual_neighbor != NULL)
        {
            if (actual_neighbor->fd == neighbor_copy->fd)
            {
                /* Fecha o socket com verificação de erros */
                if (close(neighbor_copy->fd) < 0)
                {
                    perror("close");
                }
                break;
            }
            actual_neighbor = actual_neighbor->next;
        }

        free(neighbor_copy);
        neighbor_copy = next;
    }

    /* Agora limpa com segurança as listas de vizinhos */
    curr = node.neighbors;
    while (curr != NULL)
    {
        Neighbor *next = curr->next;
        free(curr);
        curr = next;
    }

    curr = node.internal_neighbors;
    while (curr != NULL)
    {
        Neighbor *next = curr->next;
        free(curr);
        curr = next;
    }

    /* Reinicia o estado do nó */
    node.neighbors = NULL;
    node.internal_neighbors = NULL;

    /* Reset external neighbor and safety node information when leaving network */
    memset(node.ext_neighbor_ip, 0, INET_ADDRSTRLEN);
    memset(node.ext_neighbor_port, 0, 6);
    memset(node.safe_node_ip, 0, INET_ADDRSTRLEN);
    memset(node.safe_node_port, 0, 6);

    node.in_network = 0;
    printf("Left network %03d\n", node.network_id);
    return 0;
}

/**
 * Sair da aplicação.
 *
 * @return 0 em caso de sucesso (nunca retorna na realidade, pois termina o programa)
 */
int cmd_exit()
{
    /* Se estiver numa rede, sai primeiro */
    if (node.in_network)
    {
        cmd_leave();
    }

    /* Limpa recursos e sai */
    cleanup_and_exit();
    exit(EXIT_SUCCESS);

    return 0; /* Nunca alcançado */
}