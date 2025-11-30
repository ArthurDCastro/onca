#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "ai.h"
#include "game.h"

#define MAX_MOVES 128

/* defina estes chars no mesmo lugar que usa no resto do projeto */
#ifndef CTRL_JAGUAR_CHAR
#define CTRL_JAGUAR_CHAR 'o'
#endif

#ifndef CTRL_DOG_CHAR
#define CTRL_DOG_CHAR 'c'
#endif

/* escolha quem eh humano e quem eh agente */
#define HUMAN_SIDE CELL_DOG
#define AGENT_SIDE CELL_JAGUAR

/**
 * @brief Le uma linha do stdin e converte para Move (formato controlador).
 */
static int human_read_move (const Game* game, Move* mv) {
	char line[256];

	printf ("> ");
	if ( !fgets (line, sizeof line, stdin) )
		return -1; /* EOF */

	/* linha vazia = sair */
	if ( line[0] == '\n' || line[0] == '\0' )
		return 1;

	/* remove \n */
	line[strcspn (line, "\r\n")] = '\0';

	if ( game_move_from_controller (game, line, mv) != 0 ) {
		fprintf (stderr, "humano: jogada invalida no parse: '%s'\n", line);
		return -2;
	}

	if ( game_is_legal_move (game, mv) != 1 ) {
		fprintf (stderr, "humano: movimento ilegal\n");
		return -3;
	}

	return 0;
}

int main (void) {
	Game game;

	AiConfig ai;
	ai.max_depth = 7;
	int err;

	if ( (err = game_init (&game)) != 0 ) {
		fprintf (stderr, "player: game_init falhou (err=%d)\n", err);
		return 1;
	}

	/* mesmo tabuleiro inicial do controlador */
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

	/* lado que comeca: onca (como no controlador) */
	if ( (err = game_from_controller_board (&game, tabuleiro_inicial,
											CTRL_JAGUAR_CHAR)) != 0 ) {
		fprintf (stderr,
				 "player: game_from_controller_board falhou (err=%d)\n", err);
		return 1;
	}

	printf ("Teste local: voce joga com '%c' (human), agente com '%c'.\n",
			CTRL_DOG_CHAR, CTRL_JAGUAR_CHAR);
	printf ("Digite jogadas no formato do controlador (ex: \"c m 4 1 5 1\").\n");
	printf ("Linha vazia ou EOF encerra.\n\n");

	while ( 1 ) {
		CellContent winner;
		int r = game_get_winner (&game, &winner);

		if ( r == 1 ) {
			game_print_board (&game);
			if ( winner == CELL_JAGUAR )
				printf ("Fim de jogo: vitoria da onca.\n");
			else if ( winner == CELL_DOG )
				printf ("Fim de jogo: vitoria dos caes.\n");
			else
				printf ("Fim de jogo: vencedor desconhecido.\n");
			break;
		}

		game_print_board (&game);

		ai.side = game.to_move;

		if ( game.to_move == HUMAN_SIDE ) {
			printf ("Vez do %c...\n", CTRL_DOG_CHAR);
		} else if ( game.to_move == AGENT_SIDE ) {
			printf ("Vez do %c...\n", CTRL_JAGUAR_CHAR);
		} else {
			fprintf (stderr, "player: to_move desconhecido (%d)\n", game.to_move);
			break;
		}

		Move best;
		int ar = ai_choose_move (&game, &ai, &best);
		if ( ar != 0 ) {
			printf ("Agente nao encontrou movimentos. Fim.\n");
			break;
		}

		char buf[128];
		if ( game_move_to_controller (&game, &best, buf, (int)sizeof buf) != 0 ) {
			fprintf (stderr,
					 "player: game_move_to_controller falhou (agente)\n");
			break;
		}

		printf ("Agente joga: %s\n", buf);

		if ( game_apply_move (&game, &best) != 0 ) {
			fprintf (stderr, "player: game_apply_move falhou (agente)\n");
			break;
		}
	}

	return 0;
}
