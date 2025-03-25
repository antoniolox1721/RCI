/*
 * Implementação de Rede de Dados Identificados por Nome (NDN)
 * Redes de Computadores e Internet - 2024/2025
 *
 * network.c - Implementação das funções de protocolos de rede
 */

#include "network.h"
#include "objects.h"
#include "debug_utils.h"
#include "commands.h"  /* For cmd_show_interest_table() function */

/**
 * Enhanced display_interest_table_update with detailed information
 */
void display_interest_table_update(const char* action, const char* name) {
    // Determine color based on action type
    const char* action_color = COLOR_MAGENTA;
    if (strstr(action, "Not Found") || 
        strstr(action, "Error") || 
        strstr(action, "Failed") || 
        strstr(action, "Removed") || 
        strstr(action, "TIMEOUT") || 
        strstr(action, "No Entry") ||
        strstr(action, "All Paths Closed")) {
        action_color = COLOR_RED;
    } else if (strstr(action, "Found") || 
               strstr(action, "OBJECT")) {
        action_color = COLOR_GREEN;
    } else if (strstr(action, "INTEREST")) {
        action_color = COLOR_CYAN;
    }
    
    printf("\n%s%s┌───────────────────────────────────────────────────┐%s\n", COLOR_BOLD, action_color, COLOR_RESET);
    printf("%s%s│ INTEREST TABLE UPDATE: %-30s │%s\n", COLOR_BOLD, action_color, action, COLOR_RESET);
    printf("%s%s└───────────────────────────────────────────────────┘%s\n", COLOR_BOLD, action_color, COLOR_RESET);
    
    if (name != NULL) {
        printf("Object: %s%s%s\n\n", COLOR_CYAN, name, COLOR_RESET);
    }
    cmd_show_interest_table();
}
/**
 * Reinicia o interesse para um objeto, removendo a sua entrada.
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
 * Atualiza as informações de um vizinho com o porto de escuta correto.
 * Esta função é chamada quando recebemos uma mensagem ENTRY e precisamos
 * de atualizar o porto.
 *
 * @param fd O descritor de ficheiro da ligação
 * @param ip O endereço IP da mensagem ENTRY
 * @param port O porto de escuta da mensagem ENTRY
 * @return 0 em caso de sucesso, -1 se o vizinho não for encontrado
 */
int update_neighbor_info(int fd, char *ip, char *port)
{
    Neighbor *curr = node.neighbors;
    int found = 0;

    /* Procura o vizinho com este fd */
    while (curr != NULL)
    {
        if (curr->fd == fd)
        {
            /* Atualiza o porto para o que foi especificado na mensagem ENTRY */
            if (strcmp(curr->port, port) != 0)
            {
                printf("Updating neighbor port from %s to %s for connection fd %d\n",
                       curr->port, port, fd);
                strcpy(curr->port, port);
            }

            /* Adiciona como vizinho interno se não for já */
            int already_internal = 0;
            Neighbor *internal = node.internal_neighbors;
            while (internal != NULL)
            {
                if (internal->fd == fd)
                {
                    /* Atualiza o porto do vizinho interno também */
                    if (strcmp(internal->port, port) != 0)
                    {
                        strcpy(internal->port, port);
                    }
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
                    memcpy(internal_copy, curr, sizeof(Neighbor));
                    internal_copy->next = node.internal_neighbors;
                    node.internal_neighbors = internal_copy;
                    printf("Added %s:%s as internal neighbor\n", ip, port);
                }
            }

            found = 1;
            break;
        }
        curr = curr->next;
    }

    if (!found)
    {
        printf("Error: Could not find neighbor with fd %d to update\n", fd);
        return -1;
    }

    return 0;
}

/**
 * Inicializa uma entrada de interesse.
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
 * Atualiza e propaga informações de nó de salvaguarda a todos os vizinhos internos.
 * Deve ser chamada sempre que a topologia muda de forma a afetar nós de salvaguarda.
 */
void update_and_propagate_safety_node()
{
    printf("SAFETY: Updating and propagating safety node information\n");
    printf("SAFETY: Current external neighbor: %s:%s\n", node.ext_neighbor_ip, node.ext_neighbor_port);
    printf("SAFETY: Current safety node: %s:%s\n", node.safe_node_ip, node.safe_node_port);

    /* First, ensure all internal neighbors have updated safety information */
    int sent_count = 0;
    Neighbor *n = node.internal_neighbors;
    while (n != NULL)
    {
        /* REMOVED interface ID check to ensure ALL internal neighbors get the update */
        /* Create SAFE message with EXTERNAL NEIGHBOR as safety node for our internal neighbors */
        char safe_msg[MAX_BUFFER];
        snprintf(safe_msg, MAX_BUFFER, "SAFE %s %s\n",
                 node.ext_neighbor_ip, node.ext_neighbor_port);

        printf("SAFETY: Sending updated SAFE message to %s:%s (fd: %d, interface: %d): %s",
               n->ip, n->port, n->fd, n->interface_id, safe_msg);

        /* Send message and handle errors */
        if (write(n->fd, safe_msg, strlen(safe_msg)) < 0)
        {
            perror("write");
            printf("SAFETY: Failed to send SAFE message to %s:%s (fd: %d, interface: %d)\n",
                   n->ip, n->port, n->fd, n->interface_id);
            /* Continue with other neighbors even if one fails */
        }
        else
        {
            printf("SAFETY: Successfully sent SAFE message to %s:%s (fd: %d, interface: %d)\n",
                   n->ip, n->port, n->fd, n->interface_id);
            sent_count++;
        }

        n = n->next;
    }

    printf("SAFETY: Finished propagating safety node information to %d internal neighbors\n", sent_count);
}


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

            /* Nota: Não usamos mais o porto da ligação para identificação,
               mas ainda precisamos dele para fins de registo */
            char client_port[6];
            snprintf(client_port, 6, "%d", ntohs(client_addr.sin_port));

            printf("New connection from %s:%s\n", client_ip, client_port);

            /* Armazena temporariamente esta ligação com o seu porto de origem
               até recebermos uma mensagem ENTRY com o porto de escuta real */
            /* Usamos 0 para is_external pois ainda não sabemos */
            add_neighbor(client_ip, client_port, new_fd, 0);

            /* Atualiza max_fd se necessário */
            if (new_fd > node.max_fd)
            {
                node.max_fd = new_fd;
            }
        }
    }

    /* Verifica mensagens de ligações existentes */
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
                /* Append new data to existing buffer */
                if (curr->buffer_len + bytes_received < MAX_BUFFER) {
                    memcpy(curr->buffer + curr->buffer_len, buffer, bytes_received);
                    curr->buffer_len += bytes_received;
                    curr->buffer[curr->buffer_len] = '\0';
                } else {
                    /* Buffer overflow scenario - discard oldest data */
                    printf("Warning: Buffer overflow, discarding oldest data\n");
                    int space_needed = (curr->buffer_len + bytes_received) - (MAX_BUFFER - 1);
                    if (space_needed > 0 && space_needed < curr->buffer_len) {
                        memmove(curr->buffer, curr->buffer + space_needed, curr->buffer_len - space_needed);
                        curr->buffer_len -= space_needed;
                        memcpy(curr->buffer + curr->buffer_len, buffer, bytes_received);
                        curr->buffer_len += bytes_received;
                    } else {
                        /* If can't make enough space, just use new data */
                        memcpy(curr->buffer, buffer, bytes_received);
                        curr->buffer_len = bytes_received;
                    }
                    curr->buffer[curr->buffer_len] = '\0';
                }
                
                printf("Received %d bytes from %s:%s, buffer now: %s\n", bytes_received, curr->ip, curr->port, curr->buffer);

                /* Process each complete message in the buffer */
                char *message_start = curr->buffer;
                char *message_end;
                
                while ((message_end = strchr(message_start, '\n')) != NULL) {
                    /* Extract the current message */
                    *message_end = '\0';  /* Temporarily replace newline with null */
                    
                    /* Process this single message */
                    printf("Processing message: %s\n", message_start);
                    
                    /* Determine message type and process it */
                    if (strncmp(message_start, "INTEREST ", 9) == 0) {
                        char name[MAX_OBJECT_NAME + 1] = {0};
                        if (sscanf(message_start, "INTEREST %100s", name) == 1) {
                            handle_interest_message(curr->fd, name);
                        }
                    }
                    else if (strncmp(message_start, "OBJECT ", 7) == 0) {
                        char name[MAX_OBJECT_NAME + 1] = {0};
                        if (sscanf(message_start, "OBJECT %100s", name) == 1) {
                            handle_object_message(curr->fd, name);
                        }
                    }
                    else if (strncmp(message_start, "NOOBJECT ", 9) == 0) {
                        char name[MAX_OBJECT_NAME + 1] = {0};
                        if (sscanf(message_start, "NOOBJECT %100s", name) == 1) {
                            handle_noobject_message(curr->fd, name);
                        }
                    }
                    else if (strncmp(message_start, "ENTRY ", 6) == 0) {
                        char sender_ip[INET_ADDRSTRLEN] = {0};
                        char sender_port[6] = {0};

                        if (sscanf(message_start, "ENTRY %s %s", sender_ip, sender_port) == 2) {
                            printf("Received ENTRY message from %s:%s\n", sender_ip, sender_port);

                            /* Update the neighbor info with correct listening port */
                            int port_updated = 0;
                            if (port_updated){}
                            for (Neighbor *n = node.neighbors; n != NULL; n = n->next) {
                                if (n->fd == curr->fd) {
                                    if (strcmp(n->port, sender_port) != 0) {
                                        printf("Updating neighbor port from %s to %s for fd %d\n",
                                               n->port, sender_port, curr->fd);
                                        strcpy(n->port, sender_port);
                                        port_updated = 1;
                                    }
                                    break;
                                }
                            }

                            /* Check if this neighbor is already in internal list with correct port */
                            int already_internal = 0;
                            Neighbor *internal = node.internal_neighbors;
                            while (internal != NULL) {
                                /* Only check IP and correct port, ignoring ephemeral ports */
                                if (strcmp(internal->ip, sender_ip) == 0 && 
                                    strcmp(internal->port, sender_port) == 0) {
                                    already_internal = 1;
                                    break;
                                }
                                internal = internal->next;
                            }

                            /* Always update or add to internal neighbors with correct port */
                            if (!already_internal) {
                                /* If we have an old entry with same IP but different port, update it */
                                int updated_existing = 0;
                                internal = node.internal_neighbors;
                                while (internal != NULL) {
                                    if (strcmp(internal->ip, sender_ip) == 0) {
                                        /* Update the port */
                                        printf("Updating internal neighbor from %s:%s to %s:%s\n", 
                                               internal->ip, internal->port, 
                                               sender_ip, sender_port);
                                        strcpy(internal->port, sender_port);
                                        updated_existing = 1;
                                        break;
                                    }
                                    internal = internal->next;
                                }
                                
                                /* If no existing entry found, add new one */
                                if (!updated_existing) {
                                    Neighbor *internal_copy = malloc(sizeof(Neighbor));
                                    if (internal_copy != NULL) {
                                        strcpy(internal_copy->ip, sender_ip);
                                        strcpy(internal_copy->port, sender_port);
                                        internal_copy->fd = curr->fd;
                                        internal_copy->interface_id = curr->interface_id;
                                        internal_copy->next = node.internal_neighbors;
                                        node.internal_neighbors = internal_copy;
                                        printf("Added %s:%s as internal neighbor\n", sender_ip, sender_port);
                                    }
                                }
                            }

                            /* If we don't have an external neighbor yet, set this node as our external neighbor */
                            int need_to_send_entry = 0;
                            if (strlen(node.ext_neighbor_ip) == 0) {
                                printf("Setting external neighbor to %s:%s\n", sender_ip, sender_port);
                                strcpy(node.ext_neighbor_ip, sender_ip);
                                strcpy(node.ext_neighbor_port, sender_port);
                                
                                /* Only send an ENTRY back if we don't have an external neighbor */
                                /* (special case for the first two nodes in network) */
                                need_to_send_entry = 1;
                                printf("First/second node special case: Will send ENTRY response\n");
                            } else {
                                printf("Already have external neighbor, not sending ENTRY response\n");
                            }

                            /* Only send ENTRY if this is a special case (first two nodes) */
                            if (need_to_send_entry) {
                                /* Send our ENTRY message */
                                char entry_msg[MAX_BUFFER];
                                snprintf(entry_msg, MAX_BUFFER, "ENTRY %s %s\n", node.ip, node.port);
                                
                                printf("Sending ENTRY message: %s", entry_msg);
                                if (write(curr->fd, entry_msg, strlen(entry_msg)) < 0) {
                                    perror("write");
                                }
                            }

                            /* Always send SAFE message with external neighbor info */
                            char safe_msg[MAX_BUFFER];
                            /* If we don't have an external neighbor yet, use self as safety node */
                            if (strlen(node.ext_neighbor_ip) == 0) {
                                snprintf(safe_msg, MAX_BUFFER, "SAFE %s %s\n", node.ip, node.port);
                            } else {
                                snprintf(safe_msg, MAX_BUFFER, "SAFE %s %s\n", 
                                         node.ext_neighbor_ip, node.ext_neighbor_port);
                            }
                            
                            printf("Sending SAFE message: %s", safe_msg);
                            if (write(curr->fd, safe_msg, strlen(safe_msg)) < 0) {
                                perror("write");
                            }
                        }
                        else {
                            printf("Malformed ENTRY message: %s\n", message_start);
                        }
                    }
                    else if (strncmp(message_start, "SAFE ", 5) == 0) {
                        char safe_ip[INET_ADDRSTRLEN] = {0};
                        char safe_port[6] = {0};

                        if (sscanf(message_start, "SAFE %s %s", safe_ip, safe_port) == 2) {
                            printf("Received SAFE message, safety node info: %s:%s\n", safe_ip, safe_port);

                            /* Always update safety node info exactly as received */
                            strcpy(node.safe_node_ip, safe_ip);
                            strcpy(node.safe_node_port, safe_port);
                            printf("Updated safety node to: %s:%s\n", safe_ip, safe_port);
                        }
                        else {
                            printf("Malformed SAFE message: %s\n", message_start);
                        }
                    }
                    else {
                        printf("Unknown message type: %s\n", message_start);
                    }
                    
                    /* Restore newline for logs, but advance past it for next message */
                    *message_end = '\n';
                    message_start = message_end + 1;
                }
                
                /* Save any remaining partial message for next time */
                if (message_start < curr->buffer + curr->buffer_len) {
                    int remaining_len = curr->buffer_len - (message_start - curr->buffer);
                    memmove(curr->buffer, message_start, remaining_len);
                    curr->buffer_len = remaining_len;
                    curr->buffer[curr->buffer_len] = '\0';
                    printf("Saved partial message for next read: %s\n", curr->buffer);
                } else {
                    /* No remaining partial message */
                    curr->buffer_len = 0;
                    curr->buffer[0] = '\0';
                }
            }
        }

        curr = next;
    }
}

/**
 * Envia uma mensagem de registo ao servidor de registo.
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

    // Garante que a conversão do endereço IP seja bem-sucedida
    if (inet_pton(AF_INET, node.reg_server_ip, &server_addr.sin_addr) != 1)
    {
        printf("Invalid registration server IP address: %s\n", node.reg_server_ip);
        return -1;
    }

    char message[MAX_BUFFER];
    snprintf(message, MAX_BUFFER, "REG %s %s %s", net, ip, port);

    printf("Sending registration to %s:%s: %s\n", node.reg_server_ip, node.reg_server_port, message);

    // Define um timeout para a operação de receção
    struct timeval timeout;
    timeout.tv_sec = 5; // 5 segundos timeout
    timeout.tv_usec = 0;
    if (setsockopt(node.reg_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0)
    {
        perror("setsockopt receive timeout");
        // Continua de qualquer forma, apenas sem timeout
    }

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

    if (strcmp(buffer, "OKREG") != 0)
    {
        printf("Unexpected response from registration server: %s\n", buffer);
        return -1;
    }

    return 0;
}

/**
 * Envia uma mensagem de remoção de registo ao servidor de registo.
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

    // Garante que a conversão do endereço IP seja bem-sucedida
    if (inet_pton(AF_INET, node.reg_server_ip, &server_addr.sin_addr) != 1)
    {
        printf("Invalid registration server IP address: %s\n", node.reg_server_ip);
        return -1;
    }

    char message[MAX_BUFFER];
    snprintf(message, MAX_BUFFER, "UNREG %s %s %s", net, ip, port);

    printf("Sending unregistration to %s:%s: %s\n", node.reg_server_ip, node.reg_server_port, message);

    // Define um timeout para a operação de receção
    struct timeval timeout;
    timeout.tv_sec = 5; // 5 segundos timeout
    timeout.tv_usec = 0;
    if (setsockopt(node.reg_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0)
    {
        perror("setsockopt receive timeout");
        // Continua de qualquer forma, apenas sem timeout
    }

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

    if (strcmp(buffer, "OKUNREG") != 0)
    {
        printf("Unexpected response from registration server: %s\n", buffer);
        return -1;
    }

    return 0;
}

/**
 * Envia um pedido de nós ao servidor de registo.
 * Modificado para garantir a formatação correta do ID de rede.
 *
 * @param net ID da rede (três dígitos)
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int send_nodes_request(char *net)
{
    // Valida o formato do ID de rede
    if (strlen(net) != 3 || !isdigit(net[0]) || !isdigit(net[1]) || !isdigit(net[2]))
    {
        printf("Invalid network ID format: %s. Must be exactly 3 digits.\n", net);
        return -1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(node.reg_server_port));

    // Garante que a conversão do endereço IP seja bem-sucedida
    if (inet_pton(AF_INET, node.reg_server_ip, &server_addr.sin_addr) != 1)
    {
        printf("Invalid registration server IP address: %s\n", node.reg_server_ip);
        return -1;
    }

    /* Garante que estamos a usar o formato de ID de rede exato */
    char message[MAX_BUFFER];
    snprintf(message, MAX_BUFFER, "NODES %s", net);

    printf("Sending request: %s to registration server %s:%s\n",
           message, node.reg_server_ip, node.reg_server_port);

    // Define um timeout para a operação
    struct timeval timeout;
    timeout.tv_sec = 5; // 5 segundos timeout
    timeout.tv_usec = 0;
    if (setsockopt(node.reg_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0)
    {
        perror("setsockopt receive timeout");
        // Continua de qualquer forma, apenas sem timeout
    }

    if (sendto(node.reg_fd, message, strlen(message), 0,
               (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("sendto");
        return -1;
    }

    return 0;
}

/**
 * Processa a resposta NODESLIST do servidor de registo.
 * Melhorado para lidar corretamente com respostas de rede mistas e verificar a adesão à rede.
 *
 * @param buffer Buffer contendo a resposta NODESLIST
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
/**
 * Fixed process_nodeslist_response function to correctly handle standalone nodes
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

    char requested_net[4] = {0};
    if (sscanf(line, "NODESLIST %3s", requested_net) != 1)
    {
        printf("Invalid NODESLIST response: %s\n", line);
        return -1;
    }

    // Valida o formato do ID de rede
    if (strlen(requested_net) != 3 || !isdigit(requested_net[0]) ||
        !isdigit(requested_net[1]) || !isdigit(requested_net[2]))
    {
        printf("Invalid network ID in response: %s\n", requested_net);
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
            if (strcmp(node_ports[node_count], "0") == 0 || strcmp(node_ips[node_count], "0.0.0.0") == 0)
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
        else
        {
            // Trata entrada de nó mal formatada
            printf("Malformed node entry in NODESLIST: %s\n", line);
            // Continua o processamento em vez de falhar
        }
    }

    printf("Received %d potential nodes from registration server\n", node_count);

    /* Se não houver nós, cria uma nova rede */
    if (node_count == 0)
    {
        printf("%sNo valid nodes found in network %s, creating new network as standalone node%s\n", 
               COLOR_GREEN, requested_net, COLOR_RESET);

        /* Regista-se na rede */
        if (send_reg_message(requested_net, node.ip, node.port) < 0)
        {
            printf("Failed to register with the network.\n");
            return -1;
        }

        /* Define o ID da rede e marca como in_network */
        node.network_id = atoi(requested_net);
        node.in_network = 1;

        /* Initially, standalone node has no external neighbor */
        memset(node.ext_neighbor_ip, 0, INET_ADDRSTRLEN);
        memset(node.ext_neighbor_port, 0, 6);

        /* Initially, standalone node has no safety node */
        memset(node.safe_node_ip, 0, INET_ADDRSTRLEN);
        memset(node.safe_node_port, 0, 6);

        printf("%sCreated and joined network %s as standalone node - waiting for connections%s\n", 
               COLOR_GREEN, requested_net, COLOR_RESET);
        return 0;
    }

    /* Tenta ligar-se a um nó aleatório da lista */
    // Usa o tempo atual como semente para o gerador de números aleatórios
    srand(time(NULL));
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

    /* Define o vizinho externo - usa o porto de escuta especificado, não o porto da ligação */
    strcpy(node.ext_neighbor_ip, chosen_ip);
    strcpy(node.ext_neighbor_port, chosen_port);

    /* Adiciona o nó como vizinho externo - usa o porto de escuta especificado */
    add_neighbor(chosen_ip, chosen_port, fd, 1);

    /* Envia mensagem ENTRY com o nosso porto de escuta, não o porto da ligação */
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
 * Envia uma mensagem ENTRY para um nó.
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
 * Envia uma mensagem de objeto para um nó.
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
    return 0;
}

/**
 * Envia uma mensagem NOOBJECT para um nó.
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
 * Processa uma mensagem de interesse recebida.
 * Versão corrigida com melhor tratamento de erros.
 *
 * @param fd Descritor de ficheiro da ligação
 * @param name Nome do objeto pretendido
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
/**
 * Enhanced handle_interest_message function with better interface information
 */
int handle_interest_message(int fd, char *name)
{
    /* Obtém o ID de interface para este descritor de ficheiro */
    int interface_id = -1;
    Neighbor *curr = node.neighbors;
    char neighbor_info[50] = "Unknown";
    
    while (curr != NULL)
    {
        if (curr->fd == fd)
        {
            interface_id = curr->interface_id;
            snprintf(neighbor_info, sizeof(neighbor_info), "%s:%s (if:%d)", 
                    curr->ip, curr->port, curr->interface_id);
            break;
        }
        curr = curr->next;
    }

    if (interface_id < 0)
    {
        printf("%sInterface ID not found for fd %d%s\n", COLOR_RED, fd, COLOR_RESET);
        return -1;
    }

    /* Ignora processamento para ID de interface 0 (ligações de saída) */
    if (interface_id == 0)
    {
        printf("%sIgnoring interest from outgoing connection (interface 0)%s\n", 
               COLOR_YELLOW, COLOR_RESET);
        return 0;
    }

    printf("Received interest for %s on interface %d from %s\n", name, interface_id, neighbor_info);

    /* Verifica se temos o objeto localmente */
    if (find_object(name) >= 0)
    {
        printf("%sFound object %s locally in objects list, sending back%s\n", 
               COLOR_GREEN, name, COLOR_RESET);
        display_interest_table_update("INTEREST - Object Found Locally", name);
        return send_object_message(fd, name);
    }
    else if (find_in_cache(name) >= 0)
    {
        printf("%sFound object %s locally in cache, sending back%s\n", 
               COLOR_GREEN, name, COLOR_RESET);
        display_interest_table_update("INTEREST - Object Found In Cache", name);
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
    
    /* Create more informative display message */
    char detailed_message[100];
    snprintf(detailed_message, sizeof(detailed_message), 
            "INTEREST - From %s", neighbor_info);
            
    display_interest_table_update(detailed_message, name);

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
        printf("%sAlready forwarding interest for %s%s\n", 
               COLOR_YELLOW, name, COLOR_RESET);
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
                printf("Forwarded interest for %s to interface %d (%s:%s)\n", 
                       name, curr->interface_id, curr->ip, curr->port);
            }
        }
        curr = curr->next;
    }

    if (forwarded == 0)
    {
        printf("%sNo neighbors to forward interest to, sending NOOBJECT%s\n", 
               COLOR_RED, COLOR_RESET);
        return send_noobject_message(fd, name);
    }

    /* Adicionar a chamada para mostrar a tabela de interesses após encaminhar */
    if (forwarded > 0) {
        char forward_msg[100];
        snprintf(forward_msg, sizeof(forward_msg), 
                "INTEREST - From %s - Fwd: %d", neighbor_info, forwarded);
        display_interest_table_update(forward_msg, name);
    }

    /* Atualiza o timestamp para iniciar o temporizador de timeout */
    entry->timestamp = time(NULL);

    return 0;
}

/**
 * Processa uma mensagem de objeto recebida.
 *
 * @param fd Descritor de ficheiro da ligação
 * @param name Nome do objeto recebido
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
/**
 * Enhanced handle_object_message function with error highlighting
 */
/**
 * Enhanced handle_object_message function with better interface information
 */
int handle_object_message(int fd, char *name)
{
    /* Obtém o ID de interface para este descritor de ficheiro */
    int interface_id = -1;
    Neighbor *curr = node.neighbors;
    char neighbor_info[50] = "Unknown";
    
    while (curr != NULL)
    {
        if (curr->fd == fd)
        {
            interface_id = curr->interface_id;
            snprintf(neighbor_info, sizeof(neighbor_info), "%s:%s (if:%d)", 
                    curr->ip, curr->port, curr->interface_id);
            break;
        }
        curr = curr->next;
    }

    if (interface_id < 0)
    {
        printf("%sInterface ID not found for fd %d%s\n", COLOR_RED, fd, COLOR_RESET);
        return -1;
    }

    /* Ignora processamento para ID de interface 0 (ligações de saída) */
    if (interface_id == 0)
    {
        printf("%sIgnoring object from outgoing connection (interface 0)%s\n", 
               COLOR_YELLOW, COLOR_RESET);
        return 0;
    }

    printf("%sReceived object %s from interface %d (fd %d)%s\n", 
           COLOR_GREEN, name, interface_id, fd, COLOR_RESET);

    /* Adiciona o objeto à cache */
    if (add_to_cache(name) < 0)
    {
        printf("%sFailed to add object %s to cache%s\n", COLOR_RED, name, COLOR_RESET);
    }
    else
    {
        printf("%sAdded object %s to cache%s\n", COLOR_GREEN, name, COLOR_RESET);
    }

    /* Procura a entrada de interesse */
    InterestEntry *entry = find_interest_entry(name);
    if (!entry)
    {
        printf("%sNo interest entry found for %s%s\n", COLOR_RED, name, COLOR_RESET);
        
        /* Create more informative display message */
        char detailed_message[100];
        snprintf(detailed_message, sizeof(detailed_message), 
                "OBJECT - No Entry - From %s", neighbor_info);
                
        display_interest_table_update(detailed_message, name);
        return 0;
    }

    /* Create a temporary array to track which file descriptors we've already forwarded to */
    int forwarded_fds[FD_SETSIZE] = {0};
    forwarded_fds[fd] = 1; /* Mark the source fd as already processed */
    int forward_count = 0;

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
                    /* Skip if we've already forwarded to this fd */
                    if (forwarded_fds[n->fd])
                    {
                        printf("%sSkipping forwarding of %s to fd %d (already processed)%s\n",
                               COLOR_YELLOW, name, n->fd, COLOR_RESET);
                    }
                    else
                    {
                        printf("%sForwarding object %s to interface %d (fd %d)%s\n",
                               COLOR_GREEN, name, i, n->fd, COLOR_RESET);
                        send_object_message(n->fd, name);
                        forwarded_fds[n->fd] = 1; /* Mark as forwarded */
                        forward_count++;
                    }
                    break;
                }
            }
        }
    }

    /* Verifica se a UI local está à espera deste objeto */
    if (entry->interface_states[MAX_INTERFACE - 1] == RESPONSE)
    {
        printf("%sObject %s found for local request%s\n", 
               COLOR_GREEN, name, COLOR_RESET);
    }

    /* Mostrar a tabela de interesses antes de remover a entrada */
    char action_msg[100];
    snprintf(action_msg, sizeof(action_msg), 
            "OBJECT - From %s - Fwd: %d", neighbor_info, forward_count);
    display_interest_table_update(action_msg, name);

    /* Remove a entrada de interesse com verificação adicional */
    int remove_result = remove_interest_entry(name);
    if (remove_result < 0)
    {
        printf("%sWarning: Interest entry for %s was not found for removal%s\n", 
               COLOR_RED, name, COLOR_RESET);
    }
    else
    {
        printf("%sSuccessfully removed interest entry for %s%s\n", 
               COLOR_GREEN, name, COLOR_RESET);
    }

    return 0;
}

/**
 * Processa uma mensagem NOOBJECT recebida.
 * Versão modificada com melhor rastreamento de estado.
 *
 * @param fd Descritor de ficheiro da ligação
 * @param name Nome do objeto não encontrado
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
/**
 * Enhanced handle_noobject_message function with error highlighting
 */
/**
 * Enhanced handle_noobject_message function with better interface information in display
 */
int handle_noobject_message(int fd, char *name)
{
    /* Obtém o ID de interface para este descritor de ficheiro */
    int interface_id = -1;
    Neighbor *curr = node.neighbors;
    char neighbor_info[50] = "Unknown";
    
    while (curr != NULL)
    {
        if (curr->fd == fd)
        {
            interface_id = curr->interface_id;
            snprintf(neighbor_info, sizeof(neighbor_info), "%s:%s (if:%d)", 
                    curr->ip, curr->port, curr->interface_id);
            break;
        }
        curr = curr->next;
    }

    if (interface_id < 0)
    {
        printf("%sInterface ID not found for fd %d%s\n", COLOR_RED, fd, COLOR_RESET);
        return -1;
    }

    /* Ignora processamento para ID de interface 0 (ligações de saída) */
    if (interface_id == 0)
    {
        printf("%sIgnoring NOOBJECT from outgoing connection (interface 0)%s\n", 
               COLOR_YELLOW, COLOR_RESET);
        return 0;
    }

    printf("Received NOOBJECT for %s from interface %d\n", name, interface_id);

    /* Procura a entrada de interesse */
    InterestEntry *entry = find_interest_entry(name);
    if (!entry)
    {
        printf("%sNo interest entry found for %s%s\n", COLOR_RED, name, COLOR_RESET);
        
        /* Create a more informative display message */
        char detailed_message[100];
        snprintf(detailed_message, sizeof(detailed_message), 
                "NOOBJECT - No Entry - From %s", neighbor_info);
                
        display_interest_table_update(detailed_message, name);
        return 0;
    }

    /* Atualiza a entrada para marcar esta interface como CLOSED */
    entry->interface_states[interface_id] = CLOSED;
    printf("Marked interface %d as CLOSED for %s\n", interface_id, name);
    
    /* Create a more informative display message */
    char detailed_message[100];
    snprintf(detailed_message, sizeof(detailed_message), 
            "NOOBJECT - From %s", neighbor_info);
            
    display_interest_table_update(detailed_message, name);

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
                printf("%sMarked invalid interface %d as CLOSED for %s%s\n", 
                       COLOR_YELLOW, i, name, COLOR_RESET);
            }
        }
    }

    if (waiting_count == 0)
    {
        /* Não há interfaces em estado WAITING, envia NOOBJECT para todas as interfaces RESPONSE */
        printf("%sNo more waiting interfaces for %s, notifying requesters%s\n", 
               COLOR_RED, name, COLOR_RESET);

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
            printf("%sObject %s not found for local request%s\n", 
                   COLOR_RED, name, COLOR_RESET);
        }

        /* Mostrar a tabela de interesses antes de remover a entrada */
        display_interest_table_update("All Paths Closed - Removing Entry", name);

        /* Remove a entrada de interesse */
        remove_interest_entry(name);
    }

    return 0;
}

/**
 * Liga-se a um nó com tratamento de erros melhorado e timeout.
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
    timeout.tv_sec = 5; /* 5 segundos de timeout */
    timeout.tv_usec = 0;

    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0)
    {
        perror("setsockopt receive timeout");
        // Continua de qualquer forma
    }

    if (setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0)
    {
        perror("setsockopt send timeout");
        // Continua de qualquer forma
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
        connect_timeout.tv_sec = 5; /* 5 segundos de timeout */
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
 * Adiciona um vizinho.
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
    new_neighbor->buffer_len = 0; /* Initialize the buffer length */

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
 * Remove um vizinho da lista de vizinhos.
 *
 * @param fd Descritor de ficheiro da ligação
 * @return 0 em caso de sucesso, -1 se o vizinho não for encontrado
 */
int remove_neighbor(int fd)
{
    /* Find the neighbor */
    Neighbor *prev = NULL;
    Neighbor *curr = node.neighbors;
    int is_external = 0;
    char removed_ip[INET_ADDRSTRLEN];
    char removed_port[6];

    while (curr != NULL)
    {
        if (curr->fd == fd)
        {
            /* Save information about the neighbor before removing it */
            strcpy(removed_ip, curr->ip);
            strcpy(removed_port, curr->port);

            /* Check if it's the external neighbor */
            if (strcmp(curr->ip, node.ext_neighbor_ip) == 0 &&
                strcmp(curr->port, node.ext_neighbor_port) == 0)
            {
                is_external = 1;
            }

            /* Remove from the neighbors list */
            if (prev == NULL)
            {
                node.neighbors = curr->next;
            }
            else
            {
                prev->next = curr->next;
            }

            /* Also remove from the internal neighbors list if it's there */
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

            /* Close the socket and free the memory */
            close(curr->fd);
            free(curr);

            /* Handle the departure of the node based on the protocol */
            if (is_external)
            {
                printf("%sExternal neighbor %s:%s disconnected%s\n", 
                       COLOR_YELLOW, removed_ip, removed_port, COLOR_RESET);

                /* Check if the safety node was the node that disconnected */
                int safety_node_disconnected = 0;
                if (strcmp(node.safe_node_ip, removed_ip) == 0 &&
                    strcmp(node.safe_node_port, removed_port) == 0)
                {
                    safety_node_disconnected = 1;
                    printf("%sWARNING: Safety node has disconnected.%s\n", 
                           COLOR_RED, COLOR_RESET);
                }

                /* Check if node is its own salvage */
                int self_is_safety = (strcmp(node.safe_node_ip, node.ip) == 0 &&
                                      strcmp(node.safe_node_port, node.port) == 0);

                if (!self_is_safety && !safety_node_disconnected)
                {
                    /* Not self-salvaged - connect to the salvage node */
                    printf("%sConnecting to safety node %s:%s%s\n",
                           COLOR_GREEN, node.safe_node_ip, node.safe_node_port, COLOR_RESET);

                    int new_fd = connect_to_node(node.safe_node_ip, node.safe_node_port);
                    if (new_fd < 0)
                    {
                        printf("%sFailed to connect to safety node %s:%s%s\n",
                               COLOR_RED, node.safe_node_ip, node.safe_node_port, COLOR_RESET);
                        return -1;
                    }

                    /* Update the external neighbor */
                    strcpy(node.ext_neighbor_ip, node.safe_node_ip);
                    strcpy(node.ext_neighbor_port, node.safe_node_port);

                    /* Add as neighbor */
                    add_neighbor(node.safe_node_ip, node.safe_node_port, new_fd, 1);

                    /* Send ENTRY message */
                    char message[MAX_BUFFER];
                    snprintf(message, MAX_BUFFER, "ENTRY %s %s\n", node.ip, node.port);

                    if (write(new_fd, message, strlen(message)) < 0)
                    {
                        perror("write");
                        return -1;
                    }

                    /* Call the updated function to propagate new safety node info */
                    update_and_propagate_safety_node();
                }
                else if (node.internal_neighbors != NULL)
                {
                    /* Self-safety or safety node disconnected, and has internal neighbors */
                    printf("%sExternal neighbor is disconnected, and node has internal neighbors%s\n",
                           COLOR_YELLOW, COLOR_RESET);
                    printf("%sChoosing new external neighbor from internal neighbors%s\n",
                           COLOR_GREEN, COLOR_RESET);

                    /* Choose first internal neighbor as new external neighbor */
                    Neighbor *chosen = node.internal_neighbors;

                    /* Set as new external neighbor */
                    strcpy(node.ext_neighbor_ip, chosen->ip);
                    strcpy(node.ext_neighbor_port, chosen->port);

                    /* Always update safety node to self when reconfiguring */
                    strcpy(node.safe_node_ip, node.ip);
                    strcpy(node.safe_node_port, node.port);
                    printf("%sUpdated safety node to self: %s:%s%s\n", 
                           COLOR_GREEN, node.ip, node.port, COLOR_RESET);

                    printf("%sSelected %s:%s as new external neighbor%s\n",
                           COLOR_GREEN, chosen->ip, chosen->port, COLOR_RESET);

                    /* Send ENTRY message */
                    char message[MAX_BUFFER];
                    snprintf(message, MAX_BUFFER, "ENTRY %s %s\n", node.ip, node.port);

                    if (write(chosen->fd, message, strlen(message)) < 0)
                    {
                        perror("write");
                        return -1;
                    }

                    /* Call the updated function to propagate new safety node info */
                    update_and_propagate_safety_node();
                }
                else
                {
                    /* Self-safety with no internal neighbors - last node in network */
                    printf("%sLast node remaining in network, becoming standalone%s\n",
                           COLOR_YELLOW, COLOR_RESET);

                    /* Clear external neighbor - ready for new connections */
                    memset(node.ext_neighbor_ip, 0, INET_ADDRSTRLEN);
                    memset(node.ext_neighbor_port, 0, 6);

                    /* Clear safety node for standalone nodes */
                    memset(node.safe_node_ip, 0, INET_ADDRSTRLEN);
                    memset(node.safe_node_port, 0, 6);

                    printf("%sCleared external neighbor and safety node - now standalone%s\n",
                           COLOR_GREEN, COLOR_RESET);
                }
            }

            return 0;
        }

        prev = curr;
        curr = curr->next;
    }

    return -1; /* Neighbor not found */
}

/**
 * Verifica entradas de interesse que expiraram o tempo limite.
 * Versão melhorada com gestão adequada do ciclo de vida dos interesses.
 */
/**
 * Enhanced check_interest_timeouts function with better interface information
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
            int timeout_seconds = (int)difftime(current_time, entry->timestamp);
            printf("%sInterest for %s has timed out (after %d seconds)%s\n", 
                   COLOR_RED, entry->name, timeout_seconds, COLOR_RESET);
            
            /* Count waiting interfaces for better reporting */
            int waiting_count = 0;
            for (int i = 1; i < MAX_INTERFACE; i++) {
                if (entry->interface_states[i] == WAITING) {
                    waiting_count++;
                }
            }
            
            /* Create more detailed message */
            char detailed_message[100];
            snprintf(detailed_message, sizeof(detailed_message), 
                    "INTEREST TIMEOUT - %d secs - %d waiting ifs", 
                    timeout_seconds, waiting_count);
                    
            display_interest_table_update(detailed_message, entry->name);

            /* Envia NOOBJECT para todas as interfaces RESPONSE */
            int response_count = 0;
            for (int i = 1; i < MAX_INTERFACE; i++)
            { // Começa em 1 para ignorar ligações de saída
                if (entry->interface_states[i] == RESPONSE)
                {
                    response_count++;
                    /* Procura o vizinho com este ID de interface */
                    for (Neighbor *n = node.neighbors; n != NULL; n = n->next)
                    {
                        if (n->interface_id == i)
                        {
                            printf("Sending NOOBJECT for %s to interface %d (%s:%s)\n", 
                                   entry->name, i, n->ip, n->port);
                            send_noobject_message(n->fd, entry->name);
                            break;
                        }
                    }
                }
            }

            /* Verifica se a UI local está à espera deste objeto */
            if (entry->interface_states[MAX_INTERFACE - 1] == RESPONSE)
            {
                printf("%sObject %s not found for local request (timeout)%s\n", 
                       COLOR_RED, entry->name, COLOR_RESET);
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
 * Trata respostas do servidor de registo.
 * Modificado para garantir a correspondência de resposta de rede correta.
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
            if (errno != EAGAIN && errno != EWOULDBLOCK)
            {
                perror("recvfrom");
            }
            // Sem mensagem se for apenas um timeout
        }
        return;
    }

    buffer[bytes_received] = '\0';
    printf("Received from server: %s\n", buffer);

    /* Verifica o tipo de resposta */
    if (strncmp(buffer, "NODESLIST", 9) == 0)
    {
        /* Processa resposta NODESLIST */
        char response_net[4] = {0};
        if (sscanf(buffer, "NODESLIST %3s", response_net) == 1)
        {
            printf("Processing NODESLIST for network %s\n", response_net);

            // Processa apenas se ainda não estiver numa rede
            if (!node.in_network)
            {
                process_nodeslist_response(buffer);
            }
            else
            {
                printf("Ignoring NODESLIST as already in network %03d\n", node.network_id);
            }
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