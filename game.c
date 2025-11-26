#include "game.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int game_init (Game* game) {
	if ( !game ) {
		fprintf (stderr, "game_init: ponteiro game == NULL\n");
		return -1;
	}

	/* cria o grafo interno a partir do mapa ASCII */
	int err = graph_create (&game->g, GAME_MAP_FILE);
	if ( err != 0 ) {
		fprintf (stderr, "game_init: graph_create falhou (err=%d)\n", err);
		return -2;
	}

	/* zera estado das pecas e contadores */
	err = game_clear (game);
	if ( err != 0 ) {
		fprintf (stderr, "game_init: game_clear falhou (err=%d)\n", err);
		return -3;
	}

	/* lado padrao: onca comeca */
	game->to_move = CELL_JAGUAR;

	return 0;
}

int game_clear (Game* game) {
	if ( !game ) {
		fprintf (stderr, "game_clear: ponteiro game == NULL\n");
		return -1;
	}

	/* limpa ocupacao de todas as casas */
	for ( int i = 0; i < GRAPH_MAX_VERTICES; i++ )
		game->cell_at[i] = CELL_EMPTY;

	game->jaguar_pos = -1;
	game->num_dogs = 0;

	return 0;
}

#define CTRL_POS(l, c) ((l) * (MAP_COL + 2) + (c))

/* mesma ideia do controlador.c */
static int ctrl_pos_valida (int l, int c) {
	if ( l < 1 || l > MAP_ROW || c < 1 || c > MAP_COL )
		return 0;
	if ( l == 6 && (c == 1 || c == 5) )
		return 0;
	if ( l == 7 && (c == 2 || c == 4) )
		return 0;
	return 1;
}

static CellContent ctrl_char_to_cell (char ch) {
	if ( ch == CTRL_JAGUAR_CHAR ) return CELL_JAGUAR;
	if ( ch == CTRL_DOG_CHAR ) return CELL_DOG;
	if ( ch == CTRL_EMPTY_CHAR ) return CELL_EMPTY;
	return CELL_EMPTY; /* qualquer coisa fora disso trata como vazio */
}

static char cell_to_ctrl_char (CellContent cell) {
	if ( cell == CELL_JAGUAR ) return CTRL_JAGUAR_CHAR;
	if ( cell == CELL_DOG ) return CTRL_DOG_CHAR;
	return '-'; /* qualquer coisa fora disso trata como vazio */
}

static MoveType ctrl_char_to_movtype (char ch) {
	if ( ch == CTRL_MOV_SALT ) return MOVE_JUMP;
	if ( ch == CTRL_MOV_SIMP ) return MOVE_SIMPLE;
	return MOVE_ERR; /* qualquer coisa fora disso trata como vazio */
}

static char movtype_to_ctrl_char (MoveType mt) {
	if ( mt == MOVE_JUMP ) return CTRL_MOV_SALT;
	if ( mt == MOVE_SIMPLE ) return CTRL_MOV_SIMP;
	return NULL; /* qualquer coisa fora disso trata como vazio */
}

int game_from_controller_board (Game* game, const char* board, char lado) {
	if ( !game || !board ) {
		fprintf (stderr,
				 "game_from_controller_board: ponteiro nulo (game=%p, board=%p)\n",
				 (void*)game, (void*)board);
		return -1;
	}

	if ( game_clear (game) != 0 ) {
		fprintf (stderr, "game_from_controller_board: game_clear falhou\n");
		return -2;
	}

	game->to_move = ctrl_char_to_cell (lado);
	if ( game->to_move == CELL_EMPTY ) {
		fprintf (stderr,
				 "game_from_controller_board: lado invalido '%c'\n", lado);
		return -3;
	}

	char ch = '#';
	int l = 0, c = 0;
	for ( int i = 0; board[i] != '\0'; i++ ) {
		ch = board[i];
		if ( ch == '#' )
			c = 0;
		else if ( ch == '\n' )
			l++;
		else
			c++;

		if ( !ctrl_pos_valida (l, c) )
			continue;

		int vid = graph_get_index (&game->g, l, c);
		if ( vid < 0 ) {
			fprintf (stderr,
					 "game_from_controller_board: sem vertice para (l=%d,c=%d)\n",
					 l, c);
			return -4;
		}

		CellContent cell = ctrl_char_to_cell (ch);
		game->cell_at[vid] = cell;

		if ( cell == CELL_JAGUAR )
			game->jaguar_pos = vid;
		else if ( cell == CELL_DOG )
			game->num_dogs++;
	}

	return 0;
}

int game_move_from_controller (const Game* game, const char* jogada, Move* mv) {
	if ( !game || !jogada || !mv ) {
		fprintf (stderr,
				 "game_move_from_controller: ponteiro nulo (game=%p, jogada=%p, mv=%p)\n",
				 (void*)game, (void*)jogada, (void*)mv);
		return -1;
	}

	char lado_ch;
	char tipo_ch;
	int n, pos = 0;

	/* lado */
	if ( sscanf (&jogada[pos], "%c %c%n", &lado_ch, &tipo_ch, &n) != 2 ) {
		fprintf (stderr,
				 "game_move_from_controller: erro lendo lado/tipo em '%s'\n",
				 jogada);
		return -2;
	}
	pos += n;

	mv->side = ctrl_char_to_cell (lado_ch);
	if ( mv->side == CELL_EMPTY ) {
		fprintf (stderr,
				 "game_move_from_controller: lado invalido '%c'\n", lado_ch);
		return -4;
	}

	mv->type = ctrl_char_to_movtype (tipo_ch);

	if ( mv->type == MOVE_ERR ) {
		fprintf (stderr,
				 "game_move_from_controller: tipo invalido '%c'\n", tipo_ch);
		return -6;
	}

	mv->path_len = 2;

	if ( mv->type == MOVE_JUMP ) {
		int ns;
		printf ("teste");

		if ( sscanf (&jogada[pos], "%d%n", &ns, &n) != 1 ) {
			fprintf (stderr,
					 "game_move_from_controller: erro lendo n (numero de saltos)\n");
			return -11;
		}
		pos += n;
		mv->path_len = ns + 1;
	}

	if ( mv->path_len > GRAPH_MAX_VERTICES ) {
		fprintf (stderr,
				 "game_move_from_controller: path estourou limite\n");
		return -10;
	}

	for ( int k = 0; k < mv->path_len; k++ ) {
		int l, c;

		if ( sscanf (&jogada[pos], "%d %d%n", &l, &c, &n) != 2 ) {
			fprintf (stderr,
					 "game_move_from_controller: erro lendo coordenada (k=%d, l=%d, c=%d) jogada=%s pos=%d\n", k, l, c, jogada, pos - n);
			return -7;
		}
		pos += n;

		int vid = graph_get_index (&game->g, l, c);
		if ( vid < 0 ) {
			fprintf (stderr,
					 "game_move_from_controller: sem vertice para (l=%d,c=%d)\n",
					 l, c);
			return -9;
		}

		mv->path[k] = vid;
	}

	return 0;
}

int game_move_to_controller (const Game* game, const Move* mv, char* buf, int bufsize) {
	if ( !game || !mv || !buf || bufsize <= 0 ) {
		fprintf (stderr,
				 "game_move_to_controller: ponteiro nulo ou bufsize invalido\n");
		return -1;
	}

	if ( mv->path_len <= 0 || mv->path_len > GRAPH_MAX_VERTICES ) {
		fprintf (stderr,
				 "game_move_to_controller: path_len invalido (%d)\n",
				 mv->path_len);
		return -2;
	}

	char lado_ch = cell_to_ctrl_char (mv->side);
	if ( lado_ch == '-' ) {
		fprintf (stderr,
				 "game_move_to_controller: lado invalido no Move\n");
		return -3;
	}

	char tipo_ch = movtype_to_ctrl_char (mv->type);
	if ( !tipo_ch ) {
		fprintf (stderr,
				 "game_move_to_controller: tipo de movimento invalido\n");
		return -4;
	}

	int used = 0;
	int written;

	/* lado e tipo: "<lado> <tipo>" */
	written = snprintf (&buf[used], bufsize - used, "%c %c", lado_ch, tipo_ch);
	if ( written < 0 || written >= bufsize - used ) {
		fprintf (stderr,
				 "game_move_to_controller: buffer insuficiente (cabecalho)\n");
		return -5;
	}
	used += written;

	/* numero de saltos se for 's' */
	if ( mv->type == MOVE_JUMP ) {
		int ns = mv->path_len - 1; /* n saltos => n+1 posicoes */
		written = snprintf (&buf[used], bufsize - used, " %d", ns);
		if ( written < 0 || written >= bufsize - used ) {
			fprintf (stderr,
					 "game_move_to_controller: buffer insuficiente (n)\n");
			return -6;
		}
		used += written;
	}

	/* coordenadas: l c para cada vertice do caminho */
	for ( int k = 0; k < mv->path_len; k++ ) {
		int vid = mv->path[k];

		if ( vid < 0 || vid >= game->g.num_vertices ) {
			fprintf (stderr,
					 "game_move_to_controller: vid invalido (%d)\n", vid);
			return -7;
		}

		int l = game->g.v[vid].c.row;
		int c = game->g.v[vid].c.col;

		written = snprintf (&buf[used], bufsize - used,
							" %d %d", l, c);
		if ( written < 0 || written >= bufsize - used ) {
			fprintf (stderr,
					 "game_move_to_controller: buffer insuficiente (coords)\n");
			return -8;
		}
		used += written;
	}

	return 0;
}

void game_print_board (const Game* game) {
	if ( !game ) {
		fprintf (stderr, "game_print_board: game nulo\n");
		return;
	}

	/* Tamanho igual ao controlador */
	const int ROWS = 9;
	const int COLS = 7;

	for ( int l = 0; l < ROWS; l++ ) {
		for ( int c = 0; c < COLS; c++ ) {
			/* bordas do controlador */
			if ( l == 0 || l == ROWS - 1 || c == 0 || c == COLS - 1 ) {
				putchar ('#');
				continue;
			}

			int gl = l; /* grafico usa 1..7 */
			int gc = c; /* grafico usa 1..5 */

			/* Checa se esta posição é válida no jogo */
			if ( !ctrl_pos_valida (gl, gc) ) {
				putchar (' '); /* borda interna do mapa */
				continue;
			}

			/* Obtem id do vertice */
			int vid = graph_get_index (&game->g, gl, gc);
			if ( vid < 0 ) {
				putchar ('#'); /* erro no grafo → usa # */
				continue;
			}

			/* Conteúdo da célula */
			CellContent cc = game->cell_at[vid];

			char ch = cell_to_ctrl_char (cc);
			putchar (ch);
		}
		putchar ('\n');
	}
}

int graph_is_valid_coord (const Graph* g, int row, int col) {
	/* Mesmo criterio usado no controlador */
	if ( row < 1 || row > 7 || col < 1 || col > 5 )
		return 0;
	if ( row == 6 && (col == 1 || col == 5) )
		return 0;
	if ( row == 7 && (col == 2 || col == 4) )
		return 0;

	return 1;
}
