#ifndef GRAPH_H
#define GRAPH_H

#define GRAPH_MAX_VERTICES 64
#define GRAPH_MAX_NEIGHBORS 8

/* ---------------- Structs ---------------- */

typedef struct {
	int row;
	int col;
} Coordinate;

typedef enum {
	PIECE_EMPTY = 0,
	PIECE_DOG,
	PIECE_JAGUAR
} Piece;

typedef struct {
	Piece piece;
	Coordinate c;
	int neighbors[GRAPH_MAX_NEIGHBORS]; /* IDs dos vizinhos */
	int degree;							/* numero de vizinhos */
} Vertex;

typedef struct {
	Vertex v[GRAPH_MAX_VERTICES];
	int num_vertices;

	int jaguar_pos;
	int num_dogs;

	Piece to_move; /* lado a jogar */
} Graph;

/* ---------------- Funcoes publicas ---------------- */

/**
 * @brief Carrega o mapa do arquivo e constroi o grafo completo.
 *
 * Inicializa o Graph, carrega o Map, cria vertices e adjacencias.
 * O usuario so precisa chamar esta funcao para ter um grafo pronto.
 */
int graph_create (Graph* g, const char* map_path);

#endif /* GRAPH_H */
