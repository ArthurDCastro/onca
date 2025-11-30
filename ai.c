#include "ai.h"

#include <stdio.h>

#define AI_MAX_MOVES 128

/* conta quantos movimentos o lado "side" tem a partir do estado atual */
static int ai_count_moves_for_side (const Game* game, CellContent side) {
	Game tmp = *game; /* copia local para nao mexer no original */
	tmp.to_move = side;

	Move moves[AI_MAX_MOVES];
	int count = 0;

	if ( game_generate_moves (&tmp, moves, AI_MAX_MOVES, &count) != 0 ) {
		fprintf (stderr,
				 "ai_count_moves_for_side: game_generate_moves falhou\n");
		return 0;
	}

	return count;
}

int ai_count_dogs_adjacent_to_jaguar (const Game* game) {
	int jpos = game->jaguar_pos;
	if ( jpos < 0 || jpos >= game->g.num_vertices )
		return 0;

	int neigh[GRAPH_MAX_NEIGHBORS];
	int deg = 0;

	if ( graph_get_neighbors (&game->g, jpos, neigh, &deg) != 0 )
		return 0;

	int count = 0;

	for ( int i = 0; i < deg; i++ ) {
		int vid = neigh[i];
		if ( game->cell_at[vid] == CELL_DOG )
			count++;
	}

	return count;
}

int ai_evaluate (const Game* game, CellContent side) {
	/* 1) material */
	int mat = 13 - game->num_dogs; /* maior = melhor pra onca */

	/* 2) mobilidade da onca */
	int jag_moves = ai_count_moves_for_side (game, CELL_JAGUAR); /* funcao helper */

	/* 3) mobilidade dos caes */
	int dog_moves = ai_count_moves_for_side (game, CELL_DOG); /* helper */

	/* 4) vizinhos da onca (grau) */
	int deg_jag = graph_degree (&game->g, game->jaguar_pos);

	/* 5) caes adjacentes à onca */
	int dogs_adj = ai_count_dogs_adjacent_to_jaguar (game);

	/* combinacao linear simples (ajusta pesos depois) */
	int score_onca =
		30 * mat /* peso maior no material */
		+ 3 * jag_moves - 2 * dog_moves + 2 * deg_jag - 5 * dogs_adj;

	/* se quiser, pode clamp: limitar score entre -20000 e 20000, etc */

	if ( side == CELL_JAGUAR )
		return score_onca;

	if ( side == CELL_DOG )
		return -score_onca; /* simetria simples */

	return 0;
}

/* avalia estado terminal ou retorna 0 se nao for terminal */
static int ai_eval_terminal (const Game* game, const AiConfig* cfg, int* out_score) {
	CellContent winner;

	if ( game_get_winner (game, &winner) != 1 )
		return 0; /* nao terminal */

	if ( winner == CELL_EMPTY ) {
		*out_score = 0; /* empate (se existir) */
		return 1;
	}

	if ( winner == cfg->side )
		*out_score = AI_WIN_SCORE;
	else
		*out_score = AI_LOSE_SCORE;

	return 1;
}

int ai_minimax (const Game* game, int depth, int maximizing, const AiConfig* cfg, int* out_score) {
	/* 1) testa estado terminal */
	int terminal_score;
	if ( ai_eval_terminal (game, cfg, &terminal_score) ) {
		*out_score = terminal_score;
		return 0;
	}

	/* 2) profundidade limite -> usa heuristica */
	if ( depth <= 0 ) {
		*out_score = ai_evaluate (game, cfg->side);
		return 0;
	}

	/* 3) gera movimentos */
	Move moves[AI_MAX_MOVES];
	int count = 0;

	if ( game_generate_moves (game, moves, AI_MAX_MOVES, &count) != 0 ) {
		fprintf (stderr, "ai_minimax: game_generate_moves falhou\n");
		return -2;
	}

	/* sem movimentos -> trata como terminal (empate ou derrota) */
	if ( count == 0 ) {
		if ( ai_eval_terminal (game, cfg, &terminal_score) ) {
			*out_score = terminal_score;
		} else {
			/* sem movimentos mas nao marcado como terminal:
			   consideramos neutro */
			*out_score = 0;
		}
		return 0;
	}

	/* 4) recursao minimax */
	int best_score;

	if ( maximizing ) {
		best_score = AI_LOSE_SCORE;

		for ( int i = 0; i < count; i++ ) {
			Game child = *game;

			if ( game_apply_move (&child, &moves[i]) != 0 ) {
				fprintf (stderr,
						 "ai_minimax: game_apply_move falhou (max branch)\n");
				continue;
			}

			int child_score;
			if ( ai_minimax (&child, depth - 1, 0, cfg, &child_score) != 0 )
				return -3;

			if ( child_score > best_score )
				best_score = child_score;
		}
	} else {
		best_score = AI_WIN_SCORE;

		for ( int i = 0; i < count; i++ ) {
			Game child = *game;

			if ( game_apply_move (&child, &moves[i]) != 0 ) {
				fprintf (stderr,
						 "ai_minimax: game_apply_move falhou (min branch)\n");
				continue;
			}

			int child_score;
			if ( ai_minimax (&child, depth - 1, 1, cfg, &child_score) != 0 )
				return -4;

			if ( child_score < best_score )
				best_score = child_score;
		}
	}

	*out_score = best_score;
	return 0;
}

int ai_alphabeta (const Game* game, int depth, int alpha, int beta, int maximizing, const AiConfig* cfg, int* out_score) {
	/* 1) testa estado terminal */
	int terminal_score;
	if ( ai_eval_terminal (game, cfg, &terminal_score) ) {
		*out_score = terminal_score;
		return 0;
	}

	/* 2) profundidade limite -> heuristica */
	if ( depth <= 0 ) {
		*out_score = ai_evaluate (game, cfg->side);
		return 0;
	}

	/* 3) gera movimentos */
	Move moves[AI_MAX_MOVES];
	int count = 0;

	if ( game_generate_moves (game, moves, AI_MAX_MOVES, &count) != 0 ) {
		fprintf (stderr, "ai_alphabeta: game_generate_moves falhou\n");
		return -2;
	}

	if ( count == 0 ) {
		/* sem movimentos: trata como terminal / neutro */
		if ( ai_eval_terminal (game, cfg, &terminal_score) ) {
			*out_score = terminal_score;
		} else {
			*out_score = 0;
		}
		return 0;
	}

	/* 4) recursao minimax com poda alfa-beta */
	int best_score;

	if ( maximizing ) {
		best_score = AI_LOSE_SCORE;

		for ( int i = 0; i < count; i++ ) {
			Game child = *game;

			if ( game_apply_move (&child, &moves[i]) != 0 ) {
				fprintf (stderr,
						 "ai_alphabeta: game_apply_move falhou (max branch)\n");
				continue;
			}

			int child_score;
			if ( ai_alphabeta (&child, depth - 1,
							   alpha, beta, 0, cfg, &child_score) != 0 )
				return -3;

			if ( child_score > best_score )
				best_score = child_score;

			if ( child_score > alpha )
				alpha = child_score;

			if ( alpha >= beta )
				break; /* poda */
		}
	} else {
		best_score = AI_WIN_SCORE;

		for ( int i = 0; i < count; i++ ) {
			Game child = *game;

			if ( game_apply_move (&child, &moves[i]) != 0 ) {
				fprintf (stderr,
						 "ai_alphabeta: game_apply_move falhou (min branch)\n");
				continue;
			}

			int child_score;
			if ( ai_alphabeta (&child, depth - 1,
							   alpha, beta, 1, cfg, &child_score) != 0 )
				return -4;

			if ( child_score < best_score )
				best_score = child_score;

			if ( child_score < beta )
				beta = child_score;

			if ( alpha >= beta )
				break; /* poda */
		}
	}

	*out_score = best_score;
	return 0;
}

int ai_choose_move (const Game* game, const AiConfig* cfg, Move* best_move) {
	Move moves[AI_MAX_MOVES];
	int count = 0;

	if ( game_generate_moves (game, moves, AI_MAX_MOVES, &count) != 0 ) {
		fprintf (stderr, "ai_choose_move: game_generate_moves falhou\n");
		return -2;
	}

	if ( count <= 0 ) {
		/* sem movimentos possiveis */
		return 1;
	}

	/* quem joga agora e maximizador se for o lado da IA */
	int maximizing_root = (game->to_move == cfg->side) ? 1 : 0;

	int best_score = maximizing_root ? AI_LOSE_SCORE : AI_WIN_SCORE;
	int best_idx = 0;

	for ( int i = 0; i < count; i++ ) {
		Game child = *game;

		if ( game_apply_move (&child, &moves[i]) != 0 ) {
			fprintf (stderr,
					 "ai_choose_move: game_apply_move falhou no movimento %d\n",
					 i);
			continue;
		}

		int score = 0;
		int alpha = AI_LOSE_SCORE;
		int beta = AI_WIN_SCORE;

		/* proximo nivel troca quem maximiza/minimiza */
		int maximizing_next = maximizing_root ? 0 : 1;

		if ( ai_alphabeta (&child, cfg->max_depth - 1, alpha, beta, maximizing_next, cfg, &score) != 0 ) {
			fprintf (stderr,
					 "ai_choose_move: ai_alphabeta falhou no movimento %d\n",
					 i);
			continue;
		}

		if ( maximizing_root ) {
			if ( score > best_score ) {
				best_score = score;
				best_idx = i;
			}
		} else {
			if ( score < best_score ) {
				best_score = score;
				best_idx = i;
			}
		}
	}

	*best_move = moves[best_idx];
	return 0;
}
