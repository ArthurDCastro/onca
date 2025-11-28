#ifndef GRAPH_H
#define GRAPH_H

#define GRAPH_MAX_VERTICES 64
#define GRAPH_MAX_NEIGHBORS 8

/* ---------------- Structs ---------------- */

typedef struct {
	int row;
	int col;
} Coordinate;

typedef struct {
	Coordinate c;
	int neighbors[GRAPH_MAX_NEIGHBORS]; /* IDs dos vizinhos */
	int degree;							/* numero de vizinhos */
} Vertex;

typedef struct {
	Vertex v[GRAPH_MAX_VERTICES];
	int num_vertices;
} Graph;

/* ---------------- Funcoes publicas ---------------- */

/**
 * @brief Carrega o mapa do arquivo e constroi o grafo completo.
 *
 * Inicializa o Graph, carrega o Map, cria vertices e adjacencias.
 * O usuario so precisa chamar esta funcao para ter um grafo pronto.
 */
int graph_create (Graph* g, const char* map_path);

/**
 * @brief Retorna a coordenada (linha, coluna) de um vertice.
 *
 * Os valores retornados correspondem à posicao do vertice no mapa ASCII
 * usado na construcao do grafo.
 *
 * @param g Ponteiro para o grafo.
 * @param vertex_id Indice do vertice em g->v[].
 * @param row Ponteiro para receber a linha.
 * @param col Ponteiro para receber a coluna.
 * @return 0 em caso de sucesso, diferente de 0 em erro.
 */
int graph_get_coord (const Graph* g, int vertex_id, int* row, int* col);

/**
 * @brief Retorna o ID do vertice cuja coordenada original é (row, col).
 *
 * Esta funcao realiza uma busca linear em g->v[], pois o grafo possui
 * poucos vertices e essa operacao eh barata.
 *
 * @param g Ponteiro para o grafo.
 * @param row Linha do vertice no mapa ASCII usado na construcao.
 * @param col Coluna correspondente.
 * @return ID do vertice (>=0) em caso de sucesso, -1 se nao encontrado.
 */
int graph_get_index (const Graph* g, int row, int col);

/**
 * @brief Verifica se dois vertices sao vizinhos diretos no grafo.
 *
 * Um vertice eh vizinho do outro se existir uma aresta entre eles
 * (isto é: se um aparece na lista de neighbors do outro).
 *
 * @param g Ponteiro para o grafo.
 * @param a ID de um vertice.
 * @param b ID de outro vertice.
 * @return 1 se sao vizinhos, 0 se nao sao, <0 em caso de erro.
 */
int graph_is_neighbor (const Graph* g, int a, int b);

/**
 * @brief Copia os vizinhos de um vertice para um buffer externo.
 *
 * @param g            Ponteiro para o grafo.
 * @param vid          Indice do vertice.
 * @param out_neighbors Vetor de saida para receber os vizinhos.
 * @param degree   Retorna a quantidade de vizinhos copiados.
 *
 * @return 0 em sucesso, <0 em erro.
 */
int graph_get_neighbors (const Graph* g, int vid, int out_neighbors[], int* degree);

void print_graph (const Graph* g);

#endif /* GRAPH_H */
