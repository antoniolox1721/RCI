/**
 * @file commands.h
 * @brief Funções para tratamento de comandos do utilizador na rede NDN
 * @author Bárbara Gonçalves Modesto e António Pedro Lima Loureiro Alves
 * @date Março de 2025
 *
 * Este ficheiro contém as declarações das funções para processar comandos 
 * introduzidos pelo utilizador na interface da aplicação NDN. Os comandos
 * suportados permitem:
 *
 * - Gestão da rede: join, direct_join, leave, exit
 * - Gestão de objetos: create, delete, retrieve
 * - Visualização de informações: show_topology, show_names, show_interest_table
 */

#ifndef COMMANDS_H
#define COMMANDS_H

#include "ndn.h"

/**
 * @brief Processa o comando "join" (j) para aderir a uma rede.
 * 
 * Regista o nó no servidor de registo, obtém a lista de nós existentes,
 * seleciona um nó aleatoriamente e liga-se a ele através de TCP.
 * 
 * @param net ID da rede (três dígitos)
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int cmd_join(char *net);

/**
 * @brief Processa o comando "direct join" (dj) para aderir diretamente a uma rede.
 * 
 * Liga-se diretamente a um nó específico sem utilizar o servidor de registo,
 * ou cria uma nova rede se o endereço for 0.0.0.0.
 * 
 * @param connect_ip Endereço IP do nó a ligar, ou 0.0.0.0 para criar uma nova rede
 * @param connect_port Porto TCP do nó a ligar
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int cmd_direct_join(char *connect_ip, char *connect_port);

/**
 * @brief Processa o comando "create" (c) para criar um objeto.
 * 
 * Adiciona um novo objeto à lista de objetos locais do nó.
 * 
 * @param name Nome do objeto a criar
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int cmd_create(char *name);

/**
 * @brief Processa o comando "delete" (dl) para eliminar um objeto.
 * 
 * Remove um objeto da lista de objetos locais do nó.
 * 
 * @param name Nome do objeto a eliminar
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int cmd_delete(char *name);

/**
 * @brief Processa o comando "retrieve" (r) para obter um objeto.
 * 
 * Procura um objeto localmente e, se não encontrar, envia uma mensagem
 * de interesse na rede para localizar o objeto.
 * 
 * @param name Nome do objeto a obter
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int cmd_retrieve(char *name);

/**
 * @brief Processa o comando "show topology" (st) para mostrar a topologia da rede.
 * 
 * Mostra informações sobre o nó, vizinho externo, nó de salvaguarda e vizinhos internos.
 * 
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int cmd_show_topology();

/**
 * @brief Processa o comando "show names" (sn) para mostrar os objetos armazenados.
 * 
 * Lista todos os objetos locais e em cache armazenados no nó.
 * 
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int cmd_show_names();

/**
 * @brief Processa o comando "show interest table" (si) para mostrar a tabela de interesses.
 * 
 * Mostra a tabela de interesses pendentes, incluindo informações sobre
 * as interfaces e seus estados para cada objeto solicitado.
 * 
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int cmd_show_interest_table();

/**
 * @brief Processa o comando "leave" (l) para sair da rede.
 * 
 * Cancela o registo do nó no servidor de registo e fecha todas as ligações com vizinhos.
 * 
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int cmd_leave();

/**
 * @brief Versão de cmd_leave sem atualização da interface de utilizador.
 * 
 * Funciona como cmd_leave mas não atualiza a interface de utilizador.
 * Utilizado internamente quando o programa está a terminar.
 * 
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int cmd_leave_no_UI();

/**
 * @brief Processa o comando "exit" (x) para sair da aplicação.
 * 
 * Limpa todos os recursos e termina o programa.
 * 
 * @return 0 em caso de sucesso (nunca retorna na realidade, pois termina o programa)
 */
int cmd_exit();

#endif /* COMMANDS_H */