/**
 * @file debug_utils.h
 * @brief Utilitários para depuração e logging
 * @author Bárbara Gonçalves Modesto e António Pedro Lima Loureiro Alves
 * @date Março de 2025
 *
 * Este ficheiro contém declarações de funções e tipos para facilitar
 * a depuração e o registo de informações durante a execução do programa.
 * 
 * Inclui:
 * - Níveis de registo configuráveis
 * - Funções para mostrar informações sobre o estado do nó
 * - Utilitários para validar estruturas de dados
 * - Funções para conversão de estados em strings
 */

#ifndef DEBUG_UTILS_H
#define DEBUG_UTILS_H

#include "ndn.h"

/**
 * @brief Níveis de registo para depuração
 */
typedef enum {
    LOG_ERROR = 0,   /* Erros críticos */
    LOG_WARN  = 1,   /* Avisos */
    LOG_INFO  = 2,   /* Informação geral */
    LOG_DEBUG = 3,   /* Informação detalhada de depuração */
    LOG_TRACE = 4    /* Informação muito detalhada de rastreio */
} LogLevel;

/**
 * @brief Nível de registo atual - pode ser alterado em tempo de execução
 */
extern LogLevel current_log_level;

/**
 * @brief Imprime informação sobre a tabela de interesses.
 * 
 * Mostra um resumo da tabela de interesses para depuração.
 */
void debug_interest_table(void);

/**
 * @brief Regista uma mensagem com nível de importância.
 * 
 * Gera um registo com timestamp e prefixo de nível apenas se
 * o nível de registo atual for >= ao nível especificado.
 * 
 * @param level Nível de registo da mensagem
 * @param format Formato da mensagem (estilo printf)
 * @param ... Argumentos variáveis para o formato
 */
void log_message(LogLevel level, const char *format, ...);

/**
 * @brief Imprime informação detalhada sobre a tabela de interesses.
 * 
 * Apresenta todas as entradas da tabela de interesses com os seus estados.
 */
void dump_interest_table();

/**
 * @brief Imprime informação detalhada sobre os vizinhos.
 * 
 * Apresenta os vizinhos externos e internos com os seus endereços e interfaces.
 */
void dump_neighbors();

/**
 * @brief Imprime informação detalhada sobre os objetos e cache.
 * 
 * Lista todos os objetos locais e em cache do nó.
 */
void dump_objects();

/**
 * @brief Ativa ou desativa o modo de depuração.
 * 
 * Altera o nível de registo para LOG_DEBUG ou LOG_INFO.
 * 
 * @param enable 1 para ativar, 0 para desativar
 */
void set_debug_mode(int enable);

/**
 * @brief Obtém a representação em string de um valor de estado.
 * 
 * Converte um valor da enumeração interface_state numa string.
 * 
 * @param state Estado a converter para string
 * @return String representativa do estado
 */
const char* state_to_string(enum interface_state state);

/**
 * @brief Imprime informação sobre mudanças de estado para uma interface de interesse.
 * 
 * Mostra as transições de estado de uma interface na tabela de interesses.
 * 
 * @param name Nome do objeto associado ao interesse
 * @param interface_id ID da interface
 * @param old_state Estado antigo da interface
 * @param new_state Novo estado da interface
 */
void print_interest_state(char *name, int interface_id, enum interface_state old_state, enum interface_state new_state);

/**
 * @brief Valida a integridade da tabela de interesses.
 * 
 * Verifica se todos os campos das entradas da tabela de interesses são válidos.
 * 
 * @return 1 se válida, 0 se foram encontrados problemas
 */
int validate_interest_table();

#endif /* DEBUG_UTILS_H */