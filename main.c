/*
 * Implementação de Rede de Dados Identificados por Nome (NDN)
 * Redes de Computadores e Internet - 2024/2025
 *
 * main.c - Ficheiro principal do programa
 */

#include "ndn.h"
#include "commands.h"
#include "network.h"
#include "objects.h"

/**
 * Variável global que representa o estado do nó
 */
Node node;

/**
 * Função principal.
 * 
 * @param argc Número de argumentos da linha de comandos
 * @param argv Array de strings com os argumentos da linha de comandos
 * @return Código de saída do programa
 */
int main(int argc, char *argv[])
{
    if (argc < 4)
    {
        fprintf(stderr, "Usage: %s cache IP TCP [regIP regUDP]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    /* Extrai os argumentos da linha de comandos */
    int cache_size = atoi(argv[1]);          /* Tamanho da cache */
    char *ip = argv[2];                      /* Endereço IP do nó */
    char *port = argv[3];                    /* Porto TCP do nó */
    char *reg_ip = (argc > 4) ? argv[4] : DEFAULT_REG_IP;  /* IP do servidor de registo (opcional) */
    int reg_udp = (argc > 5) ? atoi(argv[5]) : DEFAULT_REG_UDP;  /* Porto UDP do servidor de registo (opcional) */

    /* Configura o manipulador de sinal para terminação graciosa */
    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_handler = handle_sigint;  /* Define o manipulador para SIGINT */
    if (sigaction(SIGINT, &act, NULL) == -1)
    {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    /* Ignora SIGPIPE para evitar que o programa termine ao escrever em sockets fechados */
    memset(&act, 0, sizeof(act));
    act.sa_handler = SIG_IGN;
    if (sigaction(SIGPIPE, &act, NULL) == -1)
    {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    /* Inicializa o nó */
    initialize_node(cache_size, ip, port, reg_ip, reg_udp);

    /**
     * Ciclo principal.
     * Monitoriza eventos de entrada do utilizador e eventos de rede.
     */
    while (1)
    {
        /* Reinicia o conjunto de descritores de ficheiro */
        FD_ZERO(&node.read_fds);

        /* Adiciona stdin para entrada do utilizador */
        FD_SET(STDIN_FILENO, &node.read_fds);

        /* Adiciona o socket de escuta */
        FD_SET(node.listen_fd, &node.read_fds);

        /* Adiciona o socket UDP para registo */
        FD_SET(node.reg_fd, &node.read_fds);

        /* Adiciona todos os sockets de vizinhos */
        Neighbor *curr = node.neighbors;
        while (curr != NULL)
        {
            FD_SET(curr->fd, &node.read_fds);
            curr = curr->next;
        }

        /* Define o timeout */
        struct timeval timeout;
        timeout.tv_sec = 5;  /* 5 segundos de timeout */
        timeout.tv_usec = 0;

        /* Aguarda por atividade */
        int activity = select(node.max_fd + 1, &node.read_fds, NULL, NULL, &timeout);

        if (activity < 0 && errno != EINTR)
        {
            perror("select");
            break;
        }

        /* Verifica entrada do utilizador - MUITO IMPORTANTE tratar isto primeiro */
        if (FD_ISSET(STDIN_FILENO, &node.read_fds))
        {
            handle_user_input();
        }

        /* Verifica respostas de registo UDP */
        if (FD_ISSET(node.reg_fd, &node.read_fds))
        {
            handle_registration_response();
        }

        /* Trata eventos de rede */
        handle_network_events();

        /* Verifica timeouts de interesses */
        check_interest_timeouts();
    }

    /* Limpa recursos e sai */
    cleanup_and_exit();
    return EXIT_SUCCESS;
}

/**
 * Manipulador de sinal para SIGINT (Ctrl+C).
 * 
 * @param sig Número do sinal recebido
 */
void handle_sigint(int sig)
{
    /* Suprime aviso de parâmetro não utilizado */
    (void)sig;

    printf("\nReceived SIGINT, cleaning up and exiting...\n");
    cleanup_and_exit();
    exit(EXIT_SUCCESS);
}

/**
 * Trata a entrada do utilizador através da linha de comandos.
 */
void handle_user_input()
{
    char cmd_buffer[MAX_CMD_SIZE];

    /* Lê um comando do utilizador */
    if (fgets(cmd_buffer, MAX_CMD_SIZE, stdin) == NULL)
    {
        if (feof(stdin))
        {
            /* Fim de ficheiro, sai graciosamente */
            cleanup_and_exit();
            exit(EXIT_SUCCESS);
        }
        else
        {
            perror("fgets");
            return;
        }
    }

    /* Remove o newline final */
    cmd_buffer[strcspn(cmd_buffer, "\n")] = '\0';

    /* Processa o comando */
    if (process_command(cmd_buffer) < 0)
    {
        /* Mostra erro apenas se o comando não estava vazio */
        if (strlen(cmd_buffer) > 0)
        {
            printf("Error processing command: %s\n", cmd_buffer);
        }
    }
}

/**
 * Inicializa o nó.
 * 
 * @param cache_size Tamanho máximo da cache
 * @param ip Endereço IP do nó
 * @param port Porto TCP do nó
 * @param reg_ip Endereço IP do servidor de registo
 * @param reg_udp Porto UDP do servidor de registo
 */
void initialize_node(int cache_size, char *ip, char *port, char *reg_ip, int reg_udp)
{
    struct addrinfo hints, *res;
    int errcode;

    /* Inicializa a estrutura do nó */
    memset(&node, 0, sizeof(Node));
    node.cache_size = cache_size;
    node.current_cache_size = 0;
    
    /* Armazena informações do nó local */
    strncpy(node.ip, ip, INET_ADDRSTRLEN-1);
    node.ip[INET_ADDRSTRLEN-1] = '\0';
    
    strncpy(node.port, port, 5);
    node.port[5] = '\0';
    
    /* Inicialmente, o vizinho externo é o próprio nó */
    strncpy(node.ext_neighbor_ip, ip, INET_ADDRSTRLEN-1);
    node.ext_neighbor_ip[INET_ADDRSTRLEN-1] = '\0';
    
    strncpy(node.ext_neighbor_port, port, 5);
    node.ext_neighbor_port[5] = '\0';
    
    /* Armazena informações do servidor de registo */
    strncpy(node.reg_server_ip, reg_ip, INET_ADDRSTRLEN-1);
    node.reg_server_ip[INET_ADDRSTRLEN-1] = '\0';
    
    snprintf(node.reg_server_port, 6, "%d", reg_udp);
    
    node.in_network = 0; /* Não está numa rede inicialmente */
    node.max_fd = 0;

    /* Cria o socket TCP de escuta */
    node.listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (node.listen_fd == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    /* Permite reutilização do endereço */
    int reuse = 1;
    if (setsockopt(node.listen_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == -1)
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    /* Liga o socket ao IP e porto especificados */
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if ((errcode = getaddrinfo(NULL, port, &hints, &res)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(errcode));
        exit(EXIT_FAILURE);
    }

    if (bind(node.listen_fd, res->ai_addr, res->ai_addrlen) == -1)
    {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    /* Começa a escuta de ligações */
    if (listen(node.listen_fd, 5) == -1)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    /* Atualiza max_fd */
    node.max_fd = node.listen_fd;

    /* Cria o socket UDP para comunicação com o servidor de registo */
    node.reg_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (node.reg_fd == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    /* Atualiza max_fd se necessário */
    if (node.reg_fd > node.max_fd)
    {
        node.max_fd = node.reg_fd;
    }

    freeaddrinfo(res);
    printf("Node initialized with IP %s and port %s\n", ip, port);
    printf("Registration server set to %s:%d\n", reg_ip, reg_udp);
    printf("Enter 'help' for a list of commands\n");
}

/**
 * Limpa recursos e sai do programa.
 * Fecha todos os sockets e liberta a memória alocada.
 */
void cleanup_and_exit()
{
    /* Se estiver numa rede, sai primeiro */
    if (node.in_network)
    {
        cmd_leave();
    }

    /* Fecha todos os sockets */
    if (node.listen_fd > 0)
    {
        close(node.listen_fd);
    }

    if (node.reg_fd > 0)
    {
        close(node.reg_fd);
    }

    /* Fecha e liberta todas as ligações de vizinhos */
    Neighbor *curr = node.neighbors;
    while (curr != NULL)
    {
        Neighbor *next = curr->next;
        close(curr->fd);
        free(curr);
        curr = next;
    }

    /* Liberta todos os objetos */
    Object *obj = node.objects;
    while (obj != NULL)
    {
        Object *next = obj->next;
        free(obj);
        obj = next;
    }

    /* Liberta todos os objetos em cache */
    obj = node.cache;
    while (obj != NULL)
    {
        Object *next = obj->next;
        free(obj);
        obj = next;
    }

    /* Liberta todas as entradas da tabela de interesses */
    InterestEntry *entry = node.interest_table;
    while (entry != NULL)
    {
        InterestEntry *next = entry->next;
        free(entry);
        entry = next;
    }
}