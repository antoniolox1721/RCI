/*
 * Implementação de Rede de Dados Identificados por Nome (NDN)
 * Redes de Computadores e Internet - 2024/2025
 * 
 * commands.h - Funções para tratamento de comandos
 */

#ifndef COMMANDS_H
#define COMMANDS_H

#include "ndn.h"

/**
 * Manipuladores de comandos
 */

/**
 * Processa o comando "join" (j) para aderir a uma rede.
 * Regista o nó no servidor de registo e liga-se a um dos nós existentes.
 * 
 * @param net ID da rede (três dígitos)
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int cmd_join(char *net);

/**
 * Processa o comando "direct join" (dj) para aderir diretamente a uma rede.
 * Liga-se diretamente ao nó especificado sem consultar o servidor de registo.
 * 
 * @param connect_ip Endereço IP do nó a ligar
 * @param connect_tcp Porto TCP do nó a ligar
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int cmd_direct_join(char *connect_ip, char *connect_tcp);

/**
 * Processa o comando "create" (c) para criar um objeto.
 * 
 * @param name Nome do objeto a criar
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int cmd_create(char *name);

/**
 * Processa o comando "delete" (dl) para eliminar um objeto.
 * 
 * @param name Nome do objeto a eliminar
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int cmd_delete(char *name);

/**
 * Processa o comando "retrieve" (r) para obter um objeto.
 * 
 * @param name Nome do objeto a obter
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int cmd_retrieve(char *name);

/**
 * Processa o comando "show topology" (st) para mostrar a topologia da rede.
 * 
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int cmd_show_topology();

/**
 * Processa o comando "show names" (sn) para mostrar os objetos armazenados.
 * 
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int cmd_show_names();

/**
 * Processa o comando "show interest table" (si) para mostrar a tabela de interesses.
 * 
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int cmd_show_interest_table();

/**
 * Processa o comando "leave" (l) para sair da rede.
 * 
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int cmd_leave();

/**
 * Processa o comando "exit" (x) para sair da aplicação.
 * 
 * @return 0 em caso de sucesso (nunca retorna na realidade, pois termina o programa)
 */
int cmd_exit();

#endif /* COMMANDS_H */