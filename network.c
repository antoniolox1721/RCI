/*
 * Implementação de Rede de Dados Identificados por Nome (NDN)
 * Redes de Computadores e Internet - 2024/2025
 *
 * network.c - Implementação das funções de protocolos de rede
 */

#include "network.h"
#include "objects.h"
#include "debug_utils.h"

/**
 * Reinicia o interesse para um objeto, removendo a sua entrada
 *
 * @param name Nome do objeto associado ao interesse a reiniciar
 */
void reset_interest_for_object(char *name)
{
    InterestEntry *entry = node.interest_table;
    InterestEntry *prev = NULL;

    while (entry != NULL)
    {
        if (strcmp(entry->name, name) == 0)
        {
            /* Encontrou a entrada - verifica se já está a ser removida */
            if (entry->marked_for_removal)
            {
                printf("WARNING: Interest entry for %s is already marked for removal\n", name);
                return;
            }

            printf("INTEREST RESET: Removing interest entry for %s\n", name);

            /* Marca-a para que outras funções saibam que não a devem usar */
            entry->marked_for_removal = 1;

            /* Remove-a realmente da lista */
            if (prev == NULL)
            {
                node.interest_table = entry->next;
            }
            else
            {
                prev->next = entry->next;
            }

            InterestEntry *to_free = entry;
            entry = entry->next;
            free(to_free);
            return;
        }

        prev = entry;
        entry = entry->next;
    }

    printf("INTEREST RESET: No entry found for %s\n", name);
}

/**
 * Inicializa uma entrada de interesse
 *
 * @param entry Apontador para a entrada de interesse a inicializar
 * @param name Nome do objeto associado à entrada
 */
void initialize_interest_entry(InterestEntry *entry, char *name)
{
    strcpy(entry->name, name);

    /* Inicializa todas as interfaces para 0 (sem estado) */
    for (int i = 0; i < MAX_INTERFACE; i++)
    {
        entry->interface_states[i] = 0;
    }

    entry->timestamp = time(NULL);
    entry->next = NULL;
}

/**
 * Updates and propagates safety node information to all internal neighbors
 * Should be called whenever the topology changes in a way that affects safety nodes
 */
void update_and_propagate_safety_node() {
    printf("SAFETY: Updating and propagating safety node information\n");
    printf("SAFETY: Current external neighbor: %s:%s\n", node.ext_neighbor_ip, node.ext_neighbor_port);
    printf("SAFETY: Current safety node: %s:%s\n", node.safe_node_ip, node.safe_node_port);
    
    /* First, ensure all internal neighbors have updated safety information */
    int sent_count = 0;
    Neighbor *n = node.internal_neighbors;
    while (n != NULL) {
        /* Check if interface ID is valid - similar to interest message propagation */
        if (n->interface_id > 0) {
            /* Create the SAFE message with the current external neighbor */
            char safe_msg[MAX_BUFFER];
            snprintf(safe_msg, MAX_BUFFER, "SAFE %s %s\n",
                    node.ext_neighbor_ip, node.ext_neighbor_port);
            
            printf("SAFETY: Sending updated SAFE message to %s:%s (fd: %d, interface: %d): %s", 
                   n->ip, n->port, n->fd, n->interface_id, safe_msg);
            
            /* Send the message and handle errors */
            if (write(n->fd, safe_msg, strlen(safe_msg)) < 0) {
                perror("write");
                printf("SAFETY: Failed to send SAFE message to %s:%s (fd: %d, interface: %d)\n", 
                      n->ip, n->port, n->fd, n->interface_id);
                /* Continue with other neighbors even if one fails */
            } else {
                printf("SAFETY: Successfully sent SAFE message to %s:%s (fd: %d, interface: %d)\n", 
                      n->ip, n->port, n->fd, n->interface_id);
                sent_count++;
            }
        } else {
            printf("SAFETY: Skipping neighbor %s:%s with invalid interface ID: %d\n", 
                   n->ip, n->port, n->interface_id);
        }
        
        n = n->next;
    }
    
    printf("SAFETY: Finished propagating safety node information to %d internal neighbors\n", sent_count);
}

/**
 * Processes a SAFE message received from a neighbor
 * @param fd File descriptor of the connection
 * @param ip IP address of the safety node
 * @param port Port of the safety node
 * @return 0 on success, -1 on error
 */
int handle_safe_message(int fd, char *ip, char *port) {
    printf("SAFETY: Received SAFE message for safety node %s:%s from fd %d\n", ip, port, fd);
    
    /* Find the neighbor that sent this message */
    Neighbor *curr = node.neighbors;
    int found = 0;

    while (curr != NULL) {
        if (curr->fd == fd) {
            found = 1;
            break;
        }
        curr = curr->next;
    }

    if (!found) {
        printf("SAFETY: Warning: Received SAFE message from unknown neighbor (fd: %d)\n", fd);
    } else {
        printf("SAFETY: Received SAFE message from %s:%s (interface %d)\n",
               curr->ip, curr->port, curr->interface_id);
    }

    /* Validate safety node information */
    if (ip == NULL || port == NULL || strlen(ip) == 0 || strlen(port) == 0) {
        printf("SAFETY: Invalid safety node information: IP or port is empty\n");
        return -1;
    }

    /* Update safety node information */
    printf("SAFETY: Updating safety node from %s:%s to %s:%s\n", 
           node.safe_node_ip, node.safe_node_port, ip, port);
    
    strcpy(node.safe_node_ip, ip);
    strcpy(node.safe_node_port, port);

    /* Debug output: show current topology */
    printf("SAFETY: Current topology:\n");
    printf("SAFETY:   External neighbor: %s:%s\n", node.ext_neighbor_ip, node.ext_neighbor_port);
    printf("SAFETY:   Safety node: %s:%s\n", node.safe_node_ip, node.safe_node_port);
    
    return 0;
}

/**
 * Trata eventos de rede
 * Processa novas ligações e dados recebidos nos sockets de vizinhos
 */
void handle_network_events()
{
    /* Verifica se há uma nova ligação no socket de escuta */
    if (FD_ISSET(node.listen_fd, &node.read_fds))
    {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        int new_fd = accept(node.listen_fd, (struct sockaddr *)&client_addr, &addr_len);

        if (new_fd == -1)
        {
            perror("accept");
        }
        else
        {
            /* Uma nova ligação foi estabelecida */
            char client_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
            char client_port[6];
            snprintf(client_port, 6, "%d", ntohs(client_addr.sin_port));

            printf("New connection from %s:%s\n", client_ip, client_port);

            /* Ainda não sabemos se este é um vizinho interno ou externo */
            /* Isto será determinado pela mensagem recebida */

            /* Adiciona à lista de vizinhos sem categorizar ainda */
            add_neighbor(client_ip, client_port, new_fd, 0);

            /* Atualiza max_fd se necessário */
            if (new_fd > node.max_fd)
            {
                node.max_fd = new_fd;
            }
        }
    }

    /* Verifica atividade nos sockets de vizinhos */
    Neighbor *curr = node.neighbors;
    while (curr != NULL)
    {
        Neighbor *next = curr->next; /* Guarda o apontador para o próximo caso o nó atual seja removido */

        if (FD_ISSET(curr->fd, &node.read_fds))
        {
            char buffer[MAX_BUFFER];
            int bytes_received = read(curr->fd, buffer, MAX_BUFFER - 1);

            if (bytes_received <= 0)
            {
                /* Ligação fechada ou erro */
                if (bytes_received == 0)
                {
                    printf("Connection closed by %s:%s\n", curr->ip, curr->port);
                }
                else
                {
                    perror("read");
                }

                /* Remove o vizinho */
                remove_neighbor(curr->fd);
            }
            else
            {
                /* Processa a mensagem recebida */
                buffer[bytes_received] = '\0';
                printf("Received message from %s:%s: %s\n", curr->ip, curr->port, buffer);

                /* Faz uma cópia do buffer antes de tokenizar */
                char buffer_copy[MAX_BUFFER];
                strncpy(buffer_copy, buffer, MAX_BUFFER - 1);
                buffer_copy[MAX_BUFFER - 1] = '\0';

                /* Analisa o tipo e conteúdo da mensagem */
                char *msg_type = strtok(buffer_copy, " \n");
                if (msg_type != NULL)
                {
                    if (strcmp(msg_type, "INTEREST") == 0)
                    {
                        char *name = strtok(NULL, "\n");
                        if (name != NULL)
                        {
                            handle_interest_message(curr->fd, name);
                        }
                    }
                    else if (strcmp(msg_type, "OBJECT") == 0)
                    {
                        char *name = strtok(NULL, "\n");
                        if (name != NULL)
                        {
                            handle_object_message(curr->fd, name);
                        }
                    }
                    else if (strcmp(msg_type, "NOOBJECT") == 0)
                    {
                        char *name = strtok(NULL, "\n");
                        if (name != NULL)
                        {
                            handle_noobject_message(curr->fd, name);
                        }
                    }
                    else if (strcmp(msg_type, "ENTRY") == 0)
                    {
                        /* Extrai o IP e porto da mensagem sem strtok no buffer original */
                        char sender_ip[INET_ADDRSTRLEN] = {0};
                        char sender_port[6] = {0};

                        /* Analisa a mensagem original para evitar problemas com strtok */
                        if (sscanf(buffer, "ENTRY %s %s", sender_ip, sender_port) == 2)
                        {
                            printf("Received ENTRY message from %s:%s\n", sender_ip, sender_port);

                            /* Trata como vizinho interno */
                            /* Primeiro marca este vizinho como interno na nossa lista */
                            Neighbor *n = node.neighbors;
                            while (n != NULL)
                            {
                                if (n->fd == curr->fd)
                                {
                                    /* Certifica-se de que está na lista de vizinhos internos */
                                    int already_internal = 0;
                                    Neighbor *internal = node.internal_neighbors;
                                    while (internal != NULL)
                                    {
                                        if (internal->fd == curr->fd)
                                        {
                                            already_internal = 1;
                                            break;
                                        }
                                        internal = internal->next;
                                    }

                                    if (!already_internal)
                                    {
                                        /* Adiciona à lista de vizinhos internos */
                                        Neighbor *internal_copy = malloc(sizeof(Neighbor));
                                        if (internal_copy != NULL)
                                        {
                                            memcpy(internal_copy, n, sizeof(Neighbor));
                                            internal_copy->next = node.internal_neighbors;
                                            node.internal_neighbors = internal_copy;
                                            printf("Added %s:%s as internal neighbor\n", sender_ip, sender_port);
                                        }
                                    }
                                    break;
                                }
                                n = n->next;
                            }

                            /* De acordo com o protocolo, responde com a informação do vizinho externo */
                            char safe_msg[MAX_BUFFER];
                            snprintf(safe_msg, MAX_BUFFER, "SAFE %s %s\n",
                                     node.ext_neighbor_ip, node.ext_neighbor_port);

                            printf("Sending SAFE message: %s", safe_msg);
                            if (write(curr->fd, safe_msg, strlen(safe_msg)) < 0)
                            {
                                perror("write");
                            }
                        }
                        else
                        {
                            printf("Malformed ENTRY message: %s\n", buffer);
                        }
                    }
                    else if (strcmp(msg_type, "SAFE") == 0)
                    {
                        /* Extrai o IP e porto da mensagem sem strtok no buffer original */
                        char safe_ip[INET_ADDRSTRLEN] = {0};
                        char safe_port[6] = {0};

                        /* Analisa a mensagem original para evitar problemas com strtok */
                        if (sscanf(buffer, "SAFE %s %s", safe_ip, safe_port) == 2)
                        {
                            printf("Received SAFE message, safety node set to %s:%s\n", safe_ip, safe_port);

                            /* Atualiza a informação do nó de salvaguarda */
                            strcpy(node.safe_node_ip, safe_ip);
                            strcpy(node.safe_node_port, safe_port);
                        }
                        else
                        {
                            printf("Malformed SAFE message: %s\n", buffer);
                        }
                    }
                    else
                    {
                        printf("Unknown message type: %s\n", msg_type);
                    }
                }
            }
        }

        curr = next;
    }
}

/**
 * Envia uma mensagem de registo ao servidor de registo
 *
 * @param net ID da rede (três dígitos)
 * @param ip Endereço IP do nó
 * @param port Porto TCP do nó
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int send_reg_message(char *net, char *ip, char *port)
{
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(node.reg_server_port));
    inet_pton(AF_INET, node.reg_server_ip, &server_addr.sin_addr);

    char message[MAX_BUFFER];
    snprintf(message, MAX_BUFFER, "REG %s %s %s", net, ip, port);

    printf("Sending registration to %s:%s: %s\n", node.reg_server_ip, node.reg_server_port, message);

    if (sendto(node.reg_fd, message, strlen(message), 0,
               (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("sendto");
        return -1;
    }

    /* Recebe e verifica a resposta */
    char buffer[MAX_BUFFER];
    socklen_t addr_len = sizeof(server_addr);

    int bytes_received = recvfrom(node.reg_fd, buffer, MAX_BUFFER - 1, 0,
                                  (struct sockaddr *)&server_addr, &addr_len);

    if (bytes_received <= 0)
    {
        perror("recvfrom");
        return -1;
    }

    buffer[bytes_received] = '\0';

    if (strcmp(buffer, "OKREG") != 0)
    {
        printf("Unexpected response from registration server: %s\n", buffer);
        return -1;
    }

    return 0;
}
/**
 * Envia uma mensagem de remoção de registo ao servidor de registo
 *
 * @param net ID da rede (três dígitos)
 * @param ip Endereço IP do nó
 * @param port Porto TCP do nó
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int send_unreg_message(char *net, char *ip, char *port)
{
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(node.reg_server_port));
    inet_pton(AF_INET, node.reg_server_ip, &server_addr.sin_addr);

    char message[MAX_BUFFER];
    snprintf(message, MAX_BUFFER, "UNREG %s %s %s", net, ip, port);

    printf("Sending unregistration to %s:%s: %s\n", node.reg_server_ip, node.reg_server_port, message);

    if (sendto(node.reg_fd, message, strlen(message), 0,
               (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("sendto");
        return -1;
    }

    /* Recebe e verifica a resposta */
    char buffer[MAX_BUFFER];
    socklen_t addr_len = sizeof(server_addr);

    int bytes_received = recvfrom(node.reg_fd, buffer, MAX_BUFFER - 1, 0,
                                  (struct sockaddr *)&server_addr, &addr_len);

    if (bytes_received <= 0)
    {
        perror("recvfrom");
        return -1;
    }

    buffer[bytes_received] = '\0';

    if (strcmp(buffer, "OKUNREG") != 0)
    {
        printf("Unexpected response from registration server: %s\n", buffer);
        return -1;
    }

    return 0;
}

/**
 * Envia um pedido de nós ao servidor de registo
 * Modificado para garantir a formatação correta do ID de rede
 *
 * @param net ID da rede (três dígitos)
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int send_nodes_request(char *net)
{
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(node.reg_server_port));
    inet_pton(AF_INET, node.reg_server_ip, &server_addr.sin_addr);

    /* Garante que estamos a usar o formato de ID de rede exato */
    char message[MAX_BUFFER];
    snprintf(message, MAX_BUFFER, "NODES %s", net);

    printf("Sending request: %s to registration server %s:%s\n", 
           message, node.reg_server_ip, node.reg_server_port);

    if (sendto(node.reg_fd, message, strlen(message), 0,
               (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("sendto");
        return -1;
    }

    return 0;
}

/**
 * Processa uma resposta NODESLIST do servidor de registo
 * Melhorado para lidar corretamente com respostas de rede mistas e verificar a adesão à rede
 *
 * @param buffer Buffer contendo a resposta NODESLIST
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int process_nodeslist_response(char *buffer)
{
    /* Extrai o ID da rede */
    char *line = strtok(buffer, "\n");
    if (line == NULL)
    {
        printf("Invalid NODESLIST response: empty\n");
        return -1;
    }

    char requested_net[4];
    if (sscanf(line, "NODESLIST %3s", requested_net) != 1)
    {
        printf("Invalid NODESLIST response: %s\n", line);
        return -1;
    }

    printf("Processing NODESLIST for network %s\n", requested_net);

    /* Extrai a lista de nós */
    int node_count = 0;
    char node_ips[100][INET_ADDRSTRLEN];
    char node_ports[100][6];

    while ((line = strtok(NULL, "\n")) != NULL && node_count < 100)
    {
        /* Cada linha tem o formato "IP PORT" */
        if (sscanf(line, "%s %s", node_ips[node_count], node_ports[node_count]) == 2)
        {
            /* Ignora entradas claramente inválidas */
            if (strcmp(node_ports[node_count], "0") == 0 ||strcmp(node_ips[node_count], "0.0.0.0") == 0)
            {
                printf("Skipping invalid node entry: %s %s\n",
                       node_ips[node_count], node_ports[node_count]);
                continue;
            }

            /* Ignora-se a si próprio */
            if (strcmp(node_ips[node_count], node.ip) == 0 &&
                strcmp(node_ports[node_count], node.port) == 0)
            {
                printf("Skipping self: %s %s\n", node_ips[node_count], node_ports[node_count]);
                continue;
            }

            node_count++;
        }
    }

    printf("Received %d potential nodes from registration server\n", node_count);

    /* Se não houver nós, cria uma nova rede */
    if (node_count == 0)
    {
        printf("No valid nodes found in network %s, creating new network\n", requested_net);

        /* Regista-se na rede */
        if (send_reg_message(requested_net, node.ip, node.port) < 0)
        {
            printf("Failed to register with the network.\n");
            return -1;
        }

        /* Define o ID da rede e marca como in_network */
        node.network_id = atoi(requested_net);
        node.in_network = 1;

        /* Este nó é o seu próprio vizinho externo e nó de salvaguarda */
        strcpy(node.ext_neighbor_ip, node.ip);
        strcpy(node.ext_neighbor_port, node.port);
        strcpy(node.safe_node_ip, node.ip);
        strcpy(node.safe_node_port, node.port);

        printf("Created and joined network %s\n", requested_net);
        return 0;
    }

    /* Tenta ligar-se a um nó aleatório da lista */
    int random_index = rand() % node_count;
    char *chosen_ip = node_ips[random_index];
    char *chosen_port = node_ports[random_index];

    printf("Attempting to connect to node %s:%s\n", chosen_ip, chosen_port);

    /* Liga-se ao nó escolhido */
    int fd = connect_to_node(chosen_ip, chosen_port);
    if (fd < 0)
    {
        printf("Failed to connect to %s:%s\n", chosen_ip, chosen_port);
        return -1;
    }

    /* Define o vizinho externo */
    strcpy(node.ext_neighbor_ip, chosen_ip);
    strcpy(node.ext_neighbor_port, chosen_port);

    /* Adiciona o nó como vizinho */
    add_neighbor(chosen_ip, chosen_port, fd, 1);

    /* Envia mensagem ENTRY */
    if (send_entry_message(fd, node.ip, node.port) < 0)
    {
        printf("Failed to send ENTRY message.\n");
        close(fd);
        return -1;
    }

    /* Regista-se na rede */
    if (send_reg_message(requested_net, node.ip, node.port) < 0)
    {
        printf("Failed to register with the network.\n");
        close(fd);
        return -1;
    }

    /* Define o ID da rede e marca como in_network */
    node.network_id = atoi(requested_net);
    node.in_network = 1;

    printf("Joined network %s through %s:%s\n", requested_net, chosen_ip, chosen_port);

    /* Aguarda mensagem SAFE para definir o nó de salvaguarda */
    printf("Waiting for SAFE message from external neighbor...\n");

    return 0;
}

/**
 * Envia uma mensagem ENTRY para um nó
 *
 * @param fd Descritor de ficheiro da ligação
 * @param ip Endereço IP do nó emissor
 * @param port Porto TCP do nó emissor
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int send_entry_message(int fd, char *ip, char *port)
{
    char message[MAX_BUFFER];
    snprintf(message, MAX_BUFFER, "ENTRY %s %s\n", ip, port);

    if (write(fd, message, strlen(message)) < 0)
    {
        perror("write");
        return -1;
    }

    return 0;
}

/**
 * Envia uma mensagem SAFE para um nó
 *
 * @param fd Descritor de ficheiro da ligação
 * @param ip Endereço IP do nó de salvaguarda
 * @param port Porto TCP do nó de salvaguarda
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int send_safe_message(int fd, char *ip, char *port)
{
    char message[MAX_BUFFER];
    snprintf(message, MAX_BUFFER, "SAFE %s %s\n", ip, port);

    if (write(fd, message, strlen(message)) < 0)
    {
        perror("write");
        return -1;
    }

    return 0;
}

/**
 * Processa uma mensagem ENTRY recebida
 *
 * @param fd Descritor de ficheiro da ligação
 * @param ip Endereço IP do nó emissor
 * @param port Porto TCP do nó emissor
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int handle_entry_message(int fd, char *ip, char *port)
{
    printf("Received ENTRY message from %s:%s\n", ip, port);

    /* Adiciona o nó como vizinho interno */
    add_neighbor(ip, port, fd, 0); /* 0 = interno */

    /* Envia mensagem SAFE com informação do vizinho externo */
    if (send_safe_message(fd, node.ext_neighbor_ip, node.ext_neighbor_port) < 0)
    {
        printf("Failed to send SAFE message.\n");
        return -1;
    }

    return 0;
}

/**
 * Envia uma mensagem de interesse para um objeto
 *
 * @param name Nome do objeto pretendido
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int send_interest_message(char *name)
{
    char message[MAX_BUFFER];
    snprintf(message, MAX_BUFFER, "INTEREST %s\n", name);

    /* Cria uma nova entrada de interesse se não existir */
    InterestEntry *entry = find_or_create_interest_entry(name);
    if (entry == NULL)
    {
        printf("Failed to create interest entry for %s\n", name);
        return -1;
    }

    /* Envia para todos os vizinhos e marca as suas interfaces como WAITING */
    Neighbor *curr = node.neighbors;
    int sent_count = 0;

    while (curr != NULL)
    {
        /* CORREÇÃO CRÍTICA: Apenas usa vizinhos com IDs de interface válidos para encaminhamento */
        if (curr->interface_id > 0)
        { // Ignora interface ID 0 (ligações de saída)
            printf("Sending interest for %s to neighbor at fd %d (interface %d)\n",
                   name, curr->fd, curr->interface_id);

            if (write(curr->fd, message, strlen(message)) > 0)
            {
                /* Marca esta interface como WAITING */
                entry->interface_states[curr->interface_id] = WAITING;
                sent_count++;
                printf("Sent interest for %s to neighbor at interface %d (marked WAITING)\n",
                       name, curr->interface_id);
            }
            else
            {
                perror("write");
                /* Continua com outros vizinhos */
            }
        }
        else
        {
            printf("Skipping outgoing connection %s:%s (interface 0)\n", curr->ip, curr->port);
        }

        curr = curr->next;
    }

    if (sent_count == 0)
    {
        /* Não há vizinhos para enviar a mensagem */
        printf("No neighbors to send interest message to.\n");
        return -1;
    }

    /* Atualiza o timestamp para iniciar o temporizador de timeout */
    entry->timestamp = time(NULL);

    return 0;
}

/**
 * Envia uma mensagem de objeto para um nó
 *
 * @param fd Descritor de ficheiro da ligação
 * @param name Nome do objeto a enviar
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int send_object_message(int fd, char *name)
{
    char message[MAX_BUFFER];
    snprintf(message, MAX_BUFFER, "OBJECT %s\n", name);

    /* Garante que a ligação ainda é válida */
    int error = 0;
    socklen_t len = sizeof(error);
    if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &len) < 0 || error != 0)
    {
        printf("Socket error detected before sending object message: %s\n", strerror(error));
        return -1;
    }

    /* Envia a mensagem com tratamento de erros cuidadoso */
    size_t message_len = strlen(message);
    ssize_t bytes_sent = write(fd, message, message_len);

    if (bytes_sent < 0)
    {
        if (errno == EPIPE)
        {
            printf("Connection closed when trying to send object message\n");
        }
        else
        {
            printf("Write error when sending object message: %s\n", strerror(errno));
        }
        return -1;
    }
    else if ((size_t)bytes_sent < message_len)
    {
        printf("Partial write when sending object message: %zd of %zu bytes\n",
               bytes_sent, message_len);
        return -1;
    }

    printf("Successfully sent object %s to fd %d\n", name, fd);
    // debug_interest_table();
    return 0;
}

/**
 * Envia uma mensagem NOOBJECT para um nó
 *
 * @param fd Descritor de ficheiro da ligação
 * @param name Nome do objeto não encontrado
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int send_noobject_message(int fd, char *name)
{
    char message[MAX_BUFFER];
    snprintf(message, MAX_BUFFER, "NOOBJECT %s\n", name);

    if (write(fd, message, strlen(message)) < 0)
    {
        perror("write");
        return -1;
    }

    return 0;
}

/**
 * Processa uma mensagem de interesse recebida
 * Versão corrigida com melhor tratamento de erros
 *
 * @param fd Descritor de ficheiro da ligação
 * @param name Nome do objeto pretendido
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int handle_interest_message(int fd, char *name)
{
    /* Obtém o ID de interface para este descritor de ficheiro */
    int interface_id = -1;
    Neighbor *curr = node.neighbors;
    while (curr != NULL)
    {
        if (curr->fd == fd)
        {
            interface_id = curr->interface_id;
            break;
        }
        curr = curr->next;
    }

    if (interface_id < 0)
    {
        printf("Interface ID not found for fd %d\n", fd);
        return -1;
    }

    /* Ignora processamento para ID de interface 0 (ligações de saída) */
    if (interface_id == 0)
    {
        printf("Ignoring interest from outgoing connection (interface 0)\n");
        return 0;
    }

    printf("Received interest for %s on interface %d\n", name, interface_id);

    /* Verifica se temos o objeto localmente */
    if (find_object(name) >= 0 || find_in_cache(name) >= 0)
    {
        printf("Found object %s locally, sending back\n", name);
        return send_object_message(fd, name);
    }

    /* Procura ou cria entrada de interesse */
    InterestEntry *entry = find_or_create_interest_entry(name);
    if (entry == NULL)
    {
        return -1;
    }

    /* Marca a interface de origem como RESPONSE */
    entry->interface_states[interface_id] = RESPONSE;
    printf("Marked interface %d as RESPONSE for %s\n", interface_id, name);

    /* Verifica se já estamos a encaminhar este interesse */
    int has_waiting = 0;
    for (int i = 1; i < MAX_INTERFACE; i++)
    {
        if (entry->interface_states[i] == WAITING)
        {
            has_waiting = 1;
            break;
        }
    }

    /* Se já estamos a encaminhar este interesse, não encaminhamos novamente */
    if (has_waiting)
    {
        printf("Already forwarding interest for %s\n", name);
        return 0;
    }

    /* Encaminha para todos os outros vizinhos com IDs de interface válidos */
    int forwarded = 0;

    curr = node.neighbors;
    while (curr != NULL)
    {
        if (curr->interface_id > 0 && curr->interface_id != interface_id)
        {
            char message[MAX_BUFFER];
            snprintf(message, MAX_BUFFER, "INTEREST %s\n", name);

            if (write(curr->fd, message, strlen(message)) > 0)
            {
                entry->interface_states[curr->interface_id] = WAITING;
                forwarded++;
                printf("Forwarded interest for %s to interface %d\n", name, curr->interface_id);
            }
        }
        curr = curr->next;
    }

    if (forwarded == 0)
    {
        printf("No neighbors to forward interest to, sending NOOBJECT\n");
        return send_noobject_message(fd, name);
    }

    /* Atualiza o timestamp para iniciar o temporizador de timeout */
    entry->timestamp = time(NULL);

    return 0;
}

/**
 * Processa uma mensagem de objeto recebida
 *
 * @param fd Descritor de ficheiro da ligação
 * @param name Nome do objeto recebido
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int handle_object_message(int fd, char *name)
{
    /* Obtém o ID de interface para este descritor de ficheiro */
    int interface_id = -1;
    Neighbor *curr = node.neighbors;
    while (curr != NULL)
    {
        if (curr->fd == fd)
        {
            interface_id = curr->interface_id;
            break;
        }
        curr = curr->next;
    }

    if (interface_id < 0)
    {
        printf("Interface ID not found for fd %d\n", fd);
        return -1;
    }

    /* Ignora processamento para ID de interface 0 (ligações de saída) */
    if (interface_id == 0)
    {
        printf("Ignoring object from outgoing connection (interface 0)\n");
        return 0;
    }

    printf("Received object %s from interface %d\n", name, interface_id);

    /* Adiciona o objeto à cache */
    if (add_to_cache(name) < 0)
    {
        printf("Failed to add object %s to cache\n", name);
    }
    else
    {
        printf("Added object %s to cache\n", name);
    }

    /* Procura a entrada de interesse */
    InterestEntry *entry = find_interest_entry(name);
    if (!entry)
    {
        printf("No interest entry found for %s\n", name);
        return 0;
    }

    /* Encaminha o objeto para todas as interfaces marcadas como RESPONSE */
    for (int i = 1; i < MAX_INTERFACE; i++)
    {
        if (entry->interface_states[i] == RESPONSE)
        {
            /* Procura o vizinho com este ID de interface */
            for (Neighbor *n = node.neighbors; n != NULL; n = n->next)
            {
                if (n->interface_id == i)
                {
                    printf("Forwarding object %s to interface %d\n", name, i);
                    send_object_message(n->fd, name);
                    break;
                }
            }
        }
    }

    /* Verifica se a UI local está à espera deste objeto */
    if (entry->interface_states[MAX_INTERFACE - 1] == RESPONSE)
    {
        printf("Object %s found for local request\n", name);
    }

    /* Remove a entrada de interesse */
    remove_interest_entry(name);

    return 0;
}

/**
 * Processa uma mensagem NOOBJECT recebida
 * Versão modificada com melhor rastreamento de estado
 *
 * @param fd Descritor de ficheiro da ligação
 * @param name Nome do objeto não encontrado
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int handle_noobject_message(int fd, char *name)
{
    /* Obtém o ID de interface para este descritor de ficheiro */
    int interface_id = -1;
    Neighbor *curr = node.neighbors;
    while (curr != NULL)
    {
        if (curr->fd == fd)
        {
            interface_id = curr->interface_id;
            break;
        }
        curr = curr->next;
    }

    if (interface_id < 0)
    {
        printf("Interface ID not found for fd %d\n", fd);
        return -1;
    }

    /* Ignora processamento para ID de interface 0 (ligações de saída) */
    if (interface_id == 0)
    {
        printf("Ignoring NOOBJECT from outgoing connection (interface 0)\n");
        return 0;
    }

    printf("Received NOOBJECT for %s from interface %d\n", name, interface_id);

    /* Procura a entrada de interesse */
    InterestEntry *entry = find_interest_entry(name);
    if (!entry)
    {
        printf("No interest entry found for %s\n", name);
        return 0;
    }

    /* Atualiza a entrada para marcar esta interface como CLOSED */
    entry->interface_states[interface_id] = CLOSED;
    printf("Marked interface %d as CLOSED for %s\n", interface_id, name);

    /* Verifica se há interfaces ainda em estado WAITING */
    int waiting_count = 0;
    for (int i = 1; i < MAX_INTERFACE; i++)
    {
        if (entry->interface_states[i] == WAITING)
        {
            /* Verifica se esta é uma interface válida */
            int valid = 0;
            for (Neighbor *n = node.neighbors; n != NULL; n = n->next)
            {
                if (n->interface_id == i)
                {
                    valid = 1;
                    break;
                }
            }

            if (valid)
            {
                waiting_count++;
            }
            else
            {
                /* Interface inválida - marca como CLOSED */
                entry->interface_states[i] = CLOSED;
                printf("Marked invalid interface %d as CLOSED for %s\n", i, name);
            }
        }
    }

    if (waiting_count == 0)
    {
        /* Não há interfaces em estado WAITING, envia NOOBJECT para todas as interfaces RESPONSE */
        printf("No more waiting interfaces for %s, notifying requesters\n", name);

        /* Notifica todas as interfaces RESPONSE */
        for (int i = 1; i < MAX_INTERFACE; i++)
        {
            if (entry->interface_states[i] == RESPONSE)
            {
                /* Procura o vizinho com este ID de interface */
                for (Neighbor *n = node.neighbors; n != NULL; n = n->next)
                {
                    if (n->interface_id == i)
                    {
                        send_noobject_message(n->fd, name);
                        break;
                    }
                }
            }
        }

        /* Verifica se a UI local está à espera deste objeto */
        if (entry->interface_states[MAX_INTERFACE - 1] == RESPONSE)
        {
            printf("Object %s not found for local request\n", name);
        }

        /* Remove a entrada de interesse */
        remove_interest_entry(name);
    }

    return 0;
}

/**
 * Liga-se a um nó com tratamento de erros melhorado e timeout
 *
 * @param ip Endereço IP do nó a ligar
 * @param port Porto TCP do nó a ligar
 * @return Descritor de ficheiro da ligação em caso de sucesso, -1 em caso de erro
 */
int connect_to_node(char *ip, char *port)
{
    struct addrinfo hints, *res;
    int fd, errcode;

    printf("Attempting to connect to %s:%s\n", ip, port);

    /* Cria socket */
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1)
    {
        perror("socket");
        return -1;
    }

    /* Define opções de timeout do socket para connect */
    struct timeval timeout;
    timeout.tv_sec = 3; /* 3 segundos de timeout */
    timeout.tv_usec = 0;

    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0)
    {
        perror("setsockopt receive timeout");
    }

    if (setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0)
    {
        perror("setsockopt send timeout");
    }

    /* Torna o socket não-bloqueante para connect */
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
    {
        perror("fcntl get flags");
        close(fd);
        return -1;
    }

    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
    {
        perror("fcntl set non-blocking");
        close(fd);
        return -1;
    }

    /* Liga-se ao nó especificado */
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if ((errcode = getaddrinfo(ip, port, &hints, &res)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(errcode));
        close(fd);
        return -1;
    }

    /* Connect não-bloqueante */
    int connect_result = connect(fd, res->ai_addr, res->ai_addrlen);
    if (connect_result < 0)
    {
        if (errno != EINPROGRESS)
        {
            perror("connect");
            freeaddrinfo(res);
            close(fd);
            return -1;
        }

        /* Aguarda pela ligação usando select */
        fd_set write_fds;
        FD_ZERO(&write_fds);
        FD_SET(fd, &write_fds);

        /* Define timeout para a ligação */
        struct timeval connect_timeout;
        connect_timeout.tv_sec = 3; /* 3 segundos de timeout */
        connect_timeout.tv_usec = 0;

        int select_result = select(fd + 1, NULL, &write_fds, NULL, &connect_timeout);

        if (select_result <= 0)
        {
            if (select_result == 0)
            {
                printf("Connection to %s:%s timed out\n", ip, port);
            }
            else
            {
                perror("select");
            }
            freeaddrinfo(res);
            close(fd);
            return -1;
        }

        /* Verifica se a ligação foi bem-sucedida */
        int so_error;
        socklen_t len = sizeof(so_error);
        if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &so_error, &len) < 0)
        {
            perror("getsockopt");
            freeaddrinfo(res);
            close(fd);
            return -1;
        }

        if (so_error != 0)
        {
            fprintf(stderr, "Connection error: %s\n", strerror(so_error));
            freeaddrinfo(res);
            close(fd);
            return -1;
        }
    }

    /* Volta a colocar o socket em modo bloqueante */
    if (fcntl(fd, F_SETFL, flags) == -1)
    {
        perror("fcntl set blocking");
        freeaddrinfo(res);
        close(fd);
        return -1;
    }

    freeaddrinfo(res);

    /* Atualiza max_fd se necessário */
    if (fd > node.max_fd)
    {
        node.max_fd = fd;
    }

    printf("Successfully connected to %s:%s (fd: %d)\n", ip, port, fd);
    return fd;
}

/**
 * Adiciona um vizinho
 *
 * @param ip Endereço IP do vizinho
 * @param port Porto TCP do vizinho
 * @param fd Descritor de ficheiro da ligação
 * @param is_external 1 se for vizinho externo, 0 se for interno
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int add_neighbor(char *ip, char *port, int fd, int is_external)
{
    /* Cria um novo vizinho */
    Neighbor *new_neighbor = malloc(sizeof(Neighbor));
    if (new_neighbor == NULL)
    {
        perror("malloc");
        return -1;
    }
    
    strcpy(new_neighbor->ip, ip);
    strcpy(new_neighbor->port, port);
    new_neighbor->fd = fd;

    /* Encontra o próximo ID de interface disponível */
    int interface_id = 1;
    Neighbor *curr = node.neighbors;
    while (curr != NULL)
    {
        /* Garante que não reutilizamos um ID de interface existente */
        if (curr->interface_id >= interface_id)
        {
            interface_id = curr->interface_id + 1;
        }
        curr = curr->next;
    }

    new_neighbor->interface_id = interface_id;
    printf("Assigned interface ID %d to neighbor %s:%s (fd %d)\n",
           interface_id, ip, port, fd);

    /* Adiciona à lista de vizinhos */
    new_neighbor->next = node.neighbors;
    node.neighbors = new_neighbor;

    /* Se este for um vizinho interno, adiciona também à lista internal_neighbors */
    if (!is_external)
    {
        /* Cria uma cópia para a lista de vizinhos internos */
        Neighbor *internal_copy = malloc(sizeof(Neighbor));
        if (internal_copy == NULL)
        {
            perror("malloc");
            /* Continua de qualquer forma, pois já adicionámos à lista principal */
        }
        else
        {
            memcpy(internal_copy, new_neighbor, sizeof(Neighbor));
            internal_copy->next = node.internal_neighbors;
            node.internal_neighbors = internal_copy;
            printf("Added %s:%s as internal neighbor\n", ip, port);
        }
    }
    else
    {
        printf("Added %s:%s as external neighbor\n", ip, port);
    }

    return 0;
}

/**
 * Remove um vizinho
 *
 * @param fd Descritor de ficheiro da ligação
 * @return 0 em caso de sucesso, -1 se o vizinho não for encontrado
 */
/**
 * Remove a neighbor from the list of neighbors
 *
 * @param fd Descritor de ficheiro da ligação
 * @return 0 em caso de sucesso, -1 se o vizinho não for encontrado
 */
int remove_neighbor(int fd)
{
    /* Procura o vizinho */
    Neighbor *prev = NULL;
    Neighbor *curr = node.neighbors;
    int is_external = 0;
    char removed_ip[INET_ADDRSTRLEN];
    char removed_port[6];

    while (curr != NULL)
    {
        if (curr->fd == fd)
        {
            /* Guarda informação do vizinho antes de o remover */
            strcpy(removed_ip, curr->ip);
            strcpy(removed_port, curr->port);

            /* Verifica se é o vizinho externo */
            if (strcmp(curr->ip, node.ext_neighbor_ip) == 0 &&
                strcmp(curr->port, node.ext_neighbor_port) == 0)
            {
                is_external = 1;
            }

            /* Remove da lista de vizinhos */
            if (prev == NULL)
            {
                node.neighbors = curr->next;
            }
            else
            {
                prev->next = curr->next;
            }

            /* Remove também da lista de vizinhos internos se estiver lá */
            Neighbor *prev_internal = NULL;
            Neighbor *curr_internal = node.internal_neighbors;

            while (curr_internal != NULL)
            {
                if (curr_internal->fd == fd)
                {
                    if (prev_internal == NULL)
                    {
                        node.internal_neighbors = curr_internal->next;
                    }
                    else
                    {
                        prev_internal->next = curr_internal->next;
                    }

                    free(curr_internal);
                    break;
                }

                prev_internal = curr_internal;
                curr_internal = curr_internal->next;
            }

            /* Fecha o socket e liberta a memória */
            close(curr->fd);
            free(curr);

            /* Trata a saída do nó com base no protocolo */
            if (is_external)
            {
                printf("External neighbor %s:%s disconnected\n", removed_ip, removed_port);

                /* O protocolo diz:
                   1. Se o nó não é a sua própria salvaguarda, liga-se ao nó de salvaguarda
                   2. Se o nó é a sua própria salvaguarda e tem vizinhos internos,
                      escolhe um como novo vizinho externo
                   3. Se o nó é a sua própria salvaguarda e não tem vizinhos internos,
                      torna-se autónomo
                */

                if (strcmp(node.safe_node_ip, node.ip) != 0)
                {
                    /* Não é auto-salvaguardado - liga-se ao nó de salvaguarda */
                    printf("Connecting to safety node %s:%s\n",
                           node.safe_node_ip, node.safe_node_port);

                    int new_fd = connect_to_node(node.safe_node_ip, node.safe_node_port);
                    if (new_fd < 0)
                    {
                        printf("Failed to connect to safety node %s:%s\n",
                               node.safe_node_ip, node.safe_node_port);
                        return -1;
                    }

                    /* Atualiza o vizinho externo */
                    strcpy(node.ext_neighbor_ip, node.safe_node_ip);
                    strcpy(node.ext_neighbor_port, node.safe_node_port);

                    /* NEW: Propagate safety node information */
                    printf("SAFETY: External neighbor updated due to loss of connection, propagating safety node information\n");
                    update_and_propagate_safety_node();

                    /* Adiciona como vizinho */
                    add_neighbor(node.safe_node_ip, node.safe_node_port, new_fd, 1);

                    /* Envia mensagem ENTRY */
                    char message[MAX_BUFFER];
                    snprintf(message, MAX_BUFFER, "ENTRY %s %s\n", node.ip, node.port);

                    if (write(new_fd, message, strlen(message)) < 0)
                    {
                        perror("write");
                        return -1;
                    }

                    /* Atualiza todos os vizinhos internos com nova salvaguarda */
                    Neighbor *n = node.internal_neighbors;
                    while (n != NULL)
                    {
                        char safe_msg[MAX_BUFFER];
                        snprintf(safe_msg, MAX_BUFFER, "SAFE %s %s\n",
                                 node.ext_neighbor_ip, node.ext_neighbor_port);

                        if (write(n->fd, safe_msg, strlen(safe_msg)) < 0)
                        {
                            perror("write");
                        }

                        n = n->next;
                    }
                }
                else if (node.internal_neighbors != NULL)
                {
                    /* Auto-salvaguardado com vizinhos internos */
                    printf("Self is safety node with internal neighbors, choosing new external\n");

                    /* Escolhe o primeiro vizinho interno como novo vizinho externo */
                    Neighbor *chosen = node.internal_neighbors;

                    /* Define como novo vizinho externo */
                    strcpy(node.ext_neighbor_ip, chosen->ip);
                    strcpy(node.ext_neighbor_port, chosen->port);
                    printf("Selected %s:%s as new external neighbor\n",
                           chosen->ip, chosen->port);

                    /* NEW: Propagate safety node information */
                    printf("SAFETY: External neighbor updated due to self-safety, propagating safety node information\n");
                    update_and_propagate_safety_node();

                    /* Envia mensagem ENTRY */
                    char message[MAX_BUFFER];
                    snprintf(message, MAX_BUFFER, "ENTRY %s %s\n", node.ip, node.port);

                    if (write(chosen->fd, message, strlen(message)) < 0)
                    {
                        perror("write");
                        return -1;
                    }

                    /* Envia mensagem SAFE para todos os outros vizinhos internos */
                    Neighbor *n = node.internal_neighbors;
                    while (n != NULL)
                    {
                        if (n != chosen)
                        {
                            char safe_msg[MAX_BUFFER];
                            snprintf(safe_msg, MAX_BUFFER, "SAFE %s %s\n",
                                     node.ext_neighbor_ip, node.ext_neighbor_port);

                            if (write(n->fd, safe_msg, strlen(safe_msg)) < 0)
                            {
                                perror("write");
                            }
                        }

                        n = n->next;
                    }
                }
                else
                {
                    /* Auto-salvaguardado sem vizinhos internos */
                    printf("Self is safety node with no internal neighbors, becoming standalone\n");

                    /* Torna-se nó autónomo */
                    strcpy(node.ext_neighbor_ip, node.ip);
                    strcpy(node.ext_neighbor_port, node.port);
                    strcpy(node.safe_node_ip, node.ip);
                    strcpy(node.safe_node_port, node.port);

                    /* NEW: No need to propagate since there are no internal neighbors */
                    printf("SAFETY: Became standalone node, no need to propagate safety node information\n");
                }
            }

            return 0;
        }

        prev = curr;
        curr = curr->next;
    }

    return -1; /* Vizinho não encontrado */
}

/**
 * Verifica entradas de interesse que expiraram o tempo limite
 * Versão melhorada com gestão adequada do ciclo de vida dos interesses
 */
void check_interest_timeouts()
{
    time_t current_time = time(NULL);
    InterestEntry *prev = NULL;
    InterestEntry *entry = node.interest_table;

    while (entry != NULL)
    {
        if (difftime(current_time, entry->timestamp) > INTEREST_TIMEOUT)
        {
            printf("Interest for %s has timed out\n", entry->name);

            /* Envia NOOBJECT para todas as interfaces RESPONSE */
            for (int i = 1; i < MAX_INTERFACE; i++)
            { // Começa em 1 para ignorar ligações de saída
                if (entry->interface_states[i] == RESPONSE)
                {
                    /* Procura o vizinho com este ID de interface */
                    for (Neighbor *n = node.neighbors; n != NULL; n = n->next)
                    {
                        if (n->interface_id == i)
                        {
                            send_noobject_message(n->fd, entry->name);
                            break;
                        }
                    }
                }
            }

            /* Verifica se a UI local está à espera deste objeto */
            if (entry->interface_states[MAX_INTERFACE - 1] == RESPONSE)
            {
                printf("Object %s not found for local request (timeout)\n", entry->name);
            }

            /* Remove a entrada */
            if (prev == NULL)
            {
                node.interest_table = entry->next;
            }
            else
            {
                prev->next = entry->next;
            }

            InterestEntry *to_free = entry;
            entry = entry->next;
            free(to_free);
        }
        else
        {
            prev = entry;
            entry = entry->next;
        }
    }
}

/**
 * Trata respostas do servidor de registo
 * Modificado para garantir a correspondência de resposta de rede correta
 */
void handle_registration_response()
{
    char buffer[MAX_BUFFER];
    struct sockaddr_in server_addr;
    socklen_t addr_len = sizeof(server_addr);

    int bytes_received = recvfrom(node.reg_fd, buffer, MAX_BUFFER - 1, 0,
                                  (struct sockaddr *)&server_addr, &addr_len);

    if (bytes_received <= 0)
    {
        if (bytes_received < 0)
        {
            perror("recvfrom");
        }
        return;
    }

    buffer[bytes_received] = '\0';
    printf("Received from server: %s\n", buffer);

    /* Verifica o tipo de resposta */
    if (strncmp(buffer, "NODESLIST", 9) == 0)
    {
        /* Processa resposta NODESLIST */
        char response_net[4];
        if (sscanf(buffer, "NODESLIST %3s", response_net) == 1)
        {
            printf("Processing NODESLIST for network %s\n", response_net);
            process_nodeslist_response(buffer);
        }
        else
        {
            printf("Invalid NODESLIST response format\n");
        }
    }
    else if (strcmp(buffer, "OKREG") == 0)
    {
        /* Registo bem-sucedido */
        printf("Registration successful\n");
    }
    else if (strcmp(buffer, "OKUNREG") == 0)
    {
        /* Cancelamento de registo bem-sucedido */
        printf("Unregistration successful\n");
    }
    else
    {
        printf("Unknown response from registration server: %s\n", buffer);
    }
}
