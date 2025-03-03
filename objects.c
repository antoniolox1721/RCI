/*
 * Implementação de Rede de Dados Identificados por Nome (NDN)
 * Redes de Computadores e Internet - 2024/2025
 * 
 * objects.c - Implementação das funções de manipulação de objetos
 */

#include "objects.h"
#include "debug_utils.h"
/*
 * Adiciona um objeto
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

/*
 * Remove um objeto
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

/*
 * Adiciona um objeto à cache
 */

/*
 * Add an object to the cache with strict size limit enforcement
 */
int add_to_cache(char *name) {
    /* Sanity check for input */
    if (name == NULL || strlen(name) == 0) {
        fprintf(stderr, "Error: Attempting to cache invalid object name\n");
        return -1;
    }

    /* Check if the object already exists in the cache */
    Object *curr = node.cache;
    Object *prev = NULL;
    while (curr != NULL) {
        if (strcmp(curr->name, name) == 0) {
            /* Object already in cache, no need to add again */
            return 0;
        }
        prev = curr;
        curr = curr->next;
    }
    
    /* Ensure cache does not exceed maximum size */
    while (node.current_cache_size >= node.cache_size) {
        if (node.cache == NULL) {
            /* Unexpected state - cache is marked as full but empty */
            fprintf(stderr, "Warning: Cache size inconsistency detected\n");
            node.current_cache_size = 0;
            break;
        }
        
        /* Remove the first (oldest) object */
        Object *oldest = node.cache;
        node.cache = oldest->next;
        
        printf("Cache full. Removing oldest object: %s to make room for %s\n", 
               oldest->name, name);
        
        free(oldest);
        node.current_cache_size--;
    }
    
    /* Create a new cache entry */
    Object *new_object = malloc(sizeof(Object));
    if (new_object == NULL) {
        perror("malloc");
        return -1;
    }
    
    /* Copy the name, ensuring no buffer overflow */
    strncpy(new_object->name, name, MAX_OBJECT_NAME);
    new_object->name[MAX_OBJECT_NAME] = '\0';  /* Ensure null-termination */
    new_object->next = NULL;
    
    /* Add to the end of the cached objects list */
    if (node.cache == NULL) {
        /* First object in cache */
        node.cache = new_object;
    } else {
        /* Find the last object and append */
        curr = node.cache;
        while (curr->next != NULL) {
            curr = curr->next;
        }
        curr->next = new_object;
    }
    
    /* Increment cache size */
    node.current_cache_size++;
    
    printf("Added object %s to cache (size: %d/%d)\n", 
           name, node.current_cache_size, node.cache_size);
    
    /* Final sanity check to prevent cache size overflow */
    if (node.current_cache_size > node.cache_size) {
        fprintf(stderr, "CRITICAL ERROR: Cache size exceeded maximum limit!\n");
        /* You might want to handle this more gracefully depending on your error handling strategy */
        exit(EXIT_FAILURE);
    }
    if (prev != NULL) { /* Used for traversal, silencing compiler warning */ }
    return 0;
}
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
/*
 * Procura um objeto pelo nome
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

/*
 * Procura um objeto na cache
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

/*
 * Adiciona uma entrada de interesse
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
    
    return 0;
}

/*
 * Atualiza uma entrada de interesse
 */
int update_interest_entry(char *name, int interface_id, enum interface_state state) {
    /* Procura a entrada */
    InterestEntry *entry = find_interest_entry(name);
    
    if (entry != NULL) {
        if (entry->marked_for_removal) {
            printf("WARNING: Updating interest entry for %s that is marked for removal\n", name);
            return -1;
        }
        
        /* Regista a transição de estado para depuração */
        enum interface_state old_state = entry->interface_states[interface_id];
        entry->interface_states[interface_id] = state;
        
        printf("INTEREST UPDATE: %s - interface %d: %s -> %s\n", 
               name, interface_id, 
               state_to_string(old_state), 
               state_to_string(state));
        return 0;
    }
    
    /* Entrada não encontrada, cria-a */
    return add_interest_entry(name, interface_id, state);
}

/*
 * Remove uma entrada de interesse
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
            return 0;
        }
        
        prev = curr;
        curr = curr->next;
    }
    
    return -1;  /* Entrada não encontrada */
}

/*
 * Remove espaços em branco no início e no fim de uma string
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

/*
 * Procura ou cria uma entrada de interesse
 */
/*
 * Procura ou cria uma entrada de interesse
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


/*
 * Verifica se um nome é válido (alfanumérico e até MAX_OBJECT_NAME caracteres)
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