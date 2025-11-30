#include "graph.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	int rows;
	int cols;
	char** data;	/* mapa ASCII (alocado dinamicamente) */
	int** vertices; /* mapeia (i,j) -> id do vertice ou -1 */
} Map;

/**
 * @brief Inicializa um vertice do grafo.
 *
 * Define:
 *   - coordenadas (row, col)
 *   - degree = 0
 *   - neighbors[] = -1
 *
 * @param v Ponteiro para o vertice.
 * @param row Linha no mapa ASCII (ou -1 se ainda desconhecida).
 * @param col Coluna no mapa ASCII (ou -1 se ainda desconhecida).
 * @return 0 em caso de sucesso, diferente de 0 em erro.
 */
int vertex_init (Vertex* v, int row, int col) {
	v->c.row = row;
	v->c.col = col;
	v->degree = 0;

	for ( int k = 0; k < GRAPH_MAX_NEIGHBORS; k++ )
		v->neighbors[k] = -1;

	return 0;
}

/**
 * Inicializa a estrutura Graph.
 *
 * Esta funcao zera todos os campos do grafo e define o numero maximo
 * de vertices que sera utilizado. Nao faz nenhuma leitura de mapa.
 *
 * @param g Ponteiro para a estrutura Graph.
 * @param n Numero maximo de vertices (deve ser <= GRAPH_MAX_VERTICES).
 * @return 0 em caso de sucesso, diferente de 0 em erro.
 */

int graph_init (Graph* g, int n) {
	if ( n <= 0 || n > GRAPH_MAX_VERTICES ) {
		fprintf (stderr,
				 "graph_init: valor n (%d) invalido, deve estar entre 1 e %d\n",
				 n, GRAPH_MAX_VERTICES);
		return -2;
	}

	/* Zera toda a estrutura */
	memset (g, 0, sizeof (Graph));

	g->num_vertices = 0;

	/* Inicializa todos os vertices */
	for ( int i = 0; i < GRAPH_MAX_VERTICES; i++ )
		vertex_init (&g->v[i], -1, -1);

	return 0;
}

/**
 * Carrega o mapa ASCII a partir de um arquivo.
 *
 * Esta funcao aloca dinamicamente as matrizes m->data e m->vertices,
 * preenche rows e cols, e copia o conteudo do arquivo para m->data.
 * A matriz m->vertices deve ser inicializada com -1 em todas as posicoes.
 *
 * @param m Ponteiro para a estrutura Map a ser preenchida.
 * @param map_path Caminho do arquivo de mapa.
 * @return 0 em caso de sucesso, diferente de 0 em erro.
 */
int load_map (Map* m, const char* map_path) {
	FILE* f;
	int rows, cols;
	int expected_vertices;
	char buffer[1024];

	if ( !m || !map_path ) {
		fprintf (stderr, "load_map: ponteiro nulo (m=%p, map_path=%p)\n",
				 (void*)m, (void*)map_path);
		return -1;
	}

	f = fopen (map_path, "r");
	if ( !f ) {
		fprintf (stderr, "load_map: nao foi possivel abrir arquivo '%s'\n",
				 map_path);
		return -2;
	}

	/* primeira linha: numero de vertices (por enquanto so le e descarta) */
	if ( fscanf (f, "%d", &expected_vertices) != 1 ) {
		fprintf (stderr, "load_map: erro ao ler numero de vertices\n");
		fclose (f);
		return -3;
	}

	/* segunda linha: rows cols */
	if ( fscanf (f, "%d %d", &rows, &cols) != 2 ) {
		fprintf (stderr, "load_map: erro ao ler dimensoes do mapa\n");
		fclose (f);
		return -4;
	}

	fgets (buffer, sizeof (buffer), f); /* consome resto da linha */

	m->rows = rows;
	m->cols = cols;

	/* aloca linhas de data */
	m->data = malloc (rows * sizeof (char*));
	if ( !m->data ) {
		fprintf (stderr, "load_map: falha ao alocar linhas de data\n");
		fclose (f);
		return -5;
	}

	/* aloca linhas de vertices */
	m->vertices = malloc (rows * sizeof (int*));
	if ( !m->vertices ) {
		fprintf (stderr, "load_map: falha ao alocar linhas de vertices\n");
		free (m->data);
		fclose (f);
		return -6;
	}

	/* aloca colunas (pattern Mazieiro) */
	for ( int i = 0; i < rows; i++ ) {
		m->data[i] = malloc ((cols + 1) * sizeof (char));
		if ( !m->data[i] ) {
			fprintf (stderr, "load_map: falha ao alocar coluna %d de data\n", i);

			for ( int k = 0; k < i; k++ ) free (m->data[k]);
			free (m->data);

			for ( int k = 0; k < i; k++ ) free (m->vertices[k]);
			free (m->vertices);

			fclose (f);
			return -7;
		}

		m->vertices[i] = malloc (cols * sizeof (int));
		if ( !m->vertices[i] ) {
			fprintf (stderr, "load_map: falha ao alocar coluna %d de vertices\n", i);

			for ( int k = 0; k < i; k++ ) free (m->data[k]);
			free (m->data);

			for ( int k = 0; k < i; k++ ) free (m->vertices[k]);
			free (m->vertices);

			fclose (f);
			return -8;
		}
	}

	/* inicializa vertices com -1 */
	for ( int i = 0; i < rows; i++ )
		for ( int j = 0; j < cols; j++ )
			m->vertices[i][j] = -1;

	/* lê ASCII */
	for ( int i = 0; i < rows; i++ ) {
		if ( !fgets (buffer, sizeof (buffer), f) ) {
			fprintf (stderr, "load_map: erro ao ler linha %d do mapa\n", i);
			fclose (f);
			return -9;
		}

		for ( int j = 0; j < cols; j++ ) {
			char ch = buffer[j];
			if ( ch == '\0' || ch == '\n' )
				m->data[i][j] = ' ';
			else
				m->data[i][j] = ch;
		}

		m->data[i][cols] = '\0';
	}

	fclose (f);
	return 0;
}

/**
 * Libera a memoria associada a um Map.
 *
 * Esta funcao libera todas as linhas de m->data e m->vertices,
 * e em seguida libera os proprios vetores de ponteiros.
 *
 * @param m Ponteiro para a estrutura Map a ser destruida.
 * @return 0 em caso de sucesso, diferente de 0 em erro.
 */
int destroy_map (Map* m) {
	if ( !m ) {
		fprintf (stderr, "destroy_map: ponteiro m == NULL\n");
		return -1;
	}

	if ( m->data ) {
		for ( int i = 0; i < m->rows; i++ ) {
			free (m->data[i]);
		}
		free (m->data);
		m->data = NULL;
	}

	if ( m->vertices ) {
		for ( int i = 0; i < m->rows; i++ ) {
			free (m->vertices[i]);
		}
		free (m->vertices);
		m->vertices = NULL;
	}

	m->rows = 0;
	m->cols = 0;

	return 0;
}

/**
 * Verifica se a posicao (i,j) do mapa contem um vertice valido.
 *
 * Um vertice valido eh representado pelo caractere '*'
 * em m->data[i][j].
 *
 * @param m Ponteiro para o mapa.
 * @param i Indice da linha (0..m->rows-1).
 * @param j Indice da coluna (0..m->cols-1).
 * @return 1 se for um vertice, 0 caso contrario.
 */
int is_vertice (const Map* m, int i, int j) {
	/* checa limites da matriz */
	if ( i < 0 || i >= m->rows || j < 0 || j >= m->cols ) {
		fprintf (stderr, "is_vertice: indices fora dos limites (i=%d, j=%d)\n", i, j);
		return 0;
	}

	/* um vertice valido eh representado por '*' no mapa ASCII */
	return (m->data[i][j] == '*') ? 1 : 0;
}

/**
 * Cria um vertice correspondente a posicao (i,j), se necessario.
 *
 * Esta funcao usa m->vertices[i][j] para verificar se o vertice
 * ja foi criado. Caso ainda nao exista, cria um novo vertice em g->v[],
 * atualiza g->num_vertices, preenche a coordenada e grava o id em
 * m->vertices[i][j].
 *
 * @param g Ponteiro para o grafo.
 * @param m Ponteiro para o mapa.
 * @param i Linha no mapa ASCII.
 * @param j Coluna no mapa ASCII.
 * @param vertex_id Ponteiro para receber o indice do vertice em g->v[].
 *
 * @return
 *   < 0 : erro
 *     0 : vertice ja existia
 *     1 : vertice criado agora
 */
int create_vertice (Graph* g, Map* m, int i, int j, int* vertex_id) {
	if ( !is_vertice (m, i, j) ) {
		fprintf (stderr, "create_vertice: vao eh vertice id\n");
		return -1;
	}

	if ( !vertex_id ) {
		fprintf (stderr, "create_vertice: ponteiro nulo id\n");
		return -2;
	}

	/* ja existe vertice criado para (i,j)? */
	if ( m->vertices[i][j] != -1 ) {
		*vertex_id = m->vertices[i][j];
		return 0; /* nada a criar */
	}

	/* garante que cabemos no vetor */
	if ( g->num_vertices >= GRAPH_MAX_VERTICES ) {
		fprintf (stderr,
				 "create_vertice: limite de vertices atingido (%d)\n",
				 GRAPH_MAX_VERTICES);
		return -3;
	}

	/* cria o vertice */
	int id = g->num_vertices++;

	vertex_init (&g->v[id], i / 3 + 1, j / 3 + 1);

	/* marca na matriz (i,j) -> id */
	m->vertices[i][j] = id;

	*vertex_id = id;
	return 1; /* criado agora */
}

/**
 * Explora arestas a partir de um vertice inicial.
 *
 * A partir da posicao (i,j) e do vertex_id correspondente, esta funcao
 * segue as arestas no mapa ASCII (caracteres '-', '|', '/' e '\\'),
 * encontra vertices vizinhos, chama create_vertice para eles e
 * adiciona seus ids em g->v[vertex_id].neighbors, atualizando degree.
 *
 * A exploracao pode ser recursiva para visitar todo o componente conexo.
 *
 * @param g Ponteiro para o grafo.
 * @param m Ponteiro para o mapa.
 * @param i Linha do vertice inicial no mapa ASCII.
 * @param j Coluna do vertice inicial no mapa ASCII.
 * @param vertex_id Id do vertice inicial em g->v[].
 *
 * @return 0 em caso de sucesso, diferente de 0 em erro.
 */
int explorer (Graph* g, Map* m, int i, int j, int vertex_id) {
	if ( !g || !m ) {
		fprintf (stderr, "explorer: ponteiro nulo (g=%p, m=%p)\n",
				 (void*)g, (void*)m);
		return -1;
	}

	/* checa limites basicos */
	if ( i < 0 || i >= m->rows || j < 0 || j >= m->cols ) {
		fprintf (stderr,
				 "explorer: indices fora dos limites (i=%d, j=%d)\n", i, j);
		return -2;
	}

	if ( vertex_id < 0 || vertex_id >= GRAPH_MAX_VERTICES ) {
		fprintf (stderr,
				 "explorer: vertex_id (%d) fora dos limites\n", vertex_id);
		return -3;
	}

	/* opcional: conferir se o mapa concorda com o id */
	if ( m->vertices[i][j] != vertex_id ) {
		fprintf (stderr,
				 "explorer: m->vertices[%d][%d]=%d mas vertex_id=%d\n",
				 i, j, m->vertices[i][j], vertex_id);
		/* nao retorno erro duro aqui, so aviso */
	}

	/* direcoes (dr, dc) e caracteres esperados de aresta nessa direcao */
	int dr[8] = {0, 0, 1, -1, 1, 1, -1, -1};
	int dc[8] = {1, -1, 0, 0, 1, -1, 1, -1};
	char edge[8] = {'-', '-', '|', '|', '\\', '/', '/', '\\'};
	int neighbor_id;

	for ( int dir = 0; dir < 8; dir++ ) {
		int r = i + dr[dir];
		int c = j + dc[dir];

		/* checa se a primeira posicao na direcao contem o caractere da aresta */
		if ( r < 0 || r >= m->rows || c < 0 || c >= m->cols )
			continue;

		if ( m->data[r][c] != edge[dir] )
			continue;

		/* caminha pela aresta ate achar outro vertice ou algo invalido */
		while ( 1 ) {
			r += dr[dir];
			c += dc[dir];

			if ( r < 0 || r >= m->rows || c < 0 || c >= m->cols )
				break;

			if ( is_vertice (m, r, c) ) {
				int status = create_vertice (g, m, r, c, &neighbor_id);

				if ( status < 0 ) {
					/* erro em create_vertice */
					return status;
				}

				/* adiciona aresta vertex_id <-> neighbor_id, sem duplicar */
				Vertex* v = &g->v[vertex_id];
				Vertex* vn = &g->v[neighbor_id];

				/* adiciona neighbor_id em v->neighbors se ainda nao estiver */
				int exists = 0;
				for ( int k = 0; k < v->degree; k++ ) {
					if ( v->neighbors[k] == neighbor_id ) {
						exists = 1;
						break;
					}
				}
				if ( !exists ) {
					if ( v->degree < GRAPH_MAX_NEIGHBORS ) {
						v->neighbors[v->degree++] = neighbor_id;
					} else {
						fprintf (stderr,
								 "explorer: vertice %d excedeu max de vizinhos\n",
								 vertex_id);
					}
				}

				/* adiciona vertex_id em vn->neighbors se ainda nao estiver */
				exists = 0;
				for ( int k = 0; k < vn->degree; k++ ) {
					if ( vn->neighbors[k] == vertex_id ) {
						exists = 1;
						break;
					}
				}
				if ( !exists ) {
					if ( vn->degree < GRAPH_MAX_NEIGHBORS ) {
						vn->neighbors[vn->degree++] = vertex_id;
					} else {
						fprintf (stderr,
								 "explorer: vertice vizinho %d excedeu max de vizinhos\n",
								 neighbor_id);
					}
				}

				/* se o vertice vizinho foi criado agora, explorar a partir dele */
				if ( status > 0 ) {
					status = explorer (g, m, r, c, neighbor_id);
					if ( status != 0 )
						return status;
				}

				break; /* para de caminhar nesta direcao apos achar um vertice */
			}

			/* se nao eh nem aresta nem vertice, para */
			if ( m->data[r][c] != edge[dir] )
				break;
		}
	}

	return 0;
}

/**
 * Constroi o grafo completo a partir de um arquivo de mapa.
 *
 * Esta funcao:
 *   - chama graph_init(g, GRAPH_MAX_VERTICES)
 *   - chama load_map para carregar o mapa
 *   - percorre todas as posicoes do mapa
 *   - para cada vertice encontrado, chama create_vertice e explorer
 *   - ao final, chama destroy_map para liberar o mapa
 *
 * @param g Ponteiro para o grafo a ser construido.
 * @param map_path Caminho para o arquivo de mapa.
 * @return 0 em caso de sucesso, diferente de 0 em erro.
 */
int graph_create (Graph* g, const char* map_path) {
	Map m;
	int status;

	if ( !g ) {
		fprintf (stderr, "graph_create: ponteiro g == NULL\n");
		return -1;
	}
	if ( !map_path ) {
		fprintf (stderr, "graph_create: map_path == NULL\n");
		return -2;
	}

	/* inicializa o grafo */
	status = graph_init (g, GRAPH_MAX_VERTICES);
	if ( status != 0 ) {
		fprintf (stderr, "graph_create: graph_init falhou (codigo %d)\n", status);
		return status;
	}

	/* carrega o mapa */
	status = load_map (&m, map_path);
	if ( status != 0 ) {
		fprintf (stderr, "graph_create: load_map falhou para '%s' (codigo %d)\n",
				 map_path, status);
		return status;
	}

	/* varre o mapa */
	for ( int i = 0; i < m.rows; i++ ) {
		for ( int j = 0; j < m.cols; j++ ) {
			if ( !is_vertice (&m, i, j) )
				continue;

			int vertex_id = -1;
			status = create_vertice (g, &m, i, j, &vertex_id);

			if ( status < 0 ) {
				fprintf (stderr,
						 "graph_create: erro em create_vertice(%d,%d) (codigo %d)\n",
						 i, j, status);
				destroy_map (&m);
				return status;
			}

			if ( status > 0 ) {
				/* criado agora — explorar */
				status = explorer (g, &m, i, j, vertex_id);
				if ( status != 0 ) {
					fprintf (stderr,
							 "graph_create: erro em explorer(%d,%d,id=%d) (codigo %d)\n",
							 i, j, vertex_id, status);
					destroy_map (&m);
					return status;
				}
			}
		}
	}

	/* libera mapa */
	status = destroy_map (&m);
	if ( status != 0 ) {
		fprintf (stderr, "graph_create: destroy_map falhou (codigo %d)\n", status);
		return status;
	}

	return 0;
}

void print_graph (const Graph* g) {
	if ( !g ) {
		fprintf (stderr, "print_graph: ponteiro g == NULL\n");
		return;
	}

	printf ("=== Grafo (%d vertices) ===\n\n", g->num_vertices);

	for ( int v = 0; v < g->num_vertices; v++ ) {
		const Vertex* vert = &g->v[v];

		printf ("Vertice %02d  (%d,%d): ",
				v, vert->c.row, vert->c.col);

		for ( int k = 0; k < vert->degree; k++ ) {
			printf ("%02d ", vert->neighbors[k]);
		}

		printf ("\n");
	}
}

int graph_get_coord (const Graph* g, int vertex_id, int* row, int* col) {
	if ( vertex_id < 0 || vertex_id >= g->num_vertices ) {
		fprintf (stderr,
				 "graph_get_coord: vertex_id (%d) fora do intervalo [0, %d)\n",
				 vertex_id, g->num_vertices);
		return -3;
	}

	const Vertex* v = &g->v[vertex_id];

	*row = v->c.row;
	*col = v->c.col;

	return 0;
}

int graph_get_index (const Graph* g, int row, int col) {
	/* busca linear nos vertices */
	for ( int v = 0; v < g->num_vertices; v++ ) {
		if ( g->v[v].c.row == row &&
			 g->v[v].c.col == col ) {
			return v; /* achou */
		}
	}

	/* nao encontrado */
	return -1;
}

int graph_is_neighbor (const Graph* g, int a, int b) {
	if ( a < 0 || a >= g->num_vertices ||
		 b < 0 || b >= g->num_vertices ) {
		fprintf (stderr,
				 "graph_is_neighbor: ids fora do intervalo "
				 "(a=%d, b=%d, num_vertices=%d)\n",
				 a, b, g->num_vertices);
		return -2;
	}

	const Vertex* va = &g->v[a];

	for ( int k = 0; k < va->degree; k++ ) {
		if ( va->neighbors[k] == b ) {
			return 1; /* sao vizinhos */
		}
	}

	return 0; /* nao sao vizinhos */
}

int graph_get_neighbors (const Graph* g, int vid, int out_neighbors[], int* degree) {
	if ( GRAPH_MAX_NEIGHBORS < g->v[vid].degree ) {
		fprintf (stderr,
				 "graph_get_neighbors: max_neighbors menor que degree (%d < %d)\n",
				 GRAPH_MAX_NEIGHBORS, g->v[vid].degree);
		return -3;
	}

	*degree = g->v[vid].degree;

	for ( int i = 0; i < g->v[vid].degree; i++ )
		out_neighbors[i] = g->v[vid].neighbors[i];

	return 0;
}

int graph_degree (const Graph* g, int vid) {
	if ( vid < 0 || vid >= g->num_vertices ) {
		fprintf (stderr,
				 "graph_degree: vid fora do intervalo (vid=%d, num_vertices=%d)\n",
				 vid, g->num_vertices);
		return -2;
	}

	return g->v[vid].degree;
}
