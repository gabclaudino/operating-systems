/*

Nome: Gabriel Claudino de Souza

GRR: 20215730

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <unistd.h>
#include "ppos.h"
#include "queue.h"

// #define DEBUG // desmarcar para poder usar

// variaveis globais
task_t main_task;                       // descritor da tarefa main
task_t *current_task;                   // tarefa atualmente em execucao
task_t dispatcher_task;                 // tarefa do dispatcher
task_t *ready_queue = NULL;             // fila de tarefas prontas
int next_task_id = 0;                   // prox ID disponivel
int user_tasks = 0;                     // numero de tarefas de usuario ativas
unsigned int system_ticks = 0;          // ticks do relogio
static task_t *sleeping_queue = NULL;   // fila de tarefas dormindo

// acorda tarefas que estavam esperando
static void wake_joiners(task_t *tcb)
{
    task_t *t;
    while ((t = tcb->join_queue)) {
        // retira o head da fila de quem espera por 'tcb'
        queue_remove((queue_t **)&tcb->join_queue, (queue_t *)t);
        // acorda essa tarefa
        t->status = TASK_READY;
        queue_append((queue_t **)&ready_queue, (queue_t *)t);
    }
}

// verifica todos os sleepers e acorda os que ja devem retornar a fila de prontas
static void check_sleepers()
{
    if (!sleeping_queue)
        return;

    unsigned int now = systime();
    // head aponta para a cabeca original da lista
    task_t *head = sleeping_queue;
    task_t *t = head;

    do {
        task_t *next = t->next;  

        if (t->wake_time <= now) {
            // se for hora de acordar, removemove t de sleeping_queue e o acorda
            task_awake(t, &sleeping_queue);

            // se t era a cabeca original, atualiza head para a nova sleeping_queue
            if (t == head) {
                head = sleeping_queue;
                // se a fila ficou vazia, encerra
                if (!head)
                    break;
            }
        }

        t = next;
        // repete ate voltar a cabeça atual ou ate a fila esvaziar
    } while (head && t != head);
}

// tratador do sinal de temporizador
void tick_handler(int signum) {
    // incrementa os ticks do relogio
    system_ticks++;
    // se a tarefa atual for de usuario, decrementa o quantum
    if (current_task && current_task->task_type == 0) {
        current_task->quantum--;

        // se o quantum acabar devolve a CPU ao dispatcher
        if (current_task->quantum <= 0) {
            task_yield();
        }
    }
}

// seleciona a proxima tarefa
task_t *scheduler() {
    // se a fila estiver vazia, retorna NULL
    if (!ready_queue)
        return NULL;

    // comeca com a primeira da fila
    task_t *selected = ready_queue;

    // ponteiro para percorrer a fila
    task_t *aux = ready_queue->next;

    // percorre a fila de tarefas prontas
    do {
        // seleciona aquela com menor valor (mais prio)
        if (aux->dynamic_prio < selected->dynamic_prio)
            selected = aux;
        aux = aux->next;
    } while (aux != ready_queue);

    // aplica o aging nas outras tarefas da fila
    aux = ready_queue;
    do {
        if (aux != selected)
            aux->dynamic_prio--;
        aux = aux->next;
    } while (aux != ready_queue);

    // reseta a prioridade dinamica da tarefa selecionada
    selected->dynamic_prio = selected->static_prio;

    #ifdef DEBUG
        printf("scheduler: tarefa %d selecionada com prioridade %d\n", selected->id, selected->dynamic_prio);
    #endif
    
    // retorna a selecionada
    return selected;
}

// funcao do dispatcher
void dispatcher(void *arg) {
    #ifdef DEBUG
        printf("dispatcher: iniciando dispatcher\n");
    #endif

    // remove o dispatcher da fila de prontas
    queue_remove((queue_t **) &ready_queue, (queue_t *) &dispatcher_task);

    // inicializa os campos de tempo do dispatcher
    dispatcher_task.start_time = systime();
    dispatcher_task.proc_start_time = systime();
    dispatcher_task.activations = 1;

    // enquanto houver tarefas de usuario para executar
    while (user_tasks > 0) {

        // a cada iteracao, acorda quem ja terminou o tempo de sleep
        check_sleepers();

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

            // atualiza ativacoes e tempo de inicio
            next->activations++;
            next->proc_start_time = systime();

            // reseta o quantum
            next->quantum = QUANTUM_TICKS; 

            // troca o contexto para a tarefa escolhida
            task_switch(next);
            
            // quando a tarefa terminar ou fazer yield, volta pra ca
            // calcula tempo decorrido e atualiza processor_time
            unsigned int now = systime();
            unsigned int elapsed = now - next->proc_start_time;
            next->processor_time += elapsed;

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
            dispatcher_task.activations++;
        }

        // incrementa as ativacoes do dispatcher cada vez que ele retoma o controle
        dispatcher_task.proc_start_time = systime();

    }

    // atualizar o tempo de processador do dispatcher antes de sair
    unsigned int now = systime();
    unsigned int elapsed = now - dispatcher_task.proc_start_time;
    dispatcher_task.processor_time += elapsed;


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

    // inicializacao da task main
    main_task.id = next_task_id++;
    main_task.status = TASK_RUNNING;
    main_task.prev = NULL;
    main_task.next = NULL;
    main_task.static_prio = 0;
    main_task.dynamic_prio = 0;
    main_task.quantum = QUANTUM_TICKS;
    main_task.task_type = 0;
    main_task.start_time = 0;
    main_task.proc_start_time = 0;
    main_task.execution_time = 0;
    main_task.processor_time = 0;
    main_task.activations = 1;

    // define a main como tarefa atualmente em execucao
    current_task = &main_task;

    // inicializa o dispatcher
    task_init(&dispatcher_task, dispatcher, NULL);

    // configura o tratador de sinal SIGALRM
    struct sigaction action;
    action.sa_handler = tick_handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    sigaction(SIGALRM, &action, 0);

    // configura o temporizador para gerar ticks a cada 1ms
    struct itimerval timer;
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = TICK_INTERVAL;
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = TICK_INTERVAL;
    setitimer(ITIMER_REAL, &timer, 0);

    #ifdef DEBUG
        printf("ppos_init: sistema inicializado\n");
        printf("ppos_init: tarefa main criada com id %d\n", main_task.id);
        printf("ppos_init: dispatcher criado com id %d\n", dispatcher_task.id);
    #endif
}


// inicia uma nova tarefa
int task_init(task_t *task, void (*start_routine)(void *), void *arg) {
    // verificacao para ver se a task eh nula
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

    // atribui o proximo ID
    task->id = next_task_id++;

    // define as prioridades da tarefa (default = 0)
    task->static_prio = 0;
    task->dynamic_prio = 0;

    // marca como pronta
    task->status = TASK_READY; 

    task->prev = NULL;
    task->next = NULL;

    // inicializa os campos de tempo e ativacoes
    task->start_time = systime();
    task->proc_start_time = 0;
    task->execution_time = 0;
    task->processor_time = 0;
    task->activations = 0;

    // tipo
    if(task == &dispatcher_task || task == &main_task) {
        task->task_type = 1;
    }
    else {
        task->task_type = 0;
    }

    // tempo de quantum
    task->quantum = QUANTUM_TICKS;

    // fila de tasks esperando essa acabar
    task->join_queue = NULL;


    // adiciona se n for nem o dispatcher nem a main na fila de prontas
    if (task->task_type == 0) {
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
void task_exit (int exit_code)
{
    #ifdef DEBUG
        printf("task_exit: tarefa %d sendo encerrada com exit code %d\n", current_task->id, exit_code);
    #endif

    // atualiza o tempo de processador da tarefa antes de sair
    unsigned int now = systime();
    unsigned int elapsed = now - current_task->proc_start_time;
    current_task->processor_time += elapsed;

    // armazena o exit_code
    current_task->exit_code = exit_code;
    // marca como TERMINATED
    current_task->status    = TASK_TERMINATED;

    // acorda quem estiver esperando (para qualquer tarefa)
    wake_joiners(current_task);

    // se a task eh o dispatcher dando exit
    if (current_task == &dispatcher_task) {
        // devolve o controle ao main para que ele finalize tudo
        task_switch(&main_task);

        // quando o main fizer task_exit(0) e der
        // switch de volta para o dispatcher, imprime as estatisticas
        printf("Task %d exit: running time %u ms, cpu time %u ms, %d activations\n",
               dispatcher_task.id,
               now - dispatcher_task.start_time,
               dispatcher_task.processor_time,
               dispatcher_task.activations);
        // encerra o processo
        exit(0);
    }
    else {
        // caso eh uma user task
        printf("Task %d exit: running time %u ms, cpu time %u ms, %d activations\n",
               current_task->id,
               now - current_task->start_time,
               current_task->processor_time,
               current_task->activations);
               
        // volta ao dispatcher
        task_switch(&dispatcher_task);
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

// ajusta a prioridade estatica
void task_setprio(task_t *task, int prio) {
    
    // garante que a prio esta dentro dos limites
    if (prio < -20) 
        prio = -20;
    else if (prio > 20) 
        prio = 20;

    // se a tarefa for nula, assume que eh a a tarefa corrente
    if (!task)
        task = current_task;

    // define as prios da task
    task->static_prio = prio;
    task->dynamic_prio = prio;

    #ifdef DEBUG
        printf("task_setprio: tarefa %d prioridade setada para %d\n", task->id, prio);
    #endif
}

// devolve o valor da prio estatica
int task_getprio(task_t *task) {
    if (!task)
        task = current_task;
    return task->static_prio;
}

unsigned int systime() {
    return system_ticks;
}

// suspende a tarefa atual,
// transferindo-a da fila de prontas para a fila "queue"
void task_suspend(task_t **queue) {
    // remove da fila de pronto
    if (current_task->status == TASK_READY)
        queue_remove((queue_t **)&ready_queue, (queue_t *)current_task);

    // maraca a task como SUSPENDED
    current_task->status = TASK_SUSPENDED;

    // insere a task na fila de espera
    if (queue)
        queue_append((queue_t **)queue, (queue_t *)current_task);

    // devolve o processador pro dispatcher
    task_switch(&dispatcher_task);
}

// acorda a tarefa indicada,
// trasferindo-a da fila "queue" para a fila de prontas
void task_awake(task_t *task, task_t **queue) {
    if (!task) return;
    
    // remove a task da fila de espera
    if (queue)
        queue_remove((queue_t **)queue, (queue_t *)task);
    
    // marca como READY
    task->status = TASK_READY;

    // insere a task de volta na fila de prontas
    queue_append((queue_t **)&ready_queue, (queue_t *)task);

    // continua a task atual
}

// a tarefa corrente aguarda o encerramento de outra task
int task_wait(task_t *task) {
    // se a task for NULL ou estiver TERMINADA, retorna erro
    if (!task || task->status == TASK_TERMINATED)
        return -1;
    
    // suspende a tarefa atual
    task_suspend(&task->join_queue);
    
    // qnd acordar, retorna o exit_code
    return task->exit_code;
}

// suspende a tarefa corrente por t milissegundos
void task_sleep (int t){

    // verificacao de tempo positivo
    if (t <= 0)
        return; 

    // calcula instante de acordar
    current_task->wake_time = systime() + t;

    // suspende a task atual, colocando-a em sleeping_queue
    task_suspend(&sleeping_queue);

    // quando voltar para ca, a task ja foi acordada pelo dispatcher
}



