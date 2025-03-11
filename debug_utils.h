/*
 * Implementação de Rede de Dados Identificados por Nome (NDN)
 * Redes de Computadores e Internet - 2024/2025
 * 
 * debug_utils.h - Utilitários para depuração
 */

#ifndef DEBUG_UTILS_H
#define DEBUG_UTILS_H

#include "ndn.h"

/**
 * Níveis de registo para depuração
 */
typedef enum {
    LOG_ERROR = 0,   /* Erros críticos */
    LOG_WARN  = 1,   /* Avisos */
    LOG_INFO  = 2,   /* Informação geral */
    LOG_DEBUG = 3,   /* Informação detalhada de depuração */
    LOG_TRACE = 4    /* Informação muito detalhada de rastreio */
} LogLevel;

/**
 * Nível de registo atual - pode ser alterado em tempo de execução
 */
extern LogLevel current_log_level;

/**
 * Imprime informação sobre a tabela de interesses - útil para depuração.
 */
void debug_interest_table(void);

/**
 * Regista uma mensagem se o nível de registo atual for >= ao nível especificado.
 * 
 * @param level Nível de registo da mensagem
 * @param format Formato da mensagem (estilo printf)
 * @param ... Argumentos variáveis para o formato
 */
void log_message(LogLevel level, const char *format, ...);

/**
 * Imprime informação detalhada sobre a tabela de interesses - útil para depuração.
 */
void dump_interest_table();

/**
 * Imprime informação detalhada sobre os vizinhos - útil para depuração.
 */
void dump_neighbors();

/**
 * Imprime informação detalhada sobre os objetos e cache - útil para depuração.
 */
void dump_objects();

/**
 * Ativa ou desativa o modo de depuração.
 * 
 * @param enable 1 para ativar, 0 para desativar
 */
void set_debug_mode(int enable);

/**
 * Obtém a representação em string de um valor de estado.
 * 
 * @param state Estado a converter para string
 * @return String representativa do estado
 */
const char* state_to_string(enum interface_state state);

/**
 * Imprime informação sobre mudanças de estado para uma interface de interesse.
 * 
 * @param name Nome do objeto associado ao interesse
 * @param interface_id ID da interface
 * @param old_state Estado antigo da interface
 * @param new_state Novo estado da interface
 */
void print_interest_state(char *name, int interface_id, enum interface_state old_state, enum interface_state new_state);

/**
 * Valida a integridade da tabela de interesses.
 * 
 * @return 1 se válida, 0 se foram encontrados problemas
 */
int validate_interest_table();

#endif /* DEBUG_UTILS_H */