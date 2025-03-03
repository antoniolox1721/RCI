/*
 * Implementação de Rede de Dados Identificados por Nome (NDN)
 * Redes de Computadores e Internet - 2024/2025
 * 
 * network.h - Funções para protocolos de rede
 */

#ifndef NETWORK_H
#define NETWORK_H

#include "ndn.h"

/**
 * Protocolo de registo
 */

/**
 * Envia uma mensagem de registo ao servidor de registo
 * 
 * @param net ID da rede (três dígitos)
 * @param ip Endereço IP do nó
 * @param port Porto TCP do nó
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int send_reg_message(char *net, char *ip, char *port);

/**
 * Envia uma mensagem de remoção de registo ao servidor de registo
 * 
 * @param net ID da rede (três dígitos)
 * @param ip Endereço IP do nó
 * @param port Porto TCP do nó
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int send_unreg_message(char *net, char *ip, char *port);

/**
 * Envia um pedido de lista de nós ao servidor de registo
 * 
 * @param net ID da rede (três dígitos)
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int send_nodes_request(char *net);

/**
 * Processa a resposta NODESLIST do servidor de registo
 * 
 * @param buffer Buffer contendo a resposta NODESLIST
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int process_nodeslist_response(char *buffer);

/**
 * Protocolo de topologia
 */

/**
 * Envia uma mensagem ENTRY para um nó
 * 
 * @param fd Descritor de ficheiro da ligação
 * @param ip Endereço IP do nó emissor
 * @param port Porto TCP do nó emissor
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int send_entry_message(int fd, char *ip, char *port);

/**
 * Envia uma mensagem SAFE para um nó
 * 
 * @param fd Descritor de ficheiro da ligação
 * @param ip Endereço IP do nó de salvaguarda
 * @param port Porto TCP do nó de salvaguarda
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int send_safe_message(int fd, char *ip, char *port);

/**
 * Processa uma mensagem ENTRY recebida
 * 
 * @param fd Descritor de ficheiro da ligação
 * @param ip Endereço IP do nó emissor
 * @param port Porto TCP do nó emissor
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int handle_entry_message(int fd, char *ip, char *port);

/**
 * Processa uma mensagem SAFE recebida
 * 
 * @param fd Descritor de ficheiro da ligação
 * @param ip Endereço IP do nó de salvaguarda
 * @param port Porto TCP do nó de salvaguarda
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int handle_safe_message(int fd, char *ip, char *port);

/**
 * Protocolo NDN
 */

/**
 * Envia uma mensagem de interesse para um objeto
 * 
 * @param name Nome do objeto pretendido
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int send_interest_message(char *name);

/**
 * Envia uma mensagem de objeto como resposta a um interesse
 * 
 * @param fd Descritor de ficheiro da ligação
 * @param name Nome do objeto
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int send_object_message(int fd, char *name);

/**
 * Envia uma mensagem NOOBJECT quando um objeto não é encontrado
 * 
 * @param fd Descritor de ficheiro da ligação
 * @param name Nome do objeto não encontrado
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int send_noobject_message(int fd, char *name);

/**
 * Processa uma mensagem de interesse recebida
 * 
 * @param fd Descritor de ficheiro da ligação
 * @param name Nome do objeto pretendido
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int handle_interest_message(int fd, char *name);

/**
 * Processa uma mensagem de objeto recebida
 * 
 * @param fd Descritor de ficheiro da ligação
 * @param name Nome do objeto recebido
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int handle_object_message(int fd, char *name);

/**
 * Processa uma mensagem NOOBJECT recebida
 * 
 * @param fd Descritor de ficheiro da ligação
 * @param name Nome do objeto não encontrado
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int handle_noobject_message(int fd, char *name);

/**
 * Gestão de timeouts de interesses
 */

/**
 * Verifica e processa interesses que excederam o tempo limite
 */
void check_interest_timeouts();

/**
 * Gestão de respostas do servidor de registo
 */

/**
 * Processa respostas recebidas do servidor de registo
 */
void handle_registration_response();

/**
 * Gestão de eventos de rede
 */

/**
 * Processa eventos de rede (novas ligações, dados recebidos, etc.)
 */
void handle_network_events();

/**
 * Utilidades de rede
 */

/**
 * Estabelece uma ligação TCP com um nó
 * 
 * @param ip Endereço IP do nó destino
 * @param port Porto TCP do nó destino
 * @return Descritor de ficheiro da ligação em caso de sucesso, -1 em caso de erro
 */
int connect_to_node(char *ip, char *port);

/**
 * Adiciona um vizinho à lista de vizinhos
 * 
 * @param ip Endereço IP do vizinho
 * @param port Porto TCP do vizinho
 * @param fd Descritor de ficheiro da ligação
 * @param is_external 1 se for vizinho externo, 0 se for interno
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int add_neighbor(char *ip, char *port, int fd, int is_external);

/**
 * Remove um vizinho da lista de vizinhos
 * 
 * @param fd Descritor de ficheiro da ligação
 * @return 0 em caso de sucesso, -1 se o vizinho não for encontrado
 */
int remove_neighbor(int fd);



#endif /* NETWORK_H */