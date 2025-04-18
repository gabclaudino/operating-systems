

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ppos.h"

// #define DEBUG
// Tamanho da pilha de cada tarefa
#define STACKSIZE 64 * 1024

// Variáveis globais
task_t main_task;        // descritor da tarefa principal
task_t *current_task;    // tarefa atualmente em execução
int next_task_id = 0;    // próximo ID a ser atribuído

// Inicializa o sistema operacional
void ppos_init() {
    // Desativa o buffer da saída padrão
    setvbuf(stdout, 0, _IONBF, 0);

    // Inicializa o descritor da tarefa main
    main_task.id = next_task_id++;
    main_task.status = 0; // status 0 = pronta
    main_task.prev = NULL;
    main_task.next = NULL;

    getcontext(&(main_task.context)); // salva o contexto atual

    current_task = &main_task;

    #ifdef DEBUG
        printf("ppos_init: sistema inicializado\n");
        printf("ppos_init: tarefa main criada com id %d\n", main_task.id);
    #endif
}

// Inicializa uma nova tarefa
int task_init(task_t *task, void (*start_routine)(void *), void *arg) {
    if (!task)
        return -1;

    // Cria um novo contexto para a tarefa
    getcontext(&(task->context));

    // Aloca a pilha da tarefa
    char *stack = malloc(STACKSIZE);
    if (!stack) {
        perror("Erro ao alocar pilha");
        return -2;
    }

    // Configura o contexto
    task->context.uc_stack.ss_sp = stack;
    task->context.uc_stack.ss_size = STACKSIZE;
    task->context.uc_stack.ss_flags = 0;
    task->context.uc_link = 0;  // quando terminar, não retorna pra ninguém

    // Prepara o contexto para iniciar a função
    makecontext(&(task->context), (void (*)(void))start_routine, 1, arg);

    // Atribui valores ao descritor
    task->id = next_task_id++;
    task->status = 0; // pronta
    task->prev = NULL;
    task->next = NULL;

    #ifdef DEBUG
        printf("task_init: iniciada tarefa %d\n", task->id);
    #endif

    return task->id;
}

// Troca o contexto para outra tarefa
int task_switch(task_t *task) {
    if (!task)
        return -1;

    #ifdef DEBUG
        printf("task_switch: trocando contexto %d -> %d\n", current_task->id, task->id);
    #endif



    task_t *prev_task = current_task;
    current_task = task;

    // Troca de contexto: salva o atual e carrega o novo
    return swapcontext(&(prev_task->context), &(task->context));
}

// Termina a tarefa corrente
void task_exit(int exit_code) {

    #ifdef DEBUG
        printf("task_exit: tarefa %d sendo encerrada\n", current_task->id);
    #endif


    // Para agora, apenas retorna à tarefa main
    if (current_task != &main_task)
        task_switch(&main_task);
    // (exit_code será usado futuramente)
}

// Retorna o ID da tarefa atual
int task_id() {
    if (current_task)
        return current_task->id;
    return -1;
}
