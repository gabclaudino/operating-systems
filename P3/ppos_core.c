

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ppos.h"
#include "queue.h"


// #define DEBUG // desmarcar para poder usar

// tamanho da pilha de cada tarefa
#define STACKSIZE 64 * 1024

// estados das tarefas
#define TASK_READY     0
#define TASK_RUNNING   1
#define TASK_TERMINATED 2

// variaveis globais

task_t main_task;              // descritor da tarefa main
task_t *current_task;          // tarefa atualmente em execucao
task_t dispatcher_task;        // tarefa do dispatcher
task_t *ready_queue = NULL;    // fila de tarefas prontas
int next_task_id = 0;          // prox ID disponivel
int user_tasks = 0;            // numero de tarefas de usuario ativas

// seleciona a proxima tarefa (FCFS)
task_t* scheduler() {
    return ready_queue;
}

// funcao do dispatcher
void dispatcher(void *arg) {
    #ifdef DEBUG
        printf("dispatcher: iniciando dispatcher\n");
    #endif

    // remove o dispatcher da fila de prontas
    queue_remove((queue_t **) &ready_queue, (queue_t *) &dispatcher_task);

    // enquanto houver tarefas de usuario para executar
    while (user_tasks > 0) {
        // o scheduler pega a proxima tarefa
        task_t *next = scheduler();
        // a tarefa for valida
        if (next) {
            #ifdef DEBUG
                printf("dispatcher: próxima tarefa %d\n", next->id);
            #endif

            // remove a tarefa da fila antes de executa-la
            queue_remove((queue_t **) &ready_queue, (queue_t *) next);
            // define o status como RUNNING
            next->status = TASK_RUNNING;
            // troca o contexto para a tarefa escolhida
            task_switch(next);

            // quando a tarefa terminar ou fazer yield, volta pra ca
            switch (next->status) {
                // se ela esta pronta, volta pro final da fila
                case TASK_READY:
                    queue_append((queue_t **) &ready_queue, (queue_t *) next);
                    break;
                    
                // se foi finalizada, drecrementa o contador de tarefas do user 
                case TASK_TERMINATED:
                    user_tasks--;
                    break;
            }
        }
    }

    #ifdef DEBUG
        printf("dispatcher: encerrando dispatcher\n");
    #endif

    task_exit(0);
}



// inicializa o sistema
void ppos_init() {
    /* desativa o buffer da saida padrao (stdout), usado pela função printf */
    setvbuf(stdout, 0, _IONBF, 0);

    // salva o contexto atual (da main) no descritor da tarefa main
    getcontext(&(main_task.context));

    // inicializa o descritor da main
    // main tem o ID = 0
    main_task.id = next_task_id++;
    // define o status como executando
    main_task.status = TASK_RUNNING;
    // ponteiros da fila
    main_task.prev = NULL;
    main_task.next = NULL;

    // define a main como tarefa atualmente em execucao
    current_task = &main_task;

    // inicializa o dispatcher
    task_init(&dispatcher_task, dispatcher, NULL);

    #ifdef DEBUG
        printf("ppos_init: sistema inicializado\n");
        printf("ppos_init: tarefa main criada com id %d\n", main_task.id);
        printf("ppos_init: dispatcher criado com id %d\n", dispatcher_task.id);
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
    task->status = TASK_READY; 
    task->prev = NULL;
    task->next = NULL;

     // nao adiciona o dispatcher nem a main na fila de prontas
    if (task != &dispatcher_task && task != &main_task) {
        queue_append((queue_t **) &ready_queue, (queue_t *) task);
        user_tasks++;
    }


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

    // marca o status da tak como TERMINATED
    current_task->status = TASK_TERMINATED;

    // se a task eh o dispatcher, entao termina o programa
    if (current_task == &dispatcher_task) {
        exit(0);
    } else {
        task_switch(&dispatcher_task); // se nao manda para o dispatcher
    }
}

// retorna o ID da tarefa atual
int task_id() {
    if (current_task)
        return current_task->id;
    return -1;
}

// devolve o processador ao dispatcher
void task_yield() {
    #ifdef DEBUG
        printf("task_yield: tarefa %d devolvendo CPU ao dispatcher\n", current_task->id);
    #endif

    // marca o status da tarefa como READY
    current_task->status = TASK_READY;
    // coloca a tarefa no fim da fila
    queue_append((queue_t **) &ready_queue, (queue_t *) current_task);
    // passa o controle pro dispatcher
    task_switch(&dispatcher_task);
}