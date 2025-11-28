#include <stdio.h>
#include <string.h>

#include "game.h"
#include "graph.h"

int main (void) {
	Game game;
	int err;

	/* inicializa jogo e grafo interno */
	err = game_init (&game);
	if ( err != 0 ) {
		fprintf (stderr, "main: game_init falhou (err=%d)\n", err);
		return 1;
	}
	print_graph (&(game.g));

	/* mesmo tabuleiro inicial do controlador.c */
	const char tabuleiro_inicial[] =
		"#######\n"
		"#ccccc#\n"
		"#ccccc#\n"
		"#ccocc#\n"
		"#-----#\n"
		"#-----#\n"
		"# --- #\n"
		"#- - -#\n"
		"#######\n";

	printf ("%s\n", tabuleiro_inicial);

	/* lado inicial: onca ('o') */
	err = game_from_controller_board (&game,
									  tabuleiro_inicial,
									  CTRL_JAGUAR_CHAR);
	if ( err != 0 ) {
		fprintf (stderr, "main: game_from_controller_board falhou (err=%d)\n", err);
		return 1;
	}

	printf ("Estado inicial carregado a partir do controlador.\n");

	printf ("Digite jogadas no formato do controlador (ex: \"o m 4 3 5 3\")\n");
	printf ("Linha vazia ou EOF encerra.\n\n");

	char line[256];

	while ( 1 ) {
		printf ("> ");
		if ( !fgets (line, sizeof line, stdin) )
			break; /* EOF */

		/* so enter -> sai */
		if ( line[0] == '\n' || line[0] == '\0' )
			break;

		/* remove \n */
		line[strcspn (line, "\r\n")] = '\0';

		Move mv;
		err = game_move_from_controller (&game, line, &mv);
		if ( err != 0 ) {
			fprintf (stderr,
					 "main: game_move_from_controller falhou (err=%d)\n", err);
			continue;
		}

		char out[256];
		err = game_move_to_controller (&game, &mv, out, (int)sizeof out);
		if ( err != 0 ) {
			fprintf (stderr,
					 "main: game_move_to_controller falhou (err=%d)\n", err);
			continue;
		}

		printf ("Round-trip: \"%s\" -> Move -> \"%s\"\n", line, out);
		if ( game_is_legal_move (&game, &mv) )
			game_apply_move (&game, &mv);
		else
			printf ("Movimento invalido: \"%s\" -> Move -> \"%s\"\n", line, out);

		game_print_board (&game);

		CellContent w = CELL_EMPTY;
		if ( game_get_winner (&game, &w) ) {
			if ( w == CELL_JAGUAR ) printf ("Onca venceu\n");
			if ( w == CELL_DOG ) printf ("CAES venceram\n");
			break;
		}
	}

	printf ("Encerrando teste.\n");
	return 0;
}
