/*

Nome: Gabriel Claudino de Souza

GRR: 20215730

*/

// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Versão 1.5 -- Março de 2023

// Estruturas de dados internas do sistema operacional

#ifndef __PPOS_DATA__
#define __PPOS_DATA__

#include <ucontext.h>		// biblioteca POSIX de trocas de contexto

#define STACKSIZE 64 * 1024     // tamanho da pilha de cada tarefa
#define TICK_INTERVAL 1000      // intervalo de 1 ms
#define QUANTUM_TICKS 20        // quantum padrão (20 ms)          

// estados das tarefas
#define TASK_READY     0
#define TASK_RUNNING   1
#define TASK_TERMINATED 2



// Estrutura que define um Task Control Block (TCB)
typedef struct task_t
{
  struct task_t *prev, *next ;		// ponteiros para usar em filas
  int id ;				      // identificador da tarefa
  ucontext_t context ;			// contexto armazenado da tarefa
  short status ;      // pronta, rodando, suspensa, ...
  int static_prio;    // prioridade estatica (-20 a +20)
  int dynamic_prio;   // prioridade dinamica usada pelo escalonador (com aging)
  int quantum;        // tempo de quantum
  int task_type;      // 0 = user, 1 = system

  unsigned int start_time;       // instante de criacao da tarefa
  unsigned int proc_start_time;  // instante da ultima ativacao
  unsigned int execution_time;   // tempo total desde a criacao ate o fim
  unsigned int processor_time;   // tempo efetivo de uso da CPU
  int activations;               // numero de ativacoes

  int exit_code;                  // codigo passado a task_exit()
  struct task_t *join_queue;      // fila de tasks esperando essa acabar
  // ... (outros campos serão adicionados mais tarde)
} task_t ;

// estrutura que define um semáforo
typedef struct
{
  // preencher quando necessário
} semaphore_t ;

// estrutura que define um mutex
typedef struct
{
  // preencher quando necessário
} mutex_t ;

// estrutura que define uma barreira
typedef struct
{
  // preencher quando necessário
} barrier_t ;

// estrutura que define uma fila de mensagens
typedef struct
{
  // preencher quando necessário
} mqueue_t ;

#endif

