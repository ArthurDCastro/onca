#include <stdio.h>

#include "graph.h"

void print_graph (const Graph* g);

int main (void) {
	Graph g;

	/* caminho do mapa que voce colocou */
	const char* path = "map.txt";

	printf ("Carregando grafo a partir de: %s\n", path);

	int status = graph_create (&g, path);

	if ( status != 0 ) {
		fprintf (stderr, "Erro ao criar grafo: %d\n", status);
		return 1;
	}

	printf ("\n=== Grafo reconstruido ===\n");
	print_graph (&g);

	printf ("\n=== Lista de adjacencias ===\n");
	for ( int v = 0; v < g.num_vertices; v++ ) {
		printf ("Vertice %02d (%d,%d): ",
				v, g.v[v].c.row, g.v[v].c.col);
		for ( int k = 0; k < g.v[v].degree; k++ )
			printf ("%02d ", g.v[v].neighbors[k]);
		printf ("\n");
	}

	return 0;
}
