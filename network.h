/**
 * @file network.h
 * @brief Funções para protocolos de rede numa implementação NDN
 * @author Bárbara Gonçalves Modesto e António Pedro Lima Loureiro Alves
 * @date Março de 2025
 *
 * Este ficheiro contém as declarações de funções para implementar os protocolos
 * de comunicação necessários para a rede NDN:
 *
 * 1. Protocolo de Registo (UDP):
 *    - Comunica com o servidor de registo para registar/cancelar registo de nós
 *    - Obtém listas de nós existentes numa rede
 *
 * 2. Protocolo de Topologia (TCP):
 *    - Trata mensagens ENTRY e SAFE para manter a topologia em árvore
 *    - Gere a recuperação de falhas de nós
 *
 * 3. Protocolo NDN (TCP):
 *    - Trata mensagens INTEREST, OBJECT e NOOBJECT
 *    - Gere a tabela de interesses e encaminha mensagens
 */

#ifndef NETWORK_H
#define NETWORK_H

#include "ndn.h"

/**
 * @brief Exibe atualizações da tabela de interesses de forma formatada.
 * 
 * Função de utilidade para mostrar atualizações na tabela de interesses
 * durante as operações de protocolo NDN.
 * 
 * @param action Descrição da ação realizada
 * @param name Nome do objeto relacionado à ação
 */
void display_interest_table_update(const char* action, const char* name);

/**
 * @brief Atualiza as informações de um vizinho com o porto de escuta correto.
 * 
 * Quando recebemos uma mensagem ENTRY, atualizamos o porto do vizinho para
 * o porto de escuta real, não o porto efémero da ligação.
 * 
 * @param fd Descritor de ficheiro da ligação
 * @param ip Endereço IP do vizinho
 * @param port Porto de escuta do vizinho
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int update_neighbor_info(int fd, char *ip, char *port);

/**
 * @brief Remove um vizinho da lista de vizinhos.
 * 
 * Remove um vizinho tanto da lista de vizinhos geral como, se aplicável,
 * da lista de vizinhos internos.
 * 
 * @param fd Descritor de ficheiro da ligação
 * @return 0 em caso de sucesso, -1 se o vizinho não for encontrado
 */
int remove_neighbor(int fd);

/**
 * @brief Trata a desconexão de um vizinho externo.
 * 
 * Implementa o procedimento de recuperação quando um vizinho externo
 * se desconecta da rede.
 * 
 * @param removed_ip Endereço IP do vizinho removido
 * @param removed_port Porto TCP do vizinho removido
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int handle_external_neighbor_disconnect(char *removed_ip, char *removed_port);

/**
 * @brief Envia uma mensagem de registo ao servidor de registo.
 * 
 * Envia uma mensagem REG ao servidor de registo para registar
 * o nó numa rede específica.
 * 
 * @param net ID da rede (três dígitos)
 * @param ip Endereço IP do nó
 * @param port Porto TCP do nó
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int send_reg_message(char *net, char *ip, char *port);

/**
 * @brief Envia uma mensagem de remoção de registo ao servidor de registo.
 * 
 * Envia uma mensagem UNREG ao servidor de registo para remover
 * o registo do nó de uma rede.
 * 
 * @param net ID da rede (três dígitos)
 * @param ip Endereço IP do nó
 * @param port Porto TCP do nó
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int send_unreg_message(char *net, char *ip, char *port);

/**
 * @brief Envia um pedido de lista de nós ao servidor de registo.
 * 
 * Envia uma mensagem NODES ao servidor de registo para obter
 * a lista de nós numa rede específica.
 * 
 * @param net ID da rede (três dígitos)
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int send_nodes_request(char *net);

/**
 * @brief Processa a resposta NODESLIST do servidor de registo.
 * 
 * Analisa a resposta NODESLIST do servidor de registo, escolhe
 * um nó aleatoriamente e tenta conectar-se a ele.
 * 
 * @param buffer Buffer contendo a resposta NODESLIST
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int process_nodeslist_response(char *buffer);

/**
 * @brief Envia uma mensagem ENTRY para um nó.
 * 
 * Envia uma mensagem ENTRY com o endereço IP e porto TCP
 * do nó emissor.
 * 
 * @param fd Descritor de ficheiro da ligação
 * @param ip Endereço IP do nó emissor
 * @param port Porto TCP do nó emissor
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int send_entry_message(int fd, char *ip, char *port);

/**
 * @brief Envia uma mensagem SAFE para um nó.
 * 
 * Envia uma mensagem SAFE com o endereço IP e porto TCP
 * do nó de salvaguarda.
 * 
 * @param fd Descritor de ficheiro da ligação
 * @param ip Endereço IP do nó de salvaguarda
 * @param port Porto TCP do nó de salvaguarda
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int send_safe_message(int fd, char *ip, char *port);

/**
 * @brief Processa uma mensagem ENTRY recebida.
 * 
 * Implementa o procedimento para tratar uma mensagem ENTRY
 * recebida de um nó.
 * 
 * @param fd Descritor de ficheiro da ligação
 * @param ip Endereço IP do nó emissor
 * @param port Porto TCP do nó emissor
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int handle_entry_message(int fd, char *ip, char *port);

/**
 * @brief Processa uma mensagem SAFE recebida.
 * 
 * Implementa o procedimento para tratar uma mensagem SAFE
 * recebida de um nó.
 * 
 * @param fd Descritor de ficheiro da ligação
 * @param ip Endereço IP do nó de salvaguarda
 * @param port Porto TCP do nó de salvaguarda
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int handle_safe_message(int fd, char *ip, char *port);

/**
 * @brief Envia uma mensagem de interesse para um objeto.
 * 
 * Envia uma mensagem INTEREST para solicitar um objeto pelo nome.
 * 
 * @param name Nome do objeto pretendido
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int send_interest_message(char *name);

/**
 * @brief Envia uma mensagem de objeto como resposta a um interesse.
 * 
 * Envia uma mensagem OBJECT contendo o nome do objeto encontrado.
 * 
 * @param fd Descritor de ficheiro da ligação
 * @param name Nome do objeto
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int send_object_message(int fd, char *name);

/**
 * @brief Envia uma mensagem NOOBJECT quando um objeto não é encontrado.
 * 
 * Envia uma mensagem NOOBJECT indicando que o objeto solicitado
 * não foi encontrado.
 * 
 * @param fd Descritor de ficheiro da ligação
 * @param name Nome do objeto não encontrado
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int send_noobject_message(int fd, char *name);

/**
 * @brief Processa uma mensagem de interesse recebida.
 * 
 * Implementa o procedimento para tratar uma mensagem INTEREST recebida:
 * - Verifica se o objeto existe localmente
 * - Atualiza a tabela de interesses
 * - Encaminha o interesse se necessário
 * 
 * @param fd Descritor de ficheiro da ligação
 * @param name Nome do objeto pretendido
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int handle_interest_message(int fd, char *name);

/**
 * @brief Processa uma mensagem de objeto recebida.
 * 
 * Implementa o procedimento para tratar uma mensagem OBJECT recebida:
 * - Adiciona o objeto à cache
 * - Encaminha o objeto para interfaces em estado RESPONSE
 * - Remove a entrada de interesse correspondente
 * 
 * @param fd Descritor de ficheiro da ligação
 * @param name Nome do objeto recebido
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int handle_object_message(int fd, char *name);

/**
 * @brief Processa uma mensagem NOOBJECT recebida.
 * 
 * Implementa o procedimento para tratar uma mensagem NOOBJECT recebida:
 * - Marca a interface como CLOSED
 * - Verifica se ainda há interfaces em estado WAITING
 * - Encaminha NOOBJECT para interfaces em estado RESPONSE se necessário
 * 
 * @param fd Descritor de ficheiro da ligação
 * @param name Nome do objeto não encontrado
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int handle_noobject_message(int fd, char *name);

/**
 * @brief Verifica e processa interesses que excederam o tempo limite.
 * 
 * Verifica se há interesses com tempo limite excedido e envia
 * mensagens NOOBJECT para interfaces em estado RESPONSE.
 */
void check_interest_timeouts();

/**
 * @brief Processa respostas recebidas do servidor de registo.
 * 
 * Trata as respostas recebidas do servidor de registo:
 * - NODESLIST: Lista de nós numa rede
 * - OKREG: Confirmação de registo
 * - OKUNREG: Confirmação de cancelamento de registo
 */
void handle_registration_response();

/**
 * @brief Processa eventos de rede (novas ligações, dados recebidos, etc.).
 * 
 * Trata os eventos de rede detetados pelo select():
 * - Aceita novas ligações
 * - Processa dados recebidos de ligações existentes
 * - Trata desconexões
 */
void handle_network_events();

/**
 * @brief Atualiza e propaga informações do nó de salvaguarda para todos os vizinhos internos.
 * 
 * Envia mensagens SAFE para todos os vizinhos internos com a informação
 * do nó de salvaguarda atualizada.
 */
void update_and_propagate_safety_node();

/**
 * @brief Estabelece uma ligação TCP com um nó.
 * 
 * Cria um socket TCP, configura timeout e estabelece ligação com o nó destino.
 * 
 * @param ip Endereço IP do nó destino
 * @param port Porto TCP do nó destino
 * @return Descritor de ficheiro da ligação em caso de sucesso, -1 em caso de erro
 */
int connect_to_node(char *ip, char *port);

/**
 * @brief Adiciona um vizinho à lista de vizinhos.
 * 
 * Adiciona um novo vizinho à lista de vizinhos e, se for um vizinho interno,
 * também à lista de vizinhos internos.
 * 
 * @param ip Endereço IP do vizinho
 * @param port Porto TCP do vizinho
 * @param fd Descritor de ficheiro da ligação
 * @param is_external 1 se for vizinho externo, 0 se for interno
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int add_neighbor(char *ip, char *port, int fd, int is_external);

/**
 * @brief Reinicia o interesse para um objeto, removendo a sua entrada.
 * 
 * Remove uma entrada da tabela de interesses, marcando-a primeiro
 * como a ser removida para evitar problemas de concorrência.
 * 
 * @param name Nome do objeto associado ao interesse a reiniciar
 */
void reset_interest_for_object(char *name);

/**
 * @brief Inicializa uma entrada de interesse.
 *
 * Configura os valores iniciais de uma nova entrada de interesse.
 *
 * @param entry Apontador para a entrada de interesse a inicializar
 * @param name Nome do objeto associado à entrada
 */
void initialize_interest_entry(InterestEntry *entry, char *name);

#endif /* NETWORK_H */