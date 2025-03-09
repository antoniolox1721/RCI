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

/*
 * Processa um comando do utilizador
 *
 * @param cmd String contendo o comando a processar
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int process_command(char *cmd)
{
    char original_cmd[MAX_CMD_SIZE];
    strncpy(original_cmd, cmd, MAX_CMD_SIZE - 1);
    original_cmd[MAX_CMD_SIZE - 1] = '\0';

    /* Ignora espaços em branco iniciais */
    while (*cmd && isspace(*cmd))
    {
        cmd++;
    }

    if (*cmd == '\0')
    {
        return 0; /* Comando vazio */
    }

    /* Extrai o nome do comando (primeira palavra) */
    char cmd_name[MAX_CMD_SIZE];
    int i = 0;
    while (cmd[i] && !isspace(cmd[i]) && i < MAX_CMD_SIZE - 1)
    {
        cmd_name[i] = tolower(cmd[i]); /* Converte para minúsculas */
        i++;
    }
    cmd_name[i] = '\0';

    /* Trata o comando "create" de forma especial */
    if (strcmp(cmd_name, "create") == 0 || strcmp(cmd_name, "c") == 0)
    {
        /* Avança para os parâmetros (após o comando e espaços) */
        char *params = cmd + i;
        while (*params && isspace(*params))
        {
            params++;
        }

        /* Verifica se há espaços no nome do objeto */
        char *space_check = params;
        while (*space_check)
        {
            if (isspace(*space_check))
            {
                printf("Invalid object name. Object names cannot contain spaces.\n");
                return -1;
            }
            space_check++;
        }

        if (*params)
        {
            return cmd_create(params);
        }
        else
        {
            printf("Usage: create (c) <name>\n");
            return -1;
        }
    }

    /* Para os restantes comandos, usa strtok para análise */
    char *token = strtok(cmd, " \n");
    /* Ignora o token do nome do comando, já processado acima */
    token = strtok(NULL, " \n");

    if (strcmp(cmd_name, "join") == 0 || strcmp(cmd_name, "j") == 0)
    {
        if (token != NULL)
        {
            return cmd_join(token);
        }
        else
        {
            printf("Usage: join (j) <net>\n");
        }
    }
    else if (strcmp(cmd_name, "direct") == 0 || strcmp(cmd_name, "dj") == 0)
    {
        char *connect_ip = token;                // Primeiro token após o comando é o IP
        char *connect_tcp = strtok(NULL, " \n"); // Segundo token é o porto TCP

        if (connect_ip != NULL && connect_tcp != NULL)
        {
            return cmd_direct_join(connect_ip, connect_tcp);
        }
        else
        {
            printf("Usage: direct join (dj) <connectIP> <connectTCP>\n");
        }
    }
    else if (strcmp(cmd_name, "delete") == 0 || strcmp(cmd_name, "dl") == 0)
    {
        if (token != NULL)
        {
            return cmd_delete(token);
        }
        else
        {
            printf("Usage: delete (dl) <name>\n");
        }
    }
    else if (strcmp(cmd_name, "retrieve") == 0 || strcmp(cmd_name, "r") == 0)
    {
        if (token != NULL)
        {
            return cmd_retrieve(token);
        }
        else
        {
            printf("Usage: retrieve (r) <name>\n");
        }
    }
    else if (strcmp(cmd_name, "show") == 0 || strcmp(cmd_name, "s") == 0)
    {
        if (token != NULL)
        {
            char what[MAX_CMD_SIZE];
            strncpy(what, token, MAX_CMD_SIZE - 1);
            what[MAX_CMD_SIZE - 1] = '\0';

            /* Converte para minúsculas */
            for (int i = 0; what[i]; i++)
            {
                what[i] = tolower(what[i]);
            }

            if (strcmp(what, "topology") == 0)
            {
                return cmd_show_topology();
            }
            else if (strcmp(what, "names") == 0)
            {
                return cmd_show_names();
            }
            else if (strcmp(what, "interest") == 0 || strcmp(what, "table") == 0)
            {
                return cmd_show_interest_table();
            }
            else
            {
                printf("Unknown show command: %s\n", what);
            }
        }
        else
        {
            printf("Usage: show <topology|names|interest>\n");
        }
    }
    /* Trata abreviaturas diretas */
    else if (strcmp(cmd_name, "st") == 0)
    {
        return cmd_show_topology();
    }
    else if (strcmp(cmd_name, "sn") == 0)
    {
        return cmd_show_names();
    }
    else if (strcmp(cmd_name, "si") == 0)
    {
        return cmd_show_interest_table();
    }
    else if (strcmp(cmd_name, "leave") == 0 || strcmp(cmd_name, "l") == 0)
    {
        return cmd_leave();
    }
    else if (strcmp(cmd_name, "exit") == 0 || strcmp(cmd_name, "x") == 0)
    {
        return cmd_exit();
    }
    else if (strcmp(cmd_name, "help") == 0 || strcmp(cmd_name, "h") == 0)
    {
        print_help();
        return 0;
    }
    else
    {
        printf("Unknown command: %s\n", cmd_name);
    }

    return -1;
}

/*
 * Imprime informações de ajuda
 * Mostra todos os comandos disponíveis e suas descrições
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

/*
 * Aderir a uma rede através do servidor de registo
 * Modificado para validar corretamente o ID da rede e processar a resposta
 */
/*
 * Aderir a uma rede através do servidor de registo
 * Modificado para validar corretamente o ID da rede e processar a resposta
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

    // Set a timeout for the receive operation
    struct timeval timeout;
    timeout.tv_sec = 5;  // 5 seconds timeout
    timeout.tv_usec = 0;
    if (setsockopt(node.reg_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        perror("setsockopt receive timeout");
        // Continue anyway, just without timeout
    }

    int bytes_received = recvfrom(node.reg_fd, buffer, MAX_BUFFER - 1, 0,
                                  (struct sockaddr *)&server_addr, &addr_len);

    if (bytes_received <= 0)
    {
        if (bytes_received < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                printf("Timeout waiting for response from registration server\n");
            } else {
                perror("recvfrom");
            }
        } else {
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

/*
 * Aderir diretamente a uma rede sem usar o servidor de registo
 */
/*
 * Aderir diretamente a uma rede sem usar o servidor de registo
 */
/*
 * Aderir diretamente a uma rede sem usar o servidor de registo
 */
int cmd_direct_join(char *connect_ip, char *connect_tcp)
{
    if (node.in_network)
    {
        printf("Already in a network. Leave first.\n");
        return -1;
    }

    /* Usa um ID de rede predefinido */
    char net[4] = "076"; // ID de rede predefinido

    /* Se connect_ip for 0.0.0.0, cria uma nova rede */
    if (strcmp(connect_ip, "0.0.0.0") == 0)
    {
        /* Regista-se na rede */
        if (send_reg_message(net, node.ip, node.port) < 0)
        {
            printf("Failed to register with the network.\n");
            return -1;
        }

        /* Define o ID da rede e marca como in_network */
        node.network_id = atoi(net);
        node.in_network = 1;

        /* Este nó é o seu próprio vizinho externo e nó de salvaguarda */
        strcpy(node.ext_neighbor_ip, node.ip);
        strcpy(node.ext_neighbor_port, node.port);
        strcpy(node.safe_node_ip, node.ip);
        strcpy(node.safe_node_port, node.port);

        printf("Created and joined network %s\n", net);
        return 0;
    }

    /* Liga-se ao nó especificado */
    int fd = connect_to_node(connect_ip, connect_tcp);
    if (fd < 0)
    {
        printf("Failed to connect to %s:%s\n", connect_ip, connect_tcp);
        return -1;
    }

    /* Define o vizinho externo */
    strcpy(node.ext_neighbor_ip, connect_ip);
    strcpy(node.ext_neighbor_port, connect_tcp);

    /* Adiciona o nó como vizinho externo - use the specified port from the command */
    add_neighbor(connect_ip, connect_tcp, fd, 1);

    /* Envia mensagem ENTRY */
    if (send_entry_message(fd, node.ip, node.port) < 0)
    {
        printf("Failed to send ENTRY message.\n");
        close(fd);
        return -1;
    }

    /* Regista-se na rede */
    if (send_reg_message(net, node.ip, node.port) < 0)
    {
        printf("Failed to register with the network.\n");
        close(fd);
        return -1;
    }

    /* Define o ID da rede e marca como in_network */
    node.network_id = atoi(net);
    node.in_network = 1;

    printf("Joined network %s through %s:%s\n", net, connect_ip, connect_tcp);
    return 0;
}

/*
 * Criar um objeto
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

/*
 * Eliminar um objeto
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

/*
 * Obter um objeto
 * Procura localmente e, se não encontrar, envia interesse pela rede
 */
int cmd_retrieve(char *name)
{
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

/*
 * Mostrar a topologia da rede
 */
int cmd_show_topology()
{
    printf("Node: %s:%s\n", node.ip, node.port);
    printf("External neighbor: %s:%s\n", node.ext_neighbor_ip, node.ext_neighbor_port);

    if (strlen(node.safe_node_ip) > 0)
    {
        printf("Safety node: %s:%s", node.safe_node_ip, node.safe_node_port);

        /* Show if this node is its own safety node */
        if (strcmp(node.safe_node_ip, node.ip) == 0 &&
            strcmp(node.safe_node_port, node.port) == 0)
        {
            printf(" (self)");
        }
        printf("\n");
    }
    else
    {
        printf("Safety node: Not set\n");
    }

    printf("Internal neighbors:\n");
    Neighbor *curr = node.internal_neighbors;
    if (curr == NULL)
    {
        printf("  None\n");
    }
    else
    {
        while (curr != NULL)
        {
            printf("  %s:%s (interface: %d)\n", curr->ip, curr->port, curr->interface_id);
            curr = curr->next;
        }
    }

    return 0;
}
/*
 * Mostrar nomes de objetos armazenados
 */
int cmd_show_names()
{
    printf("Objects:\n");
    Object *obj = node.objects;
    while (obj != NULL)
    {
        printf("  %s\n", obj->name);
        obj = obj->next;
    }

    printf("Cache:\n");
    obj = node.cache;
    while (obj != NULL)
    {
        printf("  %s\n", obj->name);
        obj = obj->next;
    }

    return 0;
}

/*
 * Mostrar a tabela de interesses
 */
int cmd_show_interest_table()
{
    printf("Interest table:\n");
    InterestEntry *entry = node.interest_table;
    while (entry != NULL)
    {
        printf("  %s:", entry->name);
        for (int i = 0; i < MAX_INTERFACE; i++)
        {
            if (entry->interface_states[i] == RESPONSE)
            {
                printf(" %d:response", i);
            }
            else if (entry->interface_states[i] == WAITING)
            {
                printf(" %d:waiting", i);
            }
            else if (entry->interface_states[i] == CLOSED)
            {
                printf(" %d:closed", i);
            }
        }
        printf("\n");
        entry = entry->next;
    }

    return 0;
}

/*
 * Sair da rede
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
    strcpy(node.ext_neighbor_ip, node.ip);
    strcpy(node.ext_neighbor_port, node.port);
    memset(node.safe_node_ip, 0, INET_ADDRSTRLEN);
    memset(node.safe_node_port, 0, 6);
    node.in_network = 0;

    printf("Left network %03d\n", node.network_id);
    return 0;
}

/*
 * Sair da aplicação
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