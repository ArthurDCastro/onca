#ifndef BOARD_H
#define BOARD_H

#define BOARD_MAX_VERTICES 32  ///< Numero maximo de vertices do tabuleiro

/**
 * @brief Representa o tipo de peca em uma posicao do tabuleiro.
 */
typedef enum {
	PIECE_EMPTY = 0,  ///< Casa vazia
	PIECE_DOG,		  ///< Cao (lado 'c')
	PIECE_JAGUAR	  ///< Onca (lado 'o')
} Piece;

/**
 * @brief Estado de um vertice (posicao valida) do tabuleiro.
 */
typedef struct {
	Piece piece;  ///< Peca presente neste vertice
	Vertex neighbors[BOARD_MAX_VERTICES];
} Vertex;

/**
 * @brief Representa o estado completo do jogo.
 */
typedef struct {
	Vertex v[BOARD_MAX_VERTICES];  ///< Vetor de estados dos vertices
	int num_vertices;			   ///< Quantidade real de vertices usados

	int jaguar_pos;	 ///< Indice da posicao da onca
	int num_dogs;	 ///< Numero de caes no tabuleiro

	char to_move;  ///< Lado a jogar: 'o' ou 'c'
} Board;

/**
 * @brief Representa um movimento interno da IA.
 *
 * Para movimentos simples:
 *   - is_jump = 0
 *   - count = 2 (origem -> destino)
 *
 * Para saltos:
 *   - is_jump = 1
 *   - count >= 2 (origem -> salto1 -> salto2 -> ...)
 *
 * Os elementos em path[] sao indices de vertices no grafo.
 */
typedef struct {
	int is_jump;				   ///< 0 = simples, 1 = salto
	int count;					   ///< Tamanho da cadeia (>=2)
	int path[BOARD_MAX_VERTICES];  ///< Sequencia de vertices
} Move;

/**
 * @brief Inicializa um board vazio.
 *
 * @param b Board a ser inicializado.
 * @param num_vertices Numero de vertices usados.
 * @return 0 em caso de sucesso, diferente de 0 em caso de erro.
 */
int board_init (Board* b, int num_vertices);

/**
 * @brief Preenche o estado a partir da string do controlador.
 *
 * @param b Board destino.
 * @param tab_str String do tabuleiro enviada pelo controlador.
 * @param to_move Lado a jogar ('o' ou 'c').
 * @return 0 em caso de sucesso, diferente de 0 em caso de erro.
 */
int board_from_controller (Board* b, const char* tab_str, char to_move);

/**
 * @brief Copia o estado de src para dst.
 *
 * @return 0 em caso de sucesso, diferente de 0 em caso de erro.
 */
int board_copy (Board* dst, const Board* src);

/**
 * @brief Aplica um movimento interno da IA ao board.
 *
 * @param b Board a ser modificado.
 * @param m Movimento interno.
 * @return 0 em caso de sucesso, diferente de 0 em caso de erro.
 */
int board_apply_move (Board* b, const Move* m);

/**
 * @brief Converte um movimento interno para o formato de string do controlador.
 *
 * @param b Estado atual do board.
 * @param m Movimento a representar.
 * @param out Buffer de saida (deve ser grande o suficiente).
 * @return 0 em caso de sucesso, diferente de 0 em caso de erro.
 */
int board_format_move (const Board* b, const Move* m, char* out);

/**
 * @brief Aplica um movimento vindo do controlador (ja parseado).
 *
 * @param b Board a ser modificado.
 * @param lado Lado do movimento ('o' ou 'c').
 * @param tipo Tipo do movimento ('n', 'm', 's').
 * @param num_mov Numero de posicoes (inclui origem).
 * @param mov_l Vetor de linhas das posicoes.
 * @param mov_c Vetor de colunas das posicoes.
 * @return 0 em caso de sucesso, diferente de 0 em caso de erro.
 */
int board_apply_raw_move (Board* b, char lado, char tipo, int num_mov, const int* mov_l, const int* mov_c);

/**
 * @brief Retorna a peca em um vertice.
 *
 * @return PIECE_* ou PIECE_EMPTY.
 */
Piece board_piece_at (const Board* b, int v);

/**
 * @brief Retorna 1 se o vertice estiver vazio, 0 caso contrario.
 */
int board_is_empty (const Board* b, int v);

#endif	// BOARD_H
