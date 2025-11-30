#ifndef AI_H
#define AI_H

#include "game.h"

#define AI_MAX_MOVES 128
#define AI_WIN_SCORE 10000
#define AI_LOSE_SCORE -10000

/**
 * @brief Configuracao da IA.
 */
typedef struct {
	int max_depth;	  /* profundidade maxima da busca */
	CellContent side; /* lado para o qual avaliamos */
} AiConfig;

/**
 * @brief Funcao de estimativa de recompensa (avaliacao heuristica).
 *
 * @param game Estado atual do jogo.
 * @param side Lado para o qual o valor e calculado.
 * @return Valor inteiro da posicao: maior e melhor para "side".
 */
int ai_evaluate (const Game* game, CellContent side);

/**
 * @brief Algoritmo MINIMAX simples (sem poda).
 *
 * @param game Estado atual (nao modificado).
 * @param depth Profundidade restante.
 * @param maximizing 1 se o jogador atual e maximizador, 0 se minimizador.
 * @param cfg Configuracao da IA (lado, profundidade etc).
 * @param out_score Saida com o valor da posicao.
 * @return 0 em sucesso, <0 em erro.
 */
int ai_minimax (const Game* game, int depth, int maximizing, const AiConfig* cfg, int* out_score);

/**
 * @brief Busca adversaria com algoritmo MINIMAX e poda alfa-beta.
 *
 * @param game Estado atual (nao modificado).
 * @param depth Profundidade restante.
 * @param alpha Limite inferior (melhor valor garantido para max).
 * @param beta Limite superior (melhor valor garantido para min).
 * @param maximizing 1 se o jogador atual e maximizador, 0 se minimizador.
 * @param cfg Configuracao da IA.
 * @param out_score Saida com o valor da posicao.
 * @return 0 em sucesso, <0 em erro.
 */
int ai_alphabeta (const Game* game, int depth, int alpha, int beta, int maximizing, const AiConfig* cfg, int* out_score);

/**
 * @brief Escolhe a melhor acao para o lado em game->to_move.
 *
 * Internamente pode usar ai_alphabeta (ou ai_minimax) para decidir.
 *
 * @param game Estado atual (nao modificado).
 * @param cfg Configuracao da IA.
 * @param best_move Saida com o melhor movimento encontrado.
 * @return 0 em sucesso, >0 se nao ha movimentos, <0 em erro.
 */
int ai_choose_move (const Game* game, const AiConfig* cfg, Move* best_move);

#endif /* AI_H */
