#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <hiredis/hiredis.h>

#include "ai.h"
#include "game.h"

#define REDIS_IP "127.0.0.1"
#define REDIS_PORT 10001
#define MAX_BUFFER_SIZE 512
#define AI_DEFAULT_DEPTH 6

// As definições de caracteres do controlador devem ser as mesmas usadas em game.c/player.c
#ifndef CTRL_JAGUAR_CHAR
#define CTRL_JAGUAR_CHAR 'o'
#endif

#ifndef CTRL_DOG_CHAR
#define CTRL_DOG_CHAR 'c'
#endif

/**
 * @brief Conecta ao servidor Redis.
 * @return Um ponteiro para a estrutura redisContext, ou NULL em caso de falha.
 */
static redisContext* connect_redis() {
    redisContext* c = redisConnect(REDIS_IP, REDIS_PORT);
    if (c == NULL || c->err) {
        if (c) {
            fprintf(stderr, "Erro ao conectar com o servidor Redis: %s\n", c->errstr);
            redisFree(c);
        } else {
            fprintf(stderr, "Não foi possível alocar o contexto Redis\n");
        }
        return NULL;
    }
    return c;
}

/**
 * @brief Lê o estado do jogo do Redis, na chave tabuleiro_<lado>.
 * @param c Contexto Redis.
 * @param side O lado da IA ('o' ou 'c').
 * @param timeout Tempo limite de bloqueio (em segundos).
 * @param out_full_state Buffer para a string completa do estado.
 * @param out_lado_a_jogar Char para o lado que o controlador espera que jogue.
 * @param out_tabuleiro String do tabuleiro.
 * @return 0 em sucesso, -1 em timeout/erro.
 */
static int read_game_state(redisContext* c, char side, const char* timeout,
                           char* out_full_state, char* out_lado_a_jogar, char* out_tabuleiro) {
    redisReply* reply;
    char key[32];
    sprintf(key, "tabuleiro_%c", side);

    // BLPOP
    reply = redisCommand(c, "BLPOP %s %s", key, timeout);

    if (reply == NULL) {
        fprintf(stderr, "Erro de comunicação com o Redis.\n");
        return -1;
    }

    if (reply->type == REDIS_REPLY_NIL) {
        // Timeout
        freeReplyObject(reply);
        return -1;
    }

    // A resposta BLPOP é uma lista: [chave, valor]. O valor é reply->element[1]->str
    // Copia o valor para out_full_state
    strcpy(out_full_state, reply->element[1]->str);
    freeReplyObject(reply);
    
    // O formato esperado pelo controlador é:
    // <lado_a_jogar>\n
    // <jogada_anterior>\n
    // <tabuleiro>
    
    char *tabuleiro_start = out_full_state;
    
    // Obtem o lado do jogo
    // Usamos o strchr para encontrar o primeiro '\n'
    char *separator1 = strchr(tabuleiro_start, '\n'); 
    if (separator1 == NULL) {
        fprintf(stderr, "Formato do estado inválido (lado/separador 1 ausente).\n");
        return -1;
    }

    // Copia o primeiro caractere (o lado)
    *out_lado_a_jogar = tabuleiro_start[0];
    
    // Procura o início da terceira linha (o tabuleiro)
    // O tabuleiro começa após o segundo '\n'.
    
    // Começa a busca após o primeiro separador
    char *separator2 = strchr(separator1 + 1, '\n');
    if (separator2 == NULL) {
        fprintf(stderr, "Formato do estado inválido (separador 2 ausente - Jogada anterior ou Tabuleiro).\n");
        return -1;
    }
    
    // O início do tabuleiro é o caractere imediatamente após o segundo '\n'
    tabuleiro_start = separator2 + 1;

    // Verifica se o buffer de destino tem espaço (segurança básica)
    if (strlen(tabuleiro_start) >= MAX_BUFFER_SIZE) {
         fprintf(stderr, "Formato do estado inválido (Tab. muito grande).\n");
         return -1;
    }

    // Copia o restante da string para o out_tabuleiro
    strcpy(out_tabuleiro, tabuleiro_start);
    
    // Opcional: Para depuração, termina a string do lado a jogar.
    *separator1 = '\0';

    return 0;
}

/**
 * @brief Envia a jogada para o Redis, na chave jogada_<lado>.
 * @param c Contexto Redis.
 * @param side O lado da IA ('o' ou 'c').
 * @param move_str A string da jogada formatada.
 * @return 0 em sucesso, -1 em caso de erro.
 */
static int send_move(redisContext* c, char side, const char* move_str) {
    redisReply* reply;
    char key[32];
    sprintf(key, "jogada_%c", side);

    // RPUSH: Adiciona a jogada no final da lista
    reply = redisCommand(c, "RPUSH %s %s", key, move_str);

    if (reply == NULL) {
        // Erro de comunicação: o erro está no contexto
        fprintf(stderr, "Erro ao enviar a jogada para o Redis: %s\n", c->errstr);
        return -1;
    }

    if (reply->type == REDIS_REPLY_ERROR) {
        // A resposta do Redis indica um erro
        fprintf(stderr, "Erro do servidor Redis: %s\n", reply->str);
        freeReplyObject(reply);
        return -1;
    }
    freeReplyObject(reply);
    return 0;
}
int main (int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <lado_ia> [profundidade]\n", argv[0]);
        fprintf(stderr, "Ex: %s o 5\n", argv[0]);
        return 1;
    }

    char ia_side_char = argv[1][0];
    if (ia_side_char != CTRL_JAGUAR_CHAR && ia_side_char != CTRL_DOG_CHAR) {
        fprintf(stderr, "Lado da IA inválido. Use '%c' (onça) ou '%c' (cão).\n", CTRL_JAGUAR_CHAR, CTRL_DOG_CHAR);
        return 1;
    }

    int depth = AI_DEFAULT_DEPTH;
    if (argc >= 3) {
        depth = atoi(argv[2]);
        if (depth < 1) {
            fprintf(stderr, "Profundidade inválida. Usando a default: %d.\n", AI_DEFAULT_DEPTH);
            depth = AI_DEFAULT_DEPTH;
        }
    }

    // O timeout será lido do controlador original,
    const char* blpop_timeout = "180"; // 10 minutos (tempo suficiente para o controlador enviar o tabuleiro)

    Game game;
    AiConfig ai_cfg;
    ai_cfg.max_depth = depth;
    ai_cfg.side = (ia_side_char == CTRL_JAGUAR_CHAR) ? CELL_JAGUAR : CELL_DOG;

    if (game_init(&game) != 0) {
        fprintf(stderr, "Falha na inicialização do jogo.\n");
        return 1;
    }

    redisContext* c = connect_redis();
    if (!c) return 1;

    printf("AI Player (Lado: %c, Profundidade: %d) conectado. Aguardando a vez...\n", ia_side_char, depth);

    // Loop principal: Aguardar a vez, calcular e enviar a jogada
    while (1) {
        char full_state_buffer[MAX_BUFFER_SIZE * 2]; // Maior que o tabuleiro
        char board_buffer[MAX_BUFFER_SIZE];
        char lado_a_jogar_char = ' ';

        //Ler o estado do Redis (BLPOP)
        if (read_game_state(c, ia_side_char, blpop_timeout,
                            full_state_buffer, &lado_a_jogar_char, board_buffer) != 0) {
            printf("Fim do jogo ou erro na leitura do estado. Encerrando.\n");
            break;
        }

        // Verificar se é sua vez (o controlador enviará o tabuleiro do seu lado)
        if (lado_a_jogar_char != ia_side_char) {
            fprintf(stderr, "Erro de sincronização: o controlador espera a jogada de '%c', mas é a vez de '%c' no loop de leitura da IA.\n", lado_a_jogar_char, ia_side_char);
            //  pode indicar o fim do jogo ou um erro de lógica do controlador.
            continue;
        }
        
        printf("\nTurno da IA (%c). Estado recebido:\n%s", ia_side_char, board_buffer);

        // Converter a string do tabuleiro para a estrutura Game
        if (game_from_controller_board(&game, board_buffer, lado_a_jogar_char) != 0) {
            fprintf(stderr, "Falha ao carregar o estado do tabuleiro.\n");
            break;
        }

        // Testar o estado terminal antes de calcular a jogada
        CellContent winner;
        if (game_get_winner(&game, &winner) == 1) {
            printf("Jogo terminado (vencedor: %c). Não farei jogada.\n", (winner == CELL_JAGUAR) ? CTRL_JAGUAR_CHAR : CTRL_DOG_CHAR);
            
            // Enviamos uma jogada nula para o outro lado (no caso do controlador original,
            // ele fará isso, mas para ser robusto, podemos garantir que não bloqueamos)
            char no_move_buf[32];
            sprintf(no_move_buf, "%c n", ia_side_char);
            send_move(c, ia_side_char, no_move_buf);
            break;
        }

        // Calcular a melhor jogada
        Move best_move;
        int ar = ai_choose_move(&game, &ai_cfg, &best_move);

        char move_buffer[MAX_BUFFER_SIZE];
        if (ar != 0) {
            if (ar == 1) {
                // Sem movimentos (derrota ou empate)
                printf("Agente (%c) não encontrou movimentos legais. Enviando jogada nula.\n", ia_side_char);
                sprintf(move_buffer, "%c n", ia_side_char);
            } else {
                fprintf(stderr, "Erro na função ai_choose_move (err=%d).\n", ar);
                sprintf(move_buffer, "%c n", ia_side_char); // Envia nulo para não bloquear
            }
        } else {
            //Formatar a jogada para o controlador
            if (game_move_to_controller(&game, &best_move, move_buffer, (int)sizeof move_buffer) != 0) {
                fprintf(stderr, "Falha ao formatar a jogada para o controlador.\n");
                sprintf(move_buffer, "%c n", ia_side_char);
            }
        }

        printf("Agente (%c) jogada calculada: %s\n", ia_side_char, move_buffer);

        // Enviar a jogada para o Redis
        if (send_move(c, ia_side_char, move_buffer) != 0) {
            fprintf(stderr, "Falha ao enviar a jogada. Encerrando.\n");
            break;
        }
    }

    redisFree(c);
    return 0;
}