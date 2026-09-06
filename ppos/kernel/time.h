// Gerência básica do tempo.

#ifndef __PPOS_TIME__
#define __PPOS_TIME__

#define TICK     1   // duração do tick em milissegundos
#define QUANTUM 10   // duracao default do quantum em ticks

// inicia o subsistema de gestão do tempo
// (chamada pelo núcleo na inicialização).
void time_init();

// encerra o subsistema de gestão do tempo
// (chamada pelo núcleo no encerramento).
void time_term();

// informa o valor atual do relógio (milissegundos)
unsigned int time();

#endif
