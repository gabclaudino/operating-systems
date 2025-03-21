/*

Nome: Gabriel Claudino de Souza

GRR: 20215730

*/

#include <stdio.h>
#include <stdlib.h>
#include "queue.h"


//------------------------------------------------------------------------------
// Conta o numero de elementos na fila
// Retorno: numero de elementos na fila

int queue_size (queue_t *queue){
    // verifica se a fila esta vazia
    if(!queue)
        return 0;
    
    // var para contador
    int cont = 1;

    // ponteiro para o comeco da fila
    queue_t *current = queue->next;

    // percorrer a fila usando o ponteiro
    while(current && current != queue){
        cont++;
        current = current->next;
    }

    // retorna o contador
    return cont;
}

//------------------------------------------------------------------------------
// Percorre a fila e imprime na tela seu conteúdo. A impressão de cada
// elemento é feita por uma função externa, definida pelo programa que
// usa a biblioteca. Essa função deve ter o seguinte protótipo:
//
// void print_elem (void *ptr) ; // ptr aponta para o elemento a imprimir

void queue_print (char *name, queue_t *queue, void print_elem (void*) ){
    printf("%s [", name);
    
    // caso fila vazia
    if(!queue){
        printf("]\n");
        return;
    }

    // var para percorrer a fila
    queue_t *current = queue;

    // chama print_elem para cada elemento
    // vai atualizando ate voltar para o comeco
    do{
        print_elem((void *)current);
        current = current -> next;
        if(current != queue)
            printf(" ");
        
    } while(current != queue);

    printf("]\n");
}

//------------------------------------------------------------------------------
// Insere um elemento no final da fila.
// Condicoes a verificar, gerando msgs de erro:
// - a fila deve existir
// - o elemento deve existir
// - o elemento nao deve estar em outra fila
// Retorno: 0 se sucesso, <0 se ocorreu algum erro

int queue_append (queue_t **queue, queue_t *elem){
    // verifica se a fila existe
    if(!queue){
        fprintf(stderr, "Erro, a fila nao existe!\n");
        return -1;
    }

    // verifica se o elemento existe
    if(!elem){
        fprintf(stderr, "Erro, o elemento nao existe!\n");
        return -2;
    }

    // verifica se o elemento ja esta em uma fila
    if(elem->prev || elem->next){
        fprintf(stderr, "Erro, o elemento ja esta em uma fila!\n");
        return -3;
    }

    // caso fila vazia
    if(!(*queue)){
        // se esta vazia o prev e next apontam para o proprio elemento
        *queue = elem;
        elem->prev = elem;
        elem->next = elem;
    } 
    // caso fila nao vazia
    else{
        // se ao esta vazia, o elemento eh inserido no fim
        // pega o ponteiro para o ultimo elem
        queue_t *end = (*queue)->prev;

        // insere e aplica a prorpiedade de fila circular
        end->next = elem;
        elem->prev = end;
        elem->next = *queue;
        (*queue)->prev = elem;
    }

    return 0;
}

//------------------------------------------------------------------------------
// Remove o elemento indicado da fila, sem o destruir.
// Condicoes a verificar, gerando msgs de erro:
// - a fila deve existir
// - a fila nao deve estar vazia
// - o elemento deve existir
// - o elemento deve pertencer a fila indicada
// Retorno: 0 se sucesso, <0 se ocorreu algum erro

int queue_remove (queue_t **queue, queue_t *elem){
    // verifica se a fila existe
    if(!queue){
        fprintf(stderr, "Erro, a fila nao existe!\n");
        return -1;
    }

    // verifica se a fila esta vazia
    if (!(*queue)) {
        fprintf(stderr, "Erro: Fila está vazia!\n");
        return -2;
    }

    // verifica se o elemento existe
    if(!elem){
        fprintf(stderr, "Erro, o elemento nao existe!\n");
        return -3;
    }

    // var para percorrer a fila
    queue_t *current = *queue;
    // var para marcar se foi encontrado ou nao
    int found = 0;

    // percorre a fila ate encontar o elemento ou voltar para o comeco
    do{
        // verifica se encontrou
        if(current == elem){
            found = 1;
            break;
        }
        current = current->next;
    }while(current != *queue);

    // verifica se o elemento pertence a fila
    if(!found){
        fprintf(stderr, "Erro, o elemento nao pertence a fila!\n");
        return -4;
    }

    // caso onde a fila tem apenas 1 elemento
    // logo ela fica vazia apos a remocao
    if(elem->next == elem){
        *queue = NULL;
    }
    // caso contrario
    else{
        elem->prev->next = elem->next;
        elem->next->prev = elem->prev;

        // caso onde o elemento removido eh o primeiro da fila
        if(*queue == elem){
            *queue = elem->next;
        }
    }

    // desconecta o elemento
    elem->next = NULL;
    elem->prev = NULL;

    return 0;
}
