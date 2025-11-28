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

int game_is_legal_move (const Game* g, const Move* mv) {
	if ( !g || !mv ) {
		fprintf (stderr,
				 "game_is_legal_move: ponteiro nulo (g=%p, mv=%p)\n",
				 (void*)g, (void*)mv);
		return -1;
	}

	if ( mv->path_len < 2 ) {
		/* caminho vazio ou grande demais */
		return 0;
	}

	/* confere indices e origem */
	for ( int k = 0; k < mv->path_len; k++ ) {
		int vid = mv->path[k];
		if ( vid < 0 || vid >= g->g.num_vertices )
			return 0;
	}

	int from = mv->path[0];
	int to = mv->path[1];
	if ( g->cell_at[from] != mv->side ) {
		/* origem nao contem peca do lado que joga */
		return 0;
	}

	if ( from == to )
		return 0;

	/* --- movimentos dos caes --- */
	if ( mv->side == CELL_DOG ) {
		if ( mv->type != MOVE_SIMPLE )
			return 0;

		if ( !graph_is_neighbor (&g->g, from, to) )
			return 0;

		if ( g->cell_at[to] != CELL_EMPTY )
			return 0;

		return 1;
	}

	/* a partir daqui, estamos tratando a onca */

	if ( mv->side != CELL_JAGUAR )
		return 0;

	/* movimento simples da onca */
	if ( mv->type == MOVE_SIMPLE ) {
		if ( !graph_is_neighbor (&g->g, from, to) )
			return 0;

		if ( g->cell_at[to] != CELL_EMPTY )
			return 0;

		return 1;
	}

	/* salto(s) da onca */
	if ( mv->type == MOVE_JUMP ) {
		/* posicao inicial deve bater com onde a onca esta */
		if ( from != g->jaguar_pos )
			return 0;

		int current = from;

		for ( int k = 0; k < mv->path_len - 1; k++ ) {
			int to = mv->path[k + 1];
			if ( current == to )
				return 0;

			int mid = graph_get_mid_jump (&g->g, current, to);
			if ( mid < 0 )
				return 0;

			if ( !graph_is_neighbor (&g->g, from, mid) && !graph_is_neighbor (&g->g, mid, to) )
				return 0;

			/* casa intermediaria deve ter cao, destino deve estar vazio */
			if ( g->cell_at[mid] != CELL_DOG )
				return 0;
			if ( g->cell_at[to] != CELL_EMPTY )
				return 0;

			/* opcional: garantir que from/to sao "saltaveis":
			   voce pode adicionar checks extras de geometria aqui se quiser */

			current = to;
		}

		return 1;
	}

	/* tipo desconhecido */
	return 0;
}

int graph_get_mid_jump (const Graph* g, int from_vid, int to_vid) {
	int lf = g->v[from_vid].c.row;
	int cf = g->v[from_vid].c.col;

	int lt = g->v[to_vid].c.row;
	int ct = g->v[to_vid].c.col;

	int lm = (lf + lt) / 2;
	int cm = (cf + ct) / 2;

	return graph_get_index (g, lm, cm);
}

static CellContent opposite_side (CellContent s) {
	if ( s == CELL_JAGUAR ) return CELL_DOG;
	if ( s == CELL_DOG ) return CELL_JAGUAR;
	return s;
}

int game_apply_move (Game* game, const Move* mv) {
	if ( !game || !mv ) {
		fprintf (stderr,
				 "game_apply_move: ponteiro nulo (game=%p, mv=%p)\n",
				 (void*)game, (void*)mv);
		return -1;
	}

	CellContent side = mv->side;

	/* -------- movimento simples (um passo) -------- */
	if ( mv->type == MOVE_SIMPLE ) {
		int from = mv->path[0];
		int to = mv->path[1];

		/* aqui assumimos que indices sao validos e o movimento eh legal */

		game->cell_at[from] = CELL_EMPTY;
		game->cell_at[to] = side;

		if ( side == CELL_JAGUAR )
			game->jaguar_pos = to;

		game->to_move = opposite_side (game->to_move);
		return 0;
	}

	/* -------- salto(s) da onca -------- */
	if ( mv->type == MOVE_JUMP ) {
		/* assumimos que so a onca usa MOVE_JUMP e que path eh valido */
		int current = mv->path[0];

		for ( int k = 0; k < mv->path_len - 1; k++ ) {
			int from = current;
			int to = mv->path[k + 1];

			int mid = graph_get_mid_jump (&game->g, from, to);

			/* aplica o salto:
			   - remove onca da origem
			   - remove cao do meio
			   - coloca onca no destino */
			game->cell_at[from] = CELL_EMPTY;
			game->cell_at[mid] = CELL_EMPTY;
			game->cell_at[to] = CELL_JAGUAR;

			game->num_dogs--;
			current = to;
		}

		game->jaguar_pos = current;
		game->to_move = opposite_side (game->to_move);
		return 0;
	}

	fprintf (stderr,
			 "game_apply_move: tipo de movimento desconhecido (%d)\n",
			 mv->type);
	return -5;
}

/* verifica se a onca tem algum movimento legal a partir do estado atual */
static int game_jaguar_has_legal_move (const Game* g) {
	if ( !g )
		return 0;

	int jpos = g->jaguar_pos;

	if ( jpos < 0 || jpos >= g->g.num_vertices )
		return 0;
	if ( g->cell_at[jpos] != CELL_JAGUAR )
		return 0;

	int neigh[GRAPH_MAX_NEIGHBORS];
	int deg = 0;
	if ( graph_get_neighbors (&g->g, jpos, neigh, &deg) < 0 )
		return 0;

	Move mv;

	mv.side = CELL_JAGUAR;
	mv.path[0] = jpos;
	mv.type = MOVE_SIMPLE;
	mv.path_len = 2; /* simples ou 1 salto: origem + destino */

	/* --- movimentos simples da onca --- */
	for ( int i = 0; i < deg; i++ ) {
		mv.path[1] = neigh[i];
		if ( game_is_legal_move (g, &mv) == 1 )
			return 1;
	}

	/* --- saltos: onca em jpos, cao em mid, destino vazio em dest --- */

	int neigh_mid[GRAPH_MAX_NEIGHBORS];
	int deg_mid = 0;
	mv.type = MOVE_JUMP;

	for ( int i = 0; i < deg; i++ ) {
		int mid = neigh[i];

		if ( g->cell_at[mid] != CELL_DOG )
			continue;

		if ( graph_get_neighbors (&g->g, mid, neigh_mid, &deg_mid) < 0 )
			continue;

		for ( int j = 0; j < deg_mid; j++ ) {
			mv.path[1] = neigh_mid[j];

			if ( game_is_legal_move (g, &mv) == 1 )
				return 1;
		}
	}

	return 0;
}

int game_get_winner (const Game* g, CellContent* winner) {
	if ( !g || !winner ) {
		fprintf (stderr,
				 "game_get_winner: ponteiro nulo (g=%p, winner=%p)\n",
				 (void*)g, (void*)winner);
		return -1;
	}

	*winner = CELL_EMPTY;

	/* regra da onca: ganha se restarem 9 ou menos caes */
	if ( g->num_dogs <= 9 ) {
		*winner = CELL_JAGUAR;
		return 1;
	}

	/* se a onca nao tiver movimentos legais, caes vencem */
	if ( !game_jaguar_has_legal_move (g) ) {
		*winner = CELL_DOG;
		return 1;
	}

	return 0; /* ninguem venceu ainda */
}

int game_generate_moves (const Game* game, Move moves[], int max_moves, int* out_count) {
	if ( !game || !moves || !out_count ) {
		fprintf (stderr,
				 "game_generate_moves: ponteiro nulo (game=%p, moves=%p, out_count=%p)\n",
				 (void*)game, (void*)moves, (void*)out_count);
		return -1;
	}

	if ( max_moves <= 0 ) {
		fprintf (stderr, "game_generate_moves: max_moves <= 0 (%d)\n", max_moves);
		return -2;
	}

	*out_count = 0;

	CellContent side = game->to_move;

	/* ---------------- CÃES: apenas movimentos simples ---------------- */
	if ( side == CELL_DOG ) {
		for ( int vid = 0; vid < game->g.num_vertices; vid++ ) {
			if ( game->cell_at[vid] != CELL_DOG )
				continue;

			int neigh[GRAPH_MAX_NEIGHBORS];
			int deg = 0;

			if ( graph_get_neighbors (&game->g, vid, neigh, &deg) != 0 )
				continue;

			for ( int i = 0; i < deg; i++ ) {
				if ( *out_count >= max_moves )
					return 0; /* truncado, mas sem erro */

				Move mv;
				mv.side = CELL_DOG;
				mv.type = MOVE_SIMPLE;
				mv.path_len = 2;
				mv.path[0] = vid;
				mv.path[1] = neigh[i];

				if ( game_is_legal_move (game, &mv) == 1 )
					moves[(*out_count)++] = mv;
			}
		}
		return 0;
	}

	/* ---------------- ONÇA: movimentos simples + saltos ---------------- */

	if ( side != CELL_JAGUAR ) {
		/* lado desconhecido: nenhum movimento */
		return 0;
	}

	int jpos = game->jaguar_pos;
	if ( jpos < 0 || jpos >= game->g.num_vertices )
		return 0;
	if ( game->cell_at[jpos] != CELL_JAGUAR )
		return 0;

	int neigh[GRAPH_MAX_NEIGHBORS];
	int deg = 0;

	if ( graph_get_neighbors (&game->g, jpos, neigh, &deg) != 0 )
		return 0;

	/* --- movimentos simples da onça --- */
	for ( int i = 0; i < deg; i++ ) {
		if ( *out_count >= max_moves )
			return 0;

		Move mv;
		mv.side = CELL_JAGUAR;
		mv.type = MOVE_SIMPLE;
		mv.path_len = 2;
		mv.path[0] = jpos;
		mv.path[1] = neigh[i];

		if ( game_is_legal_move (game, &mv) == 1 )
			moves[(*out_count)++] = mv;
	}

	/* --- saltos (apenas um salto por movimento, por enquanto) --- */

	int neigh_mid[GRAPH_MAX_NEIGHBORS];
	int deg_mid = 0;

	for ( int i = 0; i < deg; i++ ) {
		int mid = neigh[i];

		if ( game->cell_at[mid] != CELL_DOG )
			continue;

		if ( graph_get_neighbors (&game->g, mid, neigh_mid, &deg_mid) != 0 )
			continue;

		for ( int j = 0; j < deg_mid; j++ ) {
			if ( *out_count >= max_moves )
				return 0;

			Move mv;
			mv.side = CELL_JAGUAR;
			mv.type = MOVE_JUMP;
			mv.path_len = 2;
			mv.path[0] = jpos;
			mv.path[1] = neigh_mid[j];

			if ( game_is_legal_move (game, &mv) == 1 )
				moves[(*out_count)++] = mv;
		}
	}

	return 0;
}
