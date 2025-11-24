#ifndef GRAPH_H
#define GRAPH_H

#include "board.h"

#define GRAPH_MAX_VERTICES BOARD_MAX_VERTICES
#define GRAPH_MAX_NEIGHBORS 8

/**
 * @brief Inicializa o grafo do tabuleiro a partir de um arquivo de mapa ASCII.
 *
 * O arquivo deve conter o desenho do tabuleiro com os vertices (ex: '*')
 * e as ligacoes entre eles (ex: '-', '|', '/', '\').
 *
 * @param map_path Caminho para o arquivo de mapa.
 * @return 0 em caso de sucesso, diferente de 0 em caso de erro.
 */
int graph_init (const char* map_path);

/**
 * @brief Retorna o numero de vertices do tabuleiro.
 *
 * @return Numero de vertices (0..GRAPH_MAX_VERTICES).
 */
int graph_num_vertices (void);

/**
 * @brief Converte coordenadas (linha,coluna) em indice de vertice.
 *
 * Coordenadas seguem o sistema do controlador:
 *   linha: 1..7
 *   coluna: 1..5
 *
 * @param row Linha.
 * @param col Coluna.
 * @return Indice do vertice (0..) ou -1 se nao existir vertice nessa posicao.
 */
int graph_coord_to_index (int row, int col);

/**
 * @brief Converte indice de vertice em coordenadas (linha,coluna).
 *
 * @param idx Indice do vertice.
 * @param row Ponteiro para receber a linha (1..7).
 * @param col Ponteiro para receber a coluna (1..5).
 * @return 0 em caso de sucesso, diferente de 0 se idx invalido.
 */
int graph_index_to_coord (int idx, int* row, int* col);

/**
 * @brief Retorna a quantidade de vizinhos de um vertice.
 *
 * @param v Indice do vertice.
 * @return Numero de vizinhos (0..GRAPH_MAX_NEIGHBORS), ou -1 se v invalido.
 */
int graph_neighbor_count (int v);

/**
 * @brief Retorna o i-esimo vizinho de um vertice.
 *
 * @param v Indice do vertice.
 * @param i Indice do vizinho (0..graph_neighbor_count(v)-1).
 * @return Indice do vizinho ou -1 se parametros invalidos.
 */
int graph_get_neighbor (int v, int i);

/**
 * @brief Retorna 1 se dois vertices sao adjacentes, 0 caso contrario.
 *
 * @param a Vertice A.
 * @param b Vertice B.
 */
int graph_are_adjacent (int a, int b);

#endif	// GRAPH_H
