

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ppos.h"


// #define DEBUG // desmarcar para poder usar

// tamanho da pilha de cada tarefa
#define STACKSIZE 64 * 1024

// variaveis globais

task_t main_task;       // descritor da main   
task_t *current_task;   // tarefa em execucao    
int next_task_id = 0;   // tarefa em execucao  

// inicializa o sistema
void ppos_init() {
    /* desativa o buffer da saida padrao (stdout), usado pela função printf */
    setvbuf(stdout, 0, _IONBF, 0);

    // inicializa o descritor da main
    // main tem o ID = 0
    main_task.id = next_task_id++;
    // status 0 = pronta
    main_task.status = 0; 
    // ponteiros da fila
    main_task.prev = NULL;
    main_task.next = NULL;

    // salva o contexto atual (da main) no descritor da tarefa main
    getcontext(&(main_task.context)); 

    // define a main como tarefa atualmente em execucao
    current_task = &main_task;

    #ifdef DEBUG
        printf("ppos_init: sistema inicializado\n");
        printf("ppos_init: tarefa main criada com id %d\n", main_task.id);
    #endif
}

// inicia uma nova tarefa
int task_init(task_t *task, void (*start_routine)(void *), void *arg) {
    // verificacao para ver se a task existe
    // se nao existe, entao eh um erro e retorna -1
    if (!task)
        return -1;

    // cria e salva um novo contexto para a tarefa
    getcontext(&(task->context));

    // aloca a pilha da tarefa
    // se nao conseguir alocar, eh um erro e retorna -2
    char *stack = malloc(STACKSIZE);
    if (!stack) {
        perror("Erro ao alocar pilha!");
        return -2;
    }

    // configura o contexto
    task->context.uc_stack.ss_sp = stack;
    task->context.uc_stack.ss_size = STACKSIZE;
    task->context.uc_stack.ss_flags = 0;
    task->context.uc_link = 0;  

    // prepara o contexto para iniciar a func
    makecontext(&(task->context), (void (*)(void))start_routine, 1, arg);

    // atribui valores ao descritor
    // atribui o proximo ID
    task->id = next_task_id++;
    // marca como pronta
    task->status = 0; 
    task->prev = NULL;
    task->next = NULL;

    #ifdef DEBUG
        printf("task_init: iniciada tarefa %d\n", task->id);
    #endif

    //retorno do ID da tarefa
    return task->id;
}

// troca o contexto para outra tarefa
int task_switch(task_t *task) {

    // verifica se a tarefa existe
    if (!task)
        return -1;

    #ifdef DEBUG
        printf("task_switch: trocando contexto %d -> %d\n", current_task->id, task->id);
    #endif

    // salva a tarefa atual em uma variavel
    task_t *prev_task = current_task;

    // salva a tarefa que ira receber o processador na var global
    current_task = task;

    // troca de contexto, salva o atual e carrega o novo
    return swapcontext(&(prev_task->context), &(task->context));
}

// termina a tarefa corrente
void task_exit(int exit_code) {

    #ifdef DEBUG
        printf("task_exit: tarefa %d sendo encerrada\n", current_task->id);
    #endif


    // retorna o processador para a main
    if (current_task != &main_task)
        task_switch(&main_task);
}

// retorna o ID da tarefa atual
int task_id() {
    if (current_task)
        return current_task->id;
    return -1;
}
