#include <stdio.h>
#include <string.h>

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
 * @brief Escolhe um movimento para o agente.
 *
 * Por enquanto: pega o primeiro movimento legal gerado.
 * Depois trocamos por minimax/alpha-beta.
 */
static int agent_choose_move (const Game* game, Move* out_mv) {
	Move moves[MAX_MOVES];
	int count = 0;

	if ( game_generate_moves (game, moves, MAX_MOVES, &count) != 0 ) {
		fprintf (stderr, "agent_choose_move: game_generate_moves falhou\n");
		return -1;
	}

	if ( count <= 0 ) {
		return 1; /* sem movimentos */
	}

	*out_mv = moves[0];
	return 0;
}

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

		if ( game.to_move == HUMAN_SIDE ) {
			printf ("Sua vez (%c).\n", CTRL_DOG_CHAR);
			Move mv;
			int hr = human_read_move (&game, &mv);
			if ( hr == 1 ) { /* linha vazia */
				printf ("Encerrando por entrada vazia.\n");
				break;
			}
			if ( hr != 0 )
				continue; /* erro na jogada, pede de novo */

			if ( game_apply_move (&game, &mv) != 0 ) {
				fprintf (stderr, "player: game_apply_move falhou (humano)\n");
				break;
			}
		} else if ( game.to_move == AGENT_SIDE ) {
			printf ("Vez do agente (%c)...\n", CTRL_JAGUAR_CHAR);

			Move best;
			int ar = agent_choose_move (&game, &best);
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
		} else {
			fprintf (stderr, "player: to_move desconhecido (%d)\n", game.to_move);
			break;
		}
	}

	return 0;
}
