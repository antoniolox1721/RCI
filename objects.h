/*
 * Implementação de Rede de Dados Identificados por Nome (NDN)
 * Redes de Computadores e Internet - 2024/2025
 * 
 * objects.h - Funções para gestão de objetos
 */

#ifndef OBJECTS_H
#define OBJECTS_H

#include "ndn.h"

/**
 * Gestão de objetos
 */

/**
 * Adiciona um novo objeto à lista de objetos do nó.
 * 
 * @param name Nome do objeto a adicionar
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int add_object(char *name);

/**
 * Remove um objeto da lista de objetos do nó.
 * 
 * @param name Nome do objeto a remover
 * @return 0 em caso de sucesso, -1 se o objeto não for encontrado
 */
int remove_object(char *name);

/**
 * Adiciona um objeto à cache do nó.
 * Se a cache estiver cheia, remove o objeto mais antigo.
 * 
 * @param name Nome do objeto a adicionar à cache
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int add_to_cache(char *name);

/**
 * Procura um objeto na lista de objetos do nó.
 * 
 * @param name Nome do objeto a procurar
 * @return 0 se o objeto for encontrado, -1 caso contrário
 */
int find_object(char *name);

/**
 * Procura um objeto na cache do nó.
 * 
 * @param name Nome do objeto a procurar na cache
 * @return 0 se o objeto for encontrado, -1 caso contrário
 */
int find_in_cache(char *name);

/**
 * Procura uma entrada na tabela de interesses.
 * 
 * @param name Nome do objeto associado à entrada de interesse
 * @return Apontador para a entrada se encontrada, NULL caso contrário
 */
InterestEntry* find_interest_entry(char *name);

/**
 * Procura ou cria uma entrada na tabela de interesses.
 * 
 * @param name Nome do objeto associado à entrada de interesse
 * @return Apontador para a entrada existente ou nova, NULL em caso de erro
 */
InterestEntry* find_or_create_interest_entry(char *name);

/**
 * Gestão da tabela de interesses
 */

/**
 * Adiciona uma nova entrada na tabela de interesses.
 * 
 * @param name Nome do objeto associado ao interesse
 * @param interface_id ID da interface a atualizar
 * @param state Estado a definir para a interface (RESPONSE, WAITING ou CLOSED)
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int add_interest_entry(char *name, int interface_id, enum interface_state state);

/**
 * Atualiza uma entrada existente na tabela de interesses.
 * Se a entrada não existir, cria uma nova.
 * 
 * @param name Nome do objeto associado ao interesse
 * @param interface_id ID da interface a atualizar
 * @param state Novo estado para a interface (RESPONSE, WAITING ou CLOSED)
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int update_interest_entry(char *name, int interface_id, enum interface_state state);

/**
 * Remove uma entrada da tabela de interesses.
 * 
 * @param name Nome do objeto associado à entrada a remover
 * @return 0 em caso de sucesso, -1 se a entrada não for encontrada
 */
int remove_interest_entry(char *name);

/**
 * Funções utilitárias
 */

/**
 * Remove espaços em branco no início e no fim de uma string.
 * 
 * @param str String a ser aparada
 */
void trim(char *str);

/**
 * Verifica se um nome é válido (alfanumérico e com tamanho adequado).
 * 
 * @param name Nome a verificar
 * @return 1 se o nome for válido, 0 caso contrário
 */
int is_valid_name(char *name);

#endif /* OBJECTS_H */