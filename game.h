#ifndef GAME_H
#define GAME_H

#include "graph.h"

#define CTRL_JAGUAR_CHAR 'o' /* caractere usado pelo controlador para onca                 */
#define CTRL_DOG_CHAR 'c'	 /* caractere usado pelo controlador para cao                  */
#define CTRL_EMPTY_CHAR '-'	 /* casa vazia                                                 */
#define CTRL_MOV_SIMP 'm'	 /* caractere usado pelo controlador para movimento simples    */
#define CTRL_MOV_SALT 's'	 /* caractere usado pelo controlador para movimento salto      */

#define GAME_MAP_FILE "map.txt" /* caminho do arquivo de mapa usado para construir o grafo interno */

#define BOARD_TAM 73

#define MAP_ROW 7
#define MAP_COL 5

/**
 * @brief Conteudo de uma casa do tabuleiro.
 *
 * CELL_EMPTY  -> casa vazia
 * CELL_DOG    -> casa com um cao
 * CELL_JAGUAR -> casa com a onca
 */
typedef enum {
	CELL_EMPTY = 0,
	CELL_DOG,
	CELL_JAGUAR
} CellContent;

/**
 * @brief Tipo de movimento do jogo.
 *
 * MOVE_SIMPLE:
 *    Movimento simples (tipo 'm' no controlador)
 *    path_len = 2 (origem -> destino)
 *
 * MOVE_JUMP:
 *    Salto simples ou multiplo da onca (tipo 's')
 *    path_len >= 2
 */
typedef enum {
	MOVE_ERR = 0,
	MOVE_SIMPLE,
	MOVE_JUMP
} MoveType;

/**
 * @brief Representa um movimento no grafo.
 *
 * path[0] = vertice origem
 * path[1..] = vertices destino
 */
typedef struct {
	MoveType type;	  /**< MOVE_SIMPLE ou MOVE_JUMP        */
	CellContent side; /**< Lado que joga (cao ou onca)      */

	int path[GRAPH_MAX_VERTICES]; /**< Sequencia de vertices             */
	int path_len;				  /**< Numero de vertices utilizados     */
} Move;

/**
 * @brief Estrutura que representa o estado do jogo.
 *
 * Contem:
 *   - grafo interno g (topologia fixa do tabuleiro)
 *   - ocupacao de cada vertice (cell_at[])
 *   - posicao da onca
 *   - numero de caes
 *   - lado que ira jogar
 */
typedef struct {
	Graph g; /**< Grafo interno (topologia)       */

	CellContent cell_at[GRAPH_MAX_VERTICES]; /**< Ocupacao de cada vertice        */
	int jaguar_pos;							 /**< ID do vertice onde esta a onca  */
	int num_dogs;							 /**< Numero de caes no tabuleiro     */

	CellContent to_move; /**< Lado que joga agora             */
} Game;

/**
 * @brief Inicializa o estado do jogo e cria o grafo interno.
 *
 * Esta funcao:
 *   - construtor do grafo interno g (graph_create)
 *   - limpa todas as casas (cell_at[])
 *   - define jaguar_pos = -1
 *   - define num_dogs = 0
 *   - define to_move = CELL_JAGUAR por padrao
 *
 * @param game Ponteiro para estrutura Game.
 * @return 0 em sucesso, <0 em erro.
 */
int game_init (Game* game);

/**
 * @brief Limpa todas as casas e reseta estado.
 *
 * @param game Ponteiro para Game.
 * @return 0 em sucesso, <0 em erro.
 */
int game_clear (Game* game);

/**
 * @brief Atualiza o estado do jogo a partir do tabuleiro enviado pelo controlador.
 *
 * Interpreta os caracteres do controlador usando as constantes:
 *   CTRL_JAGUAR_CHAR -> CELL_JAGUAR
 *   CTRL_DOG_CHAR    -> CELL_DOG
 *   CTRL_EMPTY_CHAR  -> CELL_EMPTY
 *
 * Campos atualizados:
 *   - cell_at[]      (ocupacao de cada vertice)
 *   - jaguar_pos     (vertice onde esta a onca)
 *   - num_dogs       (quantidade de caes)
 *   - to_move        (lado a jogar, baseado no parametro lado)
 *
 * @param game   Ponteiro para Game.
 * @param board  String bruta do tabuleiro, exatamente como recebida do controlador.
 * @param lado   Caractere indicando o lado que joga agora (CTRL_JAGUAR_CHAR ou CTRL_DOG_CHAR).
 *#define CTRL_COLS 8
 * @return 0 em sucesso, <0 em erro.
 */
int game_from_controller_board (Game* game, const char* board, char lado);

/**
 * @brief Verifica se um movimento eh legal segundo as regras.
 *
 * @param game Estado atual.
 * @param mv Movimento a verificar.
 * @return 1 se legal, 0 se ilegal, <0 em erro.
 */
int game_is_legal_move (const Game* game, const Move* mv);

/**
 * @brief Aplica um movimento ao estado do jogo.
 *
 * Assume que ja foi verificado como legal.
 *
 * @param game Estado atual.
 * @param mv Movimento.
 * @return 0 em sucesso, <0 em erro.
 */
int game_apply_move (Game* game, const Move* mv);

/**
 * @brief Gera todos os movimentos legais para o lado atual.
 *
 * @param game Estado atual.
 * @param moves Vetor de saida.
 * @param max_moves Tamanho maximo do vetor moves.
 * @param out_count Numero de movimentos gerados.
 * @return 0 em sucesso, <0 em erro.
 */
int game_generate_moves (const Game* game, Move moves[], int max_moves, int* out_count);

/**
 * @brief Verifica se ha vencedor.
 *
 * @param game Estado atual.
 * @param winner Ponteiro para receber o vencedor:
 *        CELL_JAGUAR, CELL_DOG ou CELL_EMPTY
 * @return 1 se ha vencedor, 0 se nao ha, <0 em erro.
 */
int game_get_winner (const Game* game, CellContent* winner);

/**
 * @brief Converte um movimento interno para a string usada pelo controlador.
 *
 * Gera strings como:
 *   "o m l0 c0 l1 c1"
 *   "o s n l0 c0 l1 c1 ... ln cn"
 *
 * @param game   Estado atual do jogo.
 * @param mv     Movimento interno.
 * @param buf    Buffer de saida.
 * @param bufsize Tamanho do buffer.
 *
 * @return 0 em sucesso, <0 em erro.
 */
int game_move_to_controller (const Game* game, const Move* mv, char* buf, int bufsize);

/**
 * @brief Converte jogada do controlador em Move interno.
 *
 * Formatos:
 *   "<lado> m l0 c0 l1 c1"
 *   "<lado> s n l0 c0 l1 c1 ... ln cn"
 *
 * @param game   Estado atual do jogo.
 * @param jogada String bruta recebida do controlador.
 * @param mv     Estrutura Move a ser preenchida.
 *
 * @return 0 em sucesso, <0 em erro de parse.
 */
int game_move_from_controller (const Game* game, const char* jogada, Move* mv);

/**
 * @brief Imprime o estado do jogo para debug.
 */
void game_print (const Game* game);

void game_print_board (const Game* game);

/**
 * @brief Retorna o vertice intermediario entre dois vertices usados em salto.
 *
 * @param g        Ponteiro para o grafo.
 * @param from_vid Indice do vertice de origem.
 * @param to_vid   Indice do vertice de destino.
 *
 * @return Indice do vertice intermediario,
 *         ou -1 se nao existir ou se nao for um salto valido.
 */
int graph_get_mid_jump (const Graph* g, int from_vid, int to_vid);

#endif /* GAME_H */
