CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11 -g
LDLIBS = -l hiredis -l readline

# Objetos comuns
OBJS_COMMON    = graph.o game.o ai.o

# Executaveis
# Atualizado para usar o novo objeto ai_controller.o
PLAYER_OBJS    = $(OBJS_COMMON) ai_controller.o
TEST_GAME_OBJS = $(OBJS_COMMON) test_game.o
TEST_GRAPH_OBJS= graph.o test_graph.o

.PHONY: all clean

# Adicionado 'controlador' à lista 'all'
all:  ai_player test_game test_graph

# ---- binarios ----

# O alvo ai_player agora vincula os objetos definidos em PLAYER_OBJS
ai_player: $(PLAYER_OBJS)
	$(CC) $(CFLAGS) -o $@ $(PLAYER_OBJS) $(LDLIBS)

test_game: $(TEST_GAME_OBJS)
	$(CC) $(CFLAGS) -o $@ $(TEST_GAME_OBJS)

test_graph: $(TEST_GRAPH_OBJS)
	$(CC) $(CFLAGS) -o $@ $(TEST_GRAPH_OBJS)

# ---- objetos ----

graph.o: graph.c graph.h
	$(CC) $(CFLAGS) -c graph.c

game.o: game.c game.h graph.h
	$(CC) $(CFLAGS) -c game.c

ai.o: ai.c ai.h
	$(CC) $(CFLAGS) -c ai.c

# Regra para o novo arquivo main do player
ai_controller.o: ai_controller.c game.h graph.h
	$(CC) $(CFLAGS) -c ai_controller.c

# A regra original ai_player.o (que usava player.c) foi substituída.

test_game.o: test_game.c game.h graph.h
	$(CC) $(CFLAGS) -c test_game.c

test_graph.o: test_graph.c graph.h
	$(CC) $(CFLAGS) -c test_graph.c

# ---- util ----

clean:
	rm -f *.o  ai_player test_game test_graph