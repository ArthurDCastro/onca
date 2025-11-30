CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11 -g

# Objetos comuns
OBJS_COMMON    = graph.o game.o ai.o

# Executaveis
PLAYER_OBJS    = $(OBJS_COMMON) player.o
TEST_GAME_OBJS = $(OBJS_COMMON) test_game.o
TEST_GRAPH_OBJS= graph.o test_graph.o

.PHONY: all clean

all: player test_game test_graph

# ---- binarios ----

player: $(PLAYER_OBJS)
	$(CC) $(CFLAGS) -o $@ $(PLAYER_OBJS)

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

player.o: player.c game.h graph.h
	$(CC) $(CFLAGS) -c player.c

test_game.o: test_game.c game.h graph.h
	$(CC) $(CFLAGS) -c test_game.c

test_graph.o: test_graph.c graph.h
	$(CC) $(CFLAGS) -c test_graph.c

# ---- util ----

clean:
	rm -f *.o player test_game test_graph
