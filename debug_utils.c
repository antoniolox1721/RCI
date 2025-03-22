/**
 * @file debug_utils.c
 * @brief Implementação de utilitários de depuração
 * @author Bárbara Gonçalves Modesto e António Pedro Lima Loureiro Alves
 * @date Março de 2025
 *
 * Este ficheiro contém a implementação de funções de utilidade para depuração
 * e logging durante a execução do programa.
 * 
 * Inclui:
 * - Funções para mostrar informações detalhadas sobre estado do nó
 * - Funções para registar mensagens com níveis de importância
 * - Utilitários para validação de estruturas de dados
 * - Conversores entre estados e suas representações em string
 */

#include "debug_utils.h"
#include <stdarg.h>

/* Nível de registo predefinido */
LogLevel current_log_level = LOG_INFO;

/* Sinalizador do modo de depuração */
int debug_mode = 0;

/**
 * @brief Regista uma mensagem se o nível de registo atual for >= ao nível especificado.
 * 
 * Gera um registo formatado com data/hora e prefixo indicando o nível de importância.
 * 
 * @param level Nível de registo da mensagem
 * @param format Formato da mensagem (estilo printf)
 * @param ... Argumentos variáveis para o formato
 */
void log_message(LogLevel level, const char *format, ...) {
    if (level > current_log_level) {
        return;
    }
    
    /* Obtém a hora atual para o timestamp */
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char timestamp[20];
    strftime(timestamp, 20, "%Y-%m-%d %H:%M:%S", tm_info);
    
    /* Prefixo baseado no nível de registo */
    const char *prefix;
    switch (level) {
        case LOG_ERROR: prefix = "ERROR"; break;
        case LOG_WARN:  prefix = "WARN "; break;
        case LOG_INFO:  prefix = "INFO "; break;
        case LOG_DEBUG: prefix = "DEBUG"; break;
        case LOG_TRACE: prefix = "TRACE"; break;
        default:        prefix = "?????"; break;
    }
    
    fprintf(stderr, "[%s] [%s] ", timestamp, prefix);
    
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    
    fprintf(stderr, "\n");
}

/**
 * @brief Imprime informação detalhada sobre a tabela de interesses.
 * 
 * Lista todas as entradas da tabela, seus estados por interface e idades.
 */
void dump_interest_table() {
    log_message(LOG_DEBUG, "Interest table dump:");
    
    int count = 0;
    InterestEntry *entry = node.interest_table;
    
    while (entry != NULL) {
        log_message(LOG_DEBUG, "Entry %d: %s", count, entry->name);
        log_message(LOG_DEBUG, "  Timestamp: %ld (now: %ld, age: %ld secs)", 
                   entry->timestamp, time(NULL), time(NULL) - entry->timestamp);
        
        for (int i = 0; i < MAX_INTERFACE; i++) {
            if (entry->interface_states[i] != 0) {
                log_message(LOG_DEBUG, "  Interface %d: %s", i, state_to_string(entry->interface_states[i]));
            }
        }
        
        entry = entry->next;
        count++;
    }
    
    if (count == 0) {
        log_message(LOG_DEBUG, "Interest table is empty");
    }
}

/**
 * @brief Imprime informação detalhada sobre os vizinhos.
 * 
 * Lista os vizinhos externos, o nó de salvaguarda e os vizinhos internos.
 */
void dump_neighbors() {
    log_message(LOG_DEBUG, "Neighbors dump:");
    
    log_message(LOG_DEBUG, "External neighbor: %s:%s", node.ext_neighbor_ip, node.ext_neighbor_port);
    log_message(LOG_DEBUG, "Safety node: %s:%s", node.safe_node_ip, node.safe_node_port);
    
    int count = 0;
    Neighbor *curr = node.neighbors;
    
    log_message(LOG_DEBUG, "All neighbors:");
    while (curr != NULL) {
        log_message(LOG_DEBUG, "  Neighbor %d: %s:%s (fd: %d, interface: %d)", 
                   count, curr->ip, curr->port, curr->fd, curr->interface_id);
        curr = curr->next;
        count++;
    }
    
    if (count == 0) {
        log_message(LOG_DEBUG, "No neighbors");
    }
    
    count = 0;
    curr = node.internal_neighbors;
    
    log_message(LOG_DEBUG, "Internal neighbors:");
    while (curr != NULL) {
        log_message(LOG_DEBUG, "  Internal neighbor %d: %s:%s (fd: %d, interface: %d)", 
                   count, curr->ip, curr->port, curr->fd, curr->interface_id);
        curr = curr->next;
        count++;
    }
    
    if (count == 0) {
        log_message(LOG_DEBUG, "No internal neighbors");
    }
}

/**
 * @brief Imprime informação detalhada sobre os objetos e cache.
 * 
 * Lista todos os objetos locais e em cache do nó.
 */
void dump_objects() {
    log_message(LOG_DEBUG, "Objects dump:");
    
    int count = 0;
    Object *obj = node.objects;
    
    log_message(LOG_DEBUG, "Local objects:");
    while (obj != NULL) {
        log_message(LOG_DEBUG, "  Object %d: %s", count, obj->name);
        obj = obj->next;
        count++;
    }
    
    if (count == 0) {
        log_message(LOG_DEBUG, "No local objects");
    }
    
    count = 0;
    obj = node.cache;
    
    log_message(LOG_DEBUG, "Cached objects (%d/%d):", node.current_cache_size, node.cache_size);
    while (obj != NULL) {
        log_message(LOG_DEBUG, "  Cached object %d: %s", count, obj->name);
        obj = obj->next;
        count++;
    }
    
    if (count == 0) {
        log_message(LOG_DEBUG, "No cached objects");
    }
}

/**
 * @brief Ativa ou desativa o modo de depuração.
 * 
 * Altera o nível de registo para controlar a quantidade de informação mostrada.
 * 
 * @param enable 1 para ativar, 0 para desativar
 */
void set_debug_mode(int enable) {
    debug_mode = enable;
    
    if (enable) {
        current_log_level = LOG_DEBUG;
        log_message(LOG_INFO, "Debug mode enabled");
    } else {
        current_log_level = LOG_INFO;
        log_message(LOG_INFO, "Debug mode disabled");
    }
}

/**
 * @brief Obtém a representação em string de um valor de estado.
 * 
 * Converte um valor da enumeração interface_state numa string.
 * 
 * @param state Estado a converter para string
 * @return String representativa do estado
 */
const char* state_to_string(enum interface_state state) {
    switch (state) {
        case RESPONSE: return "RESPONSE";
        case WAITING:  return "WAITING";
        case CLOSED:   return "CLOSED";
        default:       return "UNKNOWN";
    }
}

/**
 * @brief Valida a integridade da tabela de interesses.
 * 
 * Verifica se todos os campos das entradas da tabela de interesses são válidos.
 * 
 * @return 1 se válida, 0 se foram encontrados problemas
 */
int validate_interest_table() {
    int valid = 1;
    InterestEntry *entry = node.interest_table;
    
    while (entry != NULL) {
        /* Verifica se o nome é válido */
        if (strlen(entry->name) == 0 || strlen(entry->name) > MAX_OBJECT_NAME) {
            log_message(LOG_ERROR, "Invalid name length for entry: %s", entry->name);
            valid = 0;
        }
        
        /* Verifica estados de interface */
        int has_waiting = 0;
        int has_response = 0;
        
        for (int i = 0; i < MAX_INTERFACE; i++) {
            if (entry->interface_states[i] != 0 && 
                entry->interface_states[i] != RESPONSE && 
                entry->interface_states[i] != WAITING && 
                entry->interface_states[i] != CLOSED) {
                log_message(LOG_ERROR, "Invalid state %d for interface %d in entry: %s", 
                           entry->interface_states[i], i, entry->name);
                valid = 0;
            }
            
            if (entry->interface_states[i] == WAITING) {
                has_waiting = 1;
            }
            
            if (entry->interface_states[i] == RESPONSE) {
                has_response = 1;
            }
        }
        
        /* Verifica que as entradas têm pelo menos uma interface num estado não-zero */
        if (!has_waiting && !has_response) {
            log_message(LOG_ERROR, "Entry has no interfaces in WAITING or RESPONSE state: %s", 
                       entry->name);
            valid = 0;
        }
        
        entry = entry->next;
    }
    
    return valid;
}

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
void print_interest_state(char *name, int interface_id, enum interface_state old_state, enum interface_state new_state) {
    const char *old_str = "UNKNOWN";
    const char *new_str = "UNKNOWN";
    
    switch (old_state) {
        case RESPONSE: old_str = "RESPONSE"; break;
        case WAITING: old_str = "WAITING"; break;
        case CLOSED: old_str = "CLOSED"; break;
        default: old_str = "UNKNOWN"; break;
    }
    
    switch (new_state) {
        case RESPONSE: new_str = "RESPONSE"; break;
        case WAITING: new_str = "WAITING"; break;
        case CLOSED: new_str = "CLOSED"; break;
        default: new_str = "UNKNOWN"; break;
    }
    
    printf("Interface %d state for %s: %s -> %s\n", 
           interface_id, name, old_str, new_str);
}

/**
 * @brief Imprime informação sobre a tabela de interesses.
 * 
 * Mostra um resumo da tabela de interesses para depuração.
 */
void debug_interest_table(void) {
    printf("==== INTEREST TABLE DUMP ====\n");
    InterestEntry *entry = node.interest_table;
    int count = 0;
    
    while (entry != NULL) {
        printf("Entry %d: %s\n", count++, entry->name);
        
        int waiting = 0, response = 0, closed = 0, unknown = 0;
        for (int i = 0; i < MAX_INTERFACE; i++) {
            if (entry->interface_states[i] == WAITING) waiting++;
            else if (entry->interface_states[i] == RESPONSE) response++;
            else if (entry->interface_states[i] == CLOSED) closed++;
            else if (entry->interface_states[i] != 0) unknown++;
        }
        
        printf("  States: WAITING=%d, RESPONSE=%d, CLOSED=%d, UNKNOWN=%d\n", 
               waiting, response, closed, unknown);
        
        printf("  Interfaces: ");
        for (int i = 0; i < MAX_INTERFACE; i++) {
            if (entry->interface_states[i] != 0) {
                const char *state = "?";
                if (entry->interface_states[i] == WAITING) state = "WAITING";
                else if (entry->interface_states[i] == RESPONSE) state = "RESPONSE";
                else if (entry->interface_states[i] == CLOSED) state = "CLOSED";
                
                printf("%d:%s ", i, state);
            }
        }
        printf("\n");
        
        entry = entry->next;
    }
    printf("============================\n");
}