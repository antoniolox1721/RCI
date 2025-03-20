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
#include <time.h>
#include <fcntl.h>  /* Adicionado para suporte a sockets não bloqueantes */
#define COLOR_RESET   "\x1B[0m"
#define COLOR_RED     "\x1B[31m"
#define COLOR_GREEN   "\x1B[32m"
#define COLOR_YELLOW  "\x1B[33m"
#define COLOR_BLUE    "\x1B[34m"
#define COLOR_MAGENTA "\x1B[35m"
#define COLOR_CYAN    "\x1B[36m"
#define COLOR_WHITE   "\x1B[37m"
#define COLOR_BOLD    "\x1B[1m"

/**
 * Definição de constantes globais
 */
#define MAX_INTERFACE 10       /* Número máximo de interfaces que um nó pode ter */
#define MAX_OBJECT_NAME 100    /* Comprimento máximo de nomes de objetos */
#define MAX_CACHE_SIZE 100     /* Tamanho máximo da cache por defeito */
#define MAX_BUFFER 1024        /* Tamanho máximo do buffer para mensagens */
#define MAX_CMD_SIZE 128       /* Tamanho máximo de comandos */
#define DEFAULT_REG_IP "193.136.138.142"  /* IP padrão do servidor de registo */
#define DEFAULT_REG_UDP 59000  /* Porto UDP padrão do servidor de registo */
#define INTEREST_TIMEOUT 10    /* Tempo limite para mensagens de interesse (em segundos) */

/**
 * Enumeração de estados possíveis para cada interface na tabela de interesses.
 * Cada interface em relação a um pedido pode estar num dos seguintes estados:
 */
enum interface_state {
    RESPONSE = 0,   /* Interface onde uma resposta deve ser enviada */
    WAITING = 1,    /* Interface onde um interesse foi enviado, aguardando resposta */
    CLOSED = 2      /* Interface onde uma mensagem NOOBJECT foi recebida */
};

/**
 * Estrutura que representa um objeto na rede NDN.
 * Cada objeto tem um nome único e está ligado numa lista ligada.
 */
typedef struct object {
    char name[MAX_OBJECT_NAME + 1];  /* Nome do objeto (com espaço para o terminador nulo) */
    struct object *next;             /* Apontador para o próximo objeto na lista */
} Object;

/**
 * Estrutura que representa uma entrada na tabela de interesses.
 * Cada entrada contém informação sobre um interesse em curso para um objeto.
 */
typedef struct interest_entry {
    char name[MAX_OBJECT_NAME + 1];  /* Nome do objeto pretendido */
    int interface_states[MAX_INTERFACE];  /* Estado de cada interface para este interesse */
    time_t timestamp;                /* Momento em que o interesse foi criado */
    int marked_for_removal;          /* Sinalizador que indica se a entrada está a ser removida */
    struct interest_entry *next;     /* Apontador para a próxima entrada na tabela */
} InterestEntry;


/**
 * Estrutura que representa um vizinho na rede NDN.
 * Cada vizinho está ligado através de uma sessão TCP.
 */
typedef struct neighbor {
    char ip[INET_ADDRSTRLEN];  /* Endereço IP do vizinho */
    char port[6];              /* Porto TCP do vizinho */
    int fd;                    /* Descritor de ficheiro para a ligação */
    int interface_id;          /* ID da interface */
    struct neighbor *next;     /* Apontador para o próximo vizinho na lista */
} Neighbor;

/**
 * Estrutura principal que representa o estado do nó.
 * Contém toda a informação necessária para o funcionamento do nó na rede NDN.
 */
typedef struct node {
    char ip[INET_ADDRSTRLEN];        /* Endereço IP do nó */
    char port[6];                    /* Porto TCP do nó */
    char ext_neighbor_ip[INET_ADDRSTRLEN];  /* IP do vizinho externo */
    char ext_neighbor_port[6];       /* Porto do vizinho externo */
    char safe_node_ip[INET_ADDRSTRLEN];  /* IP do nó de salvaguarda */
    char safe_node_port[6];          /* Porto do nó de salvaguarda */
    char reg_server_ip[INET_ADDRSTRLEN];  /* IP do servidor de registo */
    char reg_server_port[6];         /* Porto UDP do servidor de registo */
    int listen_fd;                   /* Descritor de ficheiro para o socket de escuta */
    int reg_fd;                      /* Descritor de ficheiro para o socket do servidor de registo */
    int max_fd;                      /* Descritor de ficheiro máximo para select() */
    int cache_size;                  /* Tamanho máximo da cache */
    int current_cache_size;          /* Tamanho atual da cache */
    int in_network;                  /* 1 se estiver numa rede, 0 caso contrário */
    int network_id;                  /* ID da rede */
    fd_set read_fds;                 /* Conjunto de descritores de ficheiro para select() */
    Neighbor *neighbors;             /* Lista de todos os vizinhos */
    Neighbor *internal_neighbors;    /* Lista de vizinhos internos */
    Object *objects;                 /* Lista de objetos locais */
    Object *cache;                   /* Lista de objetos em cache */
    InterestEntry *interest_table;   /* Tabela de interesses */
} Node;

/* Variável global que representa o estado do nó atual */
extern Node node;

/**
 * Funções de inicialização e limpeza
 */

/**
 * Inicializa o nó com as configurações especificadas.
 * 
 * @param cache_size Tamanho máximo da cache
 * @param ip Endereço IP do nó
 * @param port Porto TCP do nó
 * @param reg_ip Endereço IP do servidor de registo
 * @param reg_udp Porto UDP do servidor de registo
 */
void initialize_node(int cache_size, char *ip, char *port, char *reg_ip, int reg_udp);

/**
 * Limpa todos os recursos alocados e termina o programa.
 */
void cleanup_and_exit();

/**
 * Manipulador para o sinal SIGINT (Ctrl+C).
 * 
 * @param sig Número do sinal recebido
 */
void handle_sigint(int sig);

/**
 * Manipuladores de eventos
 */

/**
 * Trata a entrada do utilizador através da linha de comandos.
 */
void handle_user_input();

/**
 * Trata eventos de rede (novas ligações, dados recebidos, etc.).
 */
void handle_network_events();

/**
 * Processa respostas recebidas do servidor de registo.
 */
void handle_registration_response();

/**
 * Verifica e processa interesses que excederam o tempo limite.
 */
void check_interest_timeouts();

/**
 * Verifica a validade do nó de salvaguarda.
 */
void check_safety_node_validity();

/**
 * Processamento de comandos
 */

/**
 * Processa um comando introduzido pelo utilizador.
 * 
 * @param cmd String contendo o comando a processar
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int process_command(char *cmd);

/**
 * Mostra informações de ajuda sobre os comandos disponíveis.
 */
void print_help();

#endif /* NDN_H */