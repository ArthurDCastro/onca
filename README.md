# README – Projeto Onça x Cães (Trabalho 2 de IA)

Este projeto implementa o jogo **Onça x Cães** como um **problema de busca adversária**, dividido em três módulos principais:

- **graph/** – representação estrutural do tabuleiro
- **game/** – regras do jogo e manipulação de estados
- **ai/** – algoritmos de MINIMAX, Alfa-Beta e função de avaliação

Além disso, existe um **player de teste** (`player.c`) usado para jogar contra o agente via `stdin`.
A comunicação interna já segue **o formato do controlador do professor** — basta substituir o `stdin/stdout` pelo Redis depois.

---

## 📦 Módulo `graph` – Representação estrutural do tabuleiro

**Responsabilidade:**
Modelar o mapa/board como um **grafo não-direcional**, onde cada nó é uma posição válida do tabuleiro e cada aresta representa um movimento permitido.

### Funcionalidades principais

- **Carregamento do mapa** a partir de um arquivo ASCII (`load_map`).
- **Exploração automática do mapa** para encontrar vértices e suas conexões (`explorer`).
- **Representação compacta do grafo** via:

  - `Vertex`
  - lista de vizinhos
  - coordenadas (linha/coluna)

- **Conversões úteis:**

  - `graph_get_coord` – obtém coordenada de um vértice
  - `graph_get_index` – converte coordenada → id do vértice
  - `graph_get_neighbors` – retorna todos os vizinhos de um vértice
  - `graph_get_mid_jump` – retorna o vértice “do meio” para movimentos de salto

O módulo **não contém regras do jogo**, apenas a estrutura geométrica.

---

## 🎮 Módulo `game` – Estado, regras e transições

**Responsabilidade:**
Ser a **fonte da verdade** sobre o estado do jogo, suas regras e transições.

### O que ele faz

- Mantém o **estado atual do tabuleiro** (quem está em qual vértice).
- Aplica **todas as regras oficiais**:

  - movimentos simples
  - saltos da onça
  - captura de cães
  - mudança de jogador

- Converte dados entre:

  - **formato interno**
  - **formato do controlador** (string `"o m 4 3 5 3"` etc)

- Verifica:

  - movimentos legais (`game_is_legal_move`)
  - estados terminais (`game_get_winner`)

- Gera movimentos válidos (`game_generate_moves`).

### Importante

Toda a comunicação já está no **mesmo formato do controlador oficial**.
Ou seja: _quando conectarmos ao Redis, nada da lógica precisa mudar_ — apenas a fonte dos dados.

---

## 🤖 Módulo `ai` – MINIMAX, Alfa-Beta e avaliação heurística

**Responsabilidade:**
Implementar a inteligência artificial do agente usando algoritmos clássicos de busca adversária.

### Funções explícitas (como pedido na especificação)

- `ai_evaluate`
  Função de **estimativa de recompensa** quando a profundidade atinge zero.

- `ai_minimax`
  MINIMAX puro, sem podas.

- `ai_alphabeta`
  MINIMAX com poda alfa-beta (versão eficiente utilizada pelo agente).

- `ai_choose_move`
  Interface simples para o jogador automático:

  - gera todos os movimentos possíveis
  - executa a busca
  - retorna o melhor movimento

O módulo **não altera o estado real do jogo** — sempre trabalha em **cópias de `Game`**.

---

## 👤 `player.c` – Jogador de teste (humano vs IA)

`player.c` funciona como um **cliente local para testar a IA**:

- permite que você jogue **via stdin** digitando jogadas no formato do controlador
  (ex.: `c m 4 3 5 3`)
- o agente responde com um movimento escolhido
- mostra o tabuleiro após cada jogada

⚠️ **Importante:**
Este `player` **ainda NÃO está plugado no controlador Redis** do professor.
Ele é apenas um driver local para depuração.

Porém…

👉 **Toda a conversão de jogadas e tabuleiros já está no formato oficial**
Ou seja, para integrar com o controlador:

- substituir a leitura (`stdin`) pela leitura via Redis
- substituir o print (`stdout`) pelo envio via Redis

Nenhuma outra lógica do `player` ou da IA precisa mudar.

---

## 🧪 Testes

Existem dois programas de teste:

- `test_graph`
  Verifica carregamento do mapa, exploração e conectividade.

- `test_game`
  Testa regras básicas, tradução de jogadas e aplicação de movimentos.

Ambos são úteis antes de rodar a IA.

---

## 🔧 Compilação

O `Makefile` compila:

- `player`
- `test_game`
- `test_graph`

Com:

```sh
make
```

E limpa com:

```sh
make clean
```
