/**
 * @file objects.h
 * @brief Funções para gestão de objetos e tabela de interesses na rede NDN
 * @author Bárbara Gonçalves Modesto e António Pedro Lima Loureiro Alves
 * @date Março de 2025
 *
 * Este ficheiro contém as declarações de funções para gerir objetos locais, 
 * objetos em cache e entradas na tabela de interesses numa rede NDN.
 * 
 * Inclui funções para:
 * - Adicionar e remover objetos locais
 * - Adicionar objetos à cache com gestão de tamanho
 * - Procurar objetos localmente ou na cache
 * - Gerir entradas de interesses pendentes
 * - Validar nomes de objetos
 */

#ifndef OBJECTS_H
#define OBJECTS_H

#include "ndn.h"

/**
 * @brief Adiciona um novo objeto à lista de objetos do nó.
 * 
 * Cria um novo objeto com o nome especificado e adiciona-o à lista
 * de objetos locais do nó.
 * 
 * @param name Nome do objeto a adicionar
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int add_object(char *name);

/**
 * @brief Remove um objeto da lista de objetos do nó.
 * 
 * Procura e remove um objeto da lista de objetos locais do nó.
 * 
 * @param name Nome do objeto a remover
 * @return 0 em caso de sucesso, -1 se o objeto não for encontrado
 */
int remove_object(char *name);

/**
 * @brief Adiciona um objeto à cache do nó.
 * 
 * Adiciona um objeto à cache do nó. Se a cache estiver cheia,
 * remove o objeto mais antigo para dar lugar ao novo.
 * 
 * @param name Nome do objeto a adicionar à cache
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int add_to_cache(char *name);

/**
 * @brief Procura um objeto na lista de objetos do nó.
 * 
 * Verifica se o objeto com o nome especificado existe na lista
 * de objetos locais do nó.
 * 
 * @param name Nome do objeto a procurar
 * @return 0 se o objeto for encontrado, -1 caso contrário
 */
int find_object(char *name);

/**
 * @brief Procura um objeto na cache do nó.
 * 
 * Verifica se o objeto com o nome especificado existe na cache do nó.
 * 
 * @param name Nome do objeto a procurar na cache
 * @return 0 se o objeto for encontrado, -1 caso contrário
 */
int find_in_cache(char *name);

/**
 * @brief Procura uma entrada na tabela de interesses.
 * 
 * Procura uma entrada na tabela de interesses pelo nome do objeto.
 * 
 * @param name Nome do objeto associado à entrada de interesse
 * @return Apontador para a entrada se encontrada, NULL caso contrário
 */
InterestEntry* find_interest_entry(char *name);

/**
 * @brief Procura ou cria uma entrada na tabela de interesses.
 * 
 * Procura uma entrada na tabela de interesses pelo nome do objeto e,
 * se não encontrar, cria uma nova entrada.
 * 
 * @param name Nome do objeto associado à entrada de interesse
 * @return Apontador para a entrada existente ou nova, NULL em caso de erro
 */
InterestEntry* find_or_create_interest_entry(char *name);

/**
 * @brief Adiciona uma nova entrada na tabela de interesses.
 * 
 * Cria uma nova entrada na tabela de interesses para o objeto especificado
 * e define o estado de uma interface.
 * 
 * @param name Nome do objeto associado ao interesse
 * @param interface_id ID da interface a atualizar
 * @param state Estado a definir para a interface (RESPONSE, WAITING ou CLOSED)
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int add_interest_entry(char *name, int interface_id, enum interface_state state);

/**
 * @brief Atualiza uma entrada existente na tabela de interesses.
 * 
 * Atualiza o estado de uma interface para um interesse existente.
 * Se a entrada não existir, cria uma nova.
 * 
 * @param name Nome do objeto associado ao interesse
 * @param interface_id ID da interface a atualizar
 * @param state Novo estado para a interface (RESPONSE, WAITING ou CLOSED)
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int update_interest_entry(char *name, int interface_id, enum interface_state state);

/**
 * @brief Remove uma entrada da tabela de interesses.
 * 
 * Remove uma entrada da tabela de interesses pelo nome do objeto.
 * 
 * @param name Nome do objeto associado à entrada a remover
 * @return 0 em caso de sucesso, -1 se a entrada não for encontrada
 */
int remove_interest_entry(char *name);

/**
 * @brief Remove espaços em branco no início e no fim de uma string.
 * 
 * Modifica a string original, removendo espaços em branco no início e no fim.
 * 
 * @param str String a ser aparada
 */
void trim(char *str);

/**
 * @brief Verifica se um nome é válido para um objeto.
 * 
 * Verifica se o nome contém apenas caracteres alfanuméricos e
 * tem tamanho dentro do limite permitido.
 * 
 * @param name Nome a verificar
 * @return 1 se o nome for válido, 0 caso contrário
 */
int is_valid_name(char *name);

#endif /* OBJECTS_H */