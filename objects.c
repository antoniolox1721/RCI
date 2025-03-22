/*
 * Implementação de Rede de Dados Identificados por Nome (NDN)
 * Redes de Computadores e Internet - 2024/2025
 * 
 * objects.c - Implementação das funções de manipulação de objetos
 */

#include "objects.h"
#include "debug_utils.h"
#include "network.h"


/**
 * Adiciona um objeto.
 * 
 * @param name Nome do objeto a adicionar
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int add_object(char *name) {
    /* Verifica se o objeto já existe */
    Object *curr = node.objects;
    while (curr != NULL) {
        if (strcmp(curr->name, name) == 0) {
            return 0;  /* Objeto já existe */
        }
        curr = curr->next;
    }
    
    /* Cria um novo objeto */
    Object *new_object = malloc(sizeof(Object));
    if (new_object == NULL) {
        perror("malloc");
        return -1;
    }
    
    strcpy(new_object->name, name);
    
    /* Adiciona à lista de objetos */
    new_object->next = node.objects;
    node.objects = new_object;
    
    return 0;
}

/**
 * Remove um objeto.
 * 
 * @param name Nome do objeto a remover
 * @return 0 em caso de sucesso, -1 se o objeto não for encontrado
 */
int remove_object(char *name) {
    /* Procura o objeto */
    Object *prev = NULL;
    Object *curr = node.objects;
    
    while (curr != NULL) {
        if (strcmp(curr->name, name) == 0) {
            /* Remove da lista */
            if (prev == NULL) {
                node.objects = curr->next;
            } else {
                prev->next = curr->next;
            }
            
            free(curr);
            return 0;
        }
        
        prev = curr;
        curr = curr->next;
    }
    
    return -1;  /* Objeto não encontrado */
}

/**
 * Adiciona um objeto à cache com limite de tamanho rigoroso.
 * 
 * @param name Nome do objeto a adicionar à cache
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int add_to_cache(char *name) {
    /* Verifica validade da entrada */
    if (name == NULL || strlen(name) == 0) {
        fprintf(stderr, "Error: Attempting to cache invalid object name\n");
        return -1;
    }

    /* Verifica se o objeto já existe na cache */
    Object *curr = node.cache;
    Object *prev = NULL;
    while (curr != NULL) {
        if (strcmp(curr->name, name) == 0) {
            /* Objeto já na cache, não é necessário adicionar novamente */
            return 0;
        }
        prev = curr;
        curr = curr->next;
    }
    
    /* Garante que a cache não excede o tamanho máximo */
    while (node.current_cache_size >= node.cache_size) {
        if (node.cache == NULL) {
            /* Estado inesperado - cache está marcada como cheia mas vazia */
            fprintf(stderr, "Warning: Cache size inconsistency detected\n");
            node.current_cache_size = 0;
            break;
        }
        
        /* Remove o primeiro (mais antigo) objeto */
        Object *oldest = node.cache;
        node.cache = oldest->next;
        
        printf("Cache full. Removing oldest object: %s to make room for %s\n", 
               oldest->name, name);
        
        free(oldest);
        node.current_cache_size--;
    }
    
    /* Cria uma nova entrada na cache */
    Object *new_object = malloc(sizeof(Object));
    if (new_object == NULL) {
        perror("malloc");
        return -1;
    }
    
    /* Copia o nome, garantindo que não há overflow no buffer */
    strncpy(new_object->name, name, MAX_OBJECT_NAME);
    new_object->name[MAX_OBJECT_NAME] = '\0';  /* Garante terminação com null */
    new_object->next = NULL;
    
    /* Adiciona ao fim da lista de objetos em cache */
    if (node.cache == NULL) {
        /* Primeiro objeto na cache */
        node.cache = new_object;
    } else {
        /* Encontra o último objeto e adiciona */
        curr = node.cache;
        while (curr->next != NULL) {
            curr = curr->next;
        }
        curr->next = new_object;
    }
    
    /* Incrementa o tamanho da cache */
    node.current_cache_size++;
    
    printf("Added object %s to cache (size: %d/%d)\n", 
           name, node.current_cache_size, node.cache_size);
    
    /* Verificação final para prevenir overflow do tamanho da cache */
    if (node.current_cache_size > node.cache_size) {
        fprintf(stderr, "CRITICAL ERROR: Cache size exceeded maximum limit!\n");
        /* Pode querer tratar isto de forma mais elegante dependendo da estratégia de tratamento de erros */
        exit(EXIT_FAILURE);
    }
    if (prev != NULL) { /* Usado para travessia, silencia aviso do compilador */ }
    return 0;
}

/**
 * Procura uma entrada na tabela de interesses.
 * 
 * @param name Nome do objeto associado à entrada de interesse
 * @return Apontador para a entrada se encontrada, NULL caso contrário
 */
InterestEntry* find_interest_entry(char *name) {
    InterestEntry *entry = node.interest_table;
    
    while (entry != NULL) {
        if (strcmp(entry->name, name) == 0) {
            return entry;
        }
        entry = entry->next;
    }
    
    return NULL;
}

/**
 * Procura um objeto pelo nome.
 * 
 * @param name Nome do objeto a procurar
 * @return 0 se o objeto for encontrado, -1 caso contrário
 */
int find_object(char *name) {
    /* Procura na lista de objetos */
    Object *curr = node.objects;
    while (curr != NULL) {
        if (strcmp(curr->name, name) == 0) {
            return 0;  /* Objeto encontrado */
        }
        curr = curr->next;
    }
    
    return -1;  /* Objeto não encontrado */
}

/**
 * Procura um objeto na cache.
 * 
 * @param name Nome do objeto a procurar na cache
 * @return 0 se o objeto for encontrado, -1 caso contrário
 */
int find_in_cache(char *name) {
    /* Procura na cache */
    Object *curr = node.cache;
    while (curr != NULL) {
        if (strcmp(curr->name, name) == 0) {
            return 0;  /* Objeto encontrado na cache */
        }
        curr = curr->next;
    }
    
    return -1;  /* Objeto não encontrado na cache */
}

/**
 * Adiciona uma entrada de interesse.
 * 
 * @param name Nome do objeto associado ao interesse
 * @param interface_id ID da interface a atualizar
 * @param state Estado a definir para a interface (RESPONSE, WAITING ou CLOSED)
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int add_interest_entry(char *name, int interface_id, enum interface_state state) {
    /* Verifica se a entrada já existe */
    InterestEntry *curr = node.interest_table;
    while (curr != NULL) {
        if (strcmp(curr->name, name) == 0) {
            /* Entrada existe, atualiza-a */
            curr->interface_states[interface_id] = state;
            return 0;
        }
        curr = curr->next;
    }
    
    /* Cria uma nova entrada */
    InterestEntry *new_entry = malloc(sizeof(InterestEntry));
    if (new_entry == NULL) {
        perror("malloc");
        return -1;
    }
    
    strcpy(new_entry->name, name);
    
    /* Inicializa todas as interfaces para 0 (sem estado) */
    for (int i = 0; i < MAX_INTERFACE; i++) {
        new_entry->interface_states[i] = 0;
    }
    
    /* Define o estado para a interface especificada */
    new_entry->interface_states[interface_id] = state;
    
    /* Regista o tempo atual */
    new_entry->timestamp = time(NULL);
    
    /* Adiciona à lista de entradas de interesse */
    new_entry->next = node.interest_table;
    node.interest_table = new_entry;
    
    printf("Added interest entry for %s with interface %d in state %d\n", 
           name, interface_id, state);
    display_interest_table_update("Entry Added", name);
    return 0;
}

/**
 * Atualiza uma entrada de interesse.
 * 
 * @param name Nome do objeto associado ao interesse
 * @param interface_id ID da interface a atualizar
 * @param state Novo estado para a interface (RESPONSE, WAITING ou CLOSED)
 * @return 0 em caso de sucesso, -1 em caso de erro
 */

int update_interest_entry(char *name, int interface_id, enum interface_state state) {
    /* Procura a entrada */
    InterestEntry *entry = find_interest_entry(name);
    
    if (entry != NULL) {
        if (entry->marked_for_removal) {
            printf("%sWARNING: Updating interest entry for %s that is marked for removal%s\n", 
                  COLOR_RED, name, COLOR_RESET);
            display_interest_table_update("Update Error", name);
            return -1;
        }
        
        /* Regista a transição de estado para depuração */
        enum interface_state old_state = entry->interface_states[interface_id];
        entry->interface_states[interface_id] = state;
        
        printf("INTEREST UPDATE: %s - interface %d: %s -> %s\n", 
               name, interface_id, 
               state_to_string(old_state), 
               state_to_string(state));
        
        /* Display updated interest table */
        display_interest_table_update("State Updated", name);
        
        return 0;
    }
    
    /* Entrada não encontrada, cria-a */
    printf("%sInterest entry for %s not found, creating new entry%s\n", 
          COLOR_YELLOW, name, COLOR_RESET);
    return add_interest_entry(name, interface_id, state);
}

/**
 * Remove uma entrada de interesse.
 * 
 * @param name Nome do objeto associado à entrada a remover
 * @return 0 em caso de sucesso, -1 se a entrada não for encontrada
 */
int remove_interest_entry(char *name) {
    /* Procura a entrada */
    InterestEntry *prev = NULL;
    InterestEntry *curr = node.interest_table;
    
    while (curr != NULL) {
        if (strcmp(curr->name, name) == 0) {
            /* Remove da lista */
            if (prev == NULL) {
                node.interest_table = curr->next;
            } else {
                prev->next = curr->next;
            }
            
            free(curr);
            printf("Removed interest entry for %s\n", name);
            display_interest_table_update("Entry Removed", name);
            return 0;
        }
        
        prev = curr;
        curr = curr->next;
    }
    display_interest_table_update("Entry Removed", name);
    return -1;  /* Entrada não encontrada */
}

/**
 * Remove espaços em branco no início e no fim de uma string.
 * 
 * @param str String a ser aparada
 */
void trim(char *str) {
    char *start = str;
    char *end;
    
    /* Remove espaços em branco iniciais */
    while (isspace(*start)) {
        start++;
    }
    
    if (*start == '\0') {
        /* String é toda espaços em branco */
        *str = '\0';
        return;
    }
    
    /* Remove espaços em branco finais */
    end = start + strlen(start) - 1;
    while (end > start && isspace(*end)) {
        end--;
    }
    
    /* Termina a string aparada com null */
    *(end + 1) = '\0';
    
    /* Move a string aparada para o início da string de entrada */
    if (start != str) {
        memmove(str, start, (end - start) + 2);
    }
}

/**
 * Procura ou cria uma entrada de interesse.
 * 
 * @param name Nome do objeto associado à entrada de interesse
 * @return Apontador para a entrada existente ou nova, NULL em caso de erro
 */
InterestEntry* find_or_create_interest_entry(char *name) {
    /* Verifica se a entrada já está marcada para remoção */
    InterestEntry *entry = node.interest_table;
    while (entry != NULL) {
        if (strcmp(entry->name, name) == 0) {
            if (entry->marked_for_removal) {
                printf("WARNING: Accessing interest entry for %s that is marked for removal\n", name);
            }
            return entry;
        }
        entry = entry->next;
    }
    
    /* Cria nova entrada se não encontrada */
    entry = malloc(sizeof(InterestEntry));
    if (entry == NULL) {
        perror("malloc");
        return NULL;
    }
    
    strcpy(entry->name, name);
    
    /* Inicializa todas as interfaces para 0 (sem estado) */
    for (int i = 0; i < MAX_INTERFACE; i++) {
        entry->interface_states[i] = 0;
    }
    
    /* Inicializa novos campos */
    entry->timestamp = time(NULL);
    entry->marked_for_removal = 0;
    
    /* Adiciona à lista de entradas de interesse */
    entry->next = node.interest_table;
    node.interest_table = entry;
    
    printf("INTEREST CREATED: New interest entry for %s\n", name);
    
    return entry;
}

/**
 * Verifica se um nome é válido (alfanumérico e até MAX_OBJECT_NAME caracteres).
 * 
 * @param name Nome a verificar
 * @return 1 se o nome for válido, 0 caso contrário
 */
int is_valid_name(char *name) {
    if (name == NULL || strlen(name) == 0 || strlen(name) > MAX_OBJECT_NAME) {
        return 0;
    }
    
    /* Verifica se todos os caracteres são alfanuméricos */
    for (int i = 0; name[i]; i++) {
        if (!isalnum(name[i])) {
            return 0;
        }
    }
    
    return 1;
}