#include "../includes/common.h"

int insert_front(list_t **list, void *data, size_t data_size)
{
        node_t *new = malloc(sizeof(node_t));
        if (new == NULL)
                return -1;

        node_t *tmp;

        new->data = (void *)heapAlloc(data_size);
        memcpy((void *)new->data, (void *)data, data_size);
	
        if ((*list)->head == NULL) {
                (*list)->head = new;
                (*list)->head->prev = NULL;
                (*list)->head->next = NULL;
                (*list)->tail = (*list)->head;
        } else {
                tmp = new;
                tmp->prev = NULL;
                tmp->next = (*list)->head;
                (*list)->head->prev = tmp;
                (*list)->head = tmp;
        } 
        
        return 0;
}

int insert_end(list_t **list, void *data, size_t data_size)
{
        node_t *new = malloc(sizeof(node_t));
        if (new == NULL)
                return -1;
        node_t *tmp;
        
        new->data = (void *)heapAlloc(data_size);
        memcpy((void *)new->data, (void *)data, data_size);

        if ((*list)->head == NULL) {
                (*list)->head = new;
                (*list)->head->prev = NULL;
                (*list)->head->next = NULL;
                (*list)->tail = (*list)->head;
        } else {
                for ((*list)->tail = (*list)->head; (*list)->tail != NULL;) {
                        tmp = (*list)->tail;
                        (*list)->tail = (*list)->tail->next;
                }
                (*list)->tail = new; 
                tmp->next = (*list)->tail; 
                (*list)->tail->prev = tmp;
                (*list)->tail->next = NULL;
        }

        return 0;
}


