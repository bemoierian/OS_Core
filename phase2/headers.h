#include <stdio.h> //if you don't use scanf/printf change this include
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <limits.h>
#include <string.h>
#include <math.h>
#include <errno.h>

typedef short bool;
#define true 1
#define false 0

#define SHKEY 300
#define MSGKEY 301
#define PS_SHM_KEY 402
#define SEM1_KEY 450

///==============================
// don't mess with this variable//
int *shmaddr;
//=====================================

// struct holds the process data
typedef struct processData
{
    int id; // ID of the process
    int arrivalTime;
    int runTime;
    int priority;
    int size;
} Process;
int getClk()
{
    return *shmaddr;
}

/*
 * All process call this function at the beginning to establish communication between them and the clock module.
 * Again, remember that the clock is only emulation!
 */
void initClk()
{
    int shmid = shmget(SHKEY, 4, 0444);
    while ((int)shmid == -1)
    {
        // Make sure that the clock exists
        printf("Wait! The clock not initialized yet!\n");
        sleep(1);
        shmid = shmget(SHKEY, 4, 0444);
    }
    shmaddr = (int *)shmat(shmid, (void *)0, 0);
}

void destroyClk(bool terminateAll)
{
    shmdt(shmaddr);
    if (terminateAll)
    {
        killpg(getpgrp(), SIGINT);
    }
}

// vector implementation
typedef struct vector
{
    void **items;
    int maxSize;
    int size;
} vector;
// initialize the vector
void vector_init(vector *v, int s)
{
    v->maxSize = s;
    v->size = 0;
    v->items = malloc(sizeof(void *) * v->maxSize);
}

int vector_size(vector *v)
{
    return v->size;
}

static void vector_resize(vector *v, int maxSize)
{
#ifdef DEBUG_ON
    printf("vector_resize: %d to %d\n", v->maxSize, maxSize);
#endif
    // reallocate the vector with larger size with the same data if it becomes full
    void **items = realloc(v->items, sizeof(void *) * maxSize);
    if (items)
    {
        v->items = items;
        v->maxSize = maxSize;
    }
}

// add new element in the vector and resize it if size of the vector becomes = the maxSize passed in create function
// so that the vector won't be full
void vector_add(vector *v, void *item)
{
    if (v->maxSize == v->size)
        vector_resize(v, v->maxSize * 2);
    v->items[v->size++] = item;
}
// sent an element in the vector
void vector_set(vector *v, int index, void *item)
{
    if (index >= 0 && index < v->size)
        v->items[index] = item;
}
// get an element in the vector
void *vector_get(vector *v, int index)
{
    if (index >= 0 && index < v->size)
        return v->items[index];
    return NULL;
}
// delete an element in the vector
void vector_delete(vector *v, int index)
{
    if (index < 0 || index >= v->size)
        return;

    v->items[index] = NULL;
    int i;
    for (i = 0; i < v->size - 1; i++)
    {
        v->items[i] = v->items[i + 1];
        v->items[i + 1] = NULL;
    }

    v->size--;

    if (v->size > 0 && v->size == v->maxSize / 4)
        vector_resize(v, v->maxSize / 2);
}

void vector_free(vector *v)
{
    free(v->items);
}

// pair definition
typedef struct Pair
{                    // to represent the hole using starting address and its size
    int startingAdd; // starting address of the hole
    int size;        // size of the hole
} pair;
// LINKED LIST IMPLEMENTATION
typedef struct listnode
{
    void *entry; // generic data in the list node
    struct listnode *next;
} Node;

typedef struct list
{
    Node *head;
    int size;
} List;

void CreateList(List *pl) // List initialization
{
    pl->head = NULL;
    pl->size = 0;
}

bool ListEmpty(List *pl)
{
    return (pl->size == 0);
    // return (!pl->head);
}

int ListSize(List *pl)
{
    return pl->size;
}
void DestroyList(List *pl)
{
    Node *q;
    while (pl->head)
    {
        q = pl->head->next;
        free(pl->head);
        pl->head = q;
    }
    pl->size = 0;
}

void Display(List *pl)
{ // Display the list of holes in buddy algorith
    Node *p = pl->head;
    while (p)
    {
        printf("p->startingAdd: %d p->size: %d \n", ((pair *)p->entry)->startingAdd, ((pair *)p->entry)->size);
        p = p->next;
    }
}
// insert a new node at the passed position
bool InsertList(int pos, void *e, List *pl)
{
    Node *p, *q;
    int i;
    if (p = (Node *)malloc(sizeof(Node)))
    {
        p->entry = e;
        p->next = NULL;

        if (pos == 0)
        { // works also for head = NULL
            p->next = pl->head;
            pl->head = p;
        }
        else
        {
            for (q = pl->head, i = 0; i < pos - 1; i++)
                q = q->next;
            p->next = q->next;
            q->next = p;
        }
        pl->size++;
        return true;
    }
    else
        return false;
}
// used with lists in buddy algorithm to sort accoriding to the starting address
int InsertList_pair(pair *e, List *pl)
{ // insert sorted in the list and return the index of the added element
    Node *p;
    int i = 0;
    if (p = (Node *)malloc(sizeof(Node)))
    {
        p->entry = e;
        p->next = NULL;

        if (pl->head == NULL)
        { // works also for head = NULL
            p->next = pl->head;
            pl->head = p;
        }
        else
        {
            Node *prev = pl->head;
            Node *curr = prev->next;
            if (((pair *)prev->entry)->startingAdd > ((pair *)p->entry)->startingAdd) // to handle the head
            {
                p->next = pl->head;
                pl->head = p;
            }
            else
            {
                while (curr)
                {
                    if (((pair *)curr->entry)->startingAdd > ((pair *)p->entry)->startingAdd)
                    {
                        prev->next = p;
                        p->next = curr;
                        break;
                    }
                    i++;
                    prev = curr;
                    curr = curr->next;
                }
                if (!curr)
                {
                    prev->next = p; // na fel a5er
                    p->next = NULL;
                }
            }
        }
        pl->size++;
        return i;
    }
    else
        return -1;
}
void DeleteList_process(int pos, Process *pe, List *pl) // delete process from the waitQ
{
    int i;
    Node *q, *tmp;

    if (pos == 0)
    {
        *pe = *((Process *)pl->head->entry);
        tmp = pl->head->next;
        free(pl->head);
        pl->head = tmp;
    } // it works also for one node
    else
    {
        for (q = pl->head, i = 0; i < pos - 1; i++)
            q = q->next;

        *pe = *((Process *)q->next->entry);
        tmp = q->next->next;
        free(q->next);
        q->next = tmp;
    } // check for pos=size-1 (tmp will be NULL)
    pl->size--;
} // O(n) but without shifting elements.

void DeleteList_pair(int pos, pair *pe, List *pl)
{
    int i;
    Node *q, *tmp;
    if (pos == 0)
    {
        if (pe)
            *pe = *((pair *)pl->head->entry);
        printf("in delete \n");
        tmp = pl->head->next;
        free(pl->head);
        pl->head = tmp;
    } // it works also for one node
    else
    {
        for (q = pl->head, i = 0; i < pos - 1; i++)
            q = q->next;
        if (pe)
            *pe = *((pair *)q->next->entry);
        tmp = q->next->next;
        free(q->next);
        q->next = tmp;
    } // check for pos=size-1 (tmp will be NULL)
    pl->size--;
} // O(n) but without shifting elements.

void RetrieveList_Node(int pos, Node **pe, List *pl)
{
    int i;
    Node *q = NULL;
    if (pos < pl->size && pos > -1)
    {
        for (q = pl->head, i = 0; i < pos; i++)
            q = q->next;
    }
    // *pe = *((pair *)q->entry);
    *pe = q;
}
void RetrieveList_process(int pos, Process *pe, List *pl)
{
    int i;
    Node *q = NULL;
    if (pos != pl->size || pos != -1)
    {
        for (q = pl->head, i = 0; i < pos; i++)
            q = q->next;
    }
    *pe = *((Process *)q->entry);
}
// void ReplaceList(int pos, void *e, List *pl)
// {
//     int i;
//     Node *q;
//     for (q = pl->head, i = 0; i < pos; i++)
//         q = q->next;
//     q->entry = e;
// }

//--------------SEMAPHORES---------------
union Semun
{
    int val;               /* value for SETVAL */
    struct semid_ds *buf;  /* buffer for IPC_STAT & IPC_SET */
    ushort *array;         /* array for GETALL & SETALL */
    struct seminfo *__buf; /* buffer for IPC_INFO */
    void *__pad;
};
void setSemaphoreValue(int sem, int value)
{
    union Semun semun;
    semun.val = value;
    int val;
    while (val = semctl(sem, 0, SETVAL, semun) == -1 && errno == EINTR)
    {
        continue;
    }
    if (val == -1)
    {
        perror("Scheduler: Error in semctl");
        exit(-1);
    }
}
void down(int sem)
{
    struct sembuf p_op;
    // index of the semaphore, here we have 1 semaphore, then index is 0
    p_op.sem_num = 0;
    //-ve means down operation, we want to obtain the resource
    p_op.sem_op = -1;
    p_op.sem_flg = !IPC_NOWAIT;
    // param1: semaphore id
    // param2: pointer to array of operations
    // param3: number of operations inside array
    int val;
    while ((val = semop(sem, &p_op, 1)) == -1 && errno == EINTR)
    {
        continue;
    }
    // printf("------INSIDE DOWN-----\n");
    if (val == -1)
    {
        perror("Error in down()\n");
        exit(-1);
    }
}

void up(int sem)
{
    struct sembuf v_op;

    v_op.sem_num = 0;
    //+ve means up operation, we want to release the resource
    v_op.sem_op = 1;
    v_op.sem_flg = !IPC_NOWAIT;

    // if (semop(sem, &v_op, 1) == -1)
    // {
    //     perror("Error in up()");
    //     exit(-1);
    // }
    int val;
    while ((val = semop(sem, &v_op, 1)) == -1 && errno == EINTR)
    {
        continue;
    }
    if (val == -1)
    {
        perror("Error in up()\n");
        exit(-1);
    }
}

struct my_msgbuff
{
    long mtype;
    Process m_process;
};

// PRIORITY QUEUE
typedef struct Pqueue
{
    int max;  // max size of the priority queue
    int size; // current size
    Process *pr;
} PriorityQueue;

// initialize the priority queue
void createPriorityQ(PriorityQueue *q, int m)
{
    q->size = 0;
    q->max = m;
    q->pr = (Process *)malloc(sizeof(Process) * q->max);
}
// check if the Prio_Q is empty
bool isPriorityQueueEmpty(PriorityQueue *q)
{
    return q->size == 0;
}
// check if the Prio_Q is full
bool isPriorityQueueFull(PriorityQueue *q)
{
    return q->size == q->max;
}
// Function to insert a new element
// into priority queue
bool enqueue(PriorityQueue *q, Process newP)
{
    if (!isPriorityQueueFull(q))
    {
        // Insert the element
        q->pr[q->size].id = newP.id;
        q->pr[q->size].priority = newP.priority;
        q->pr[q->size].arrivalTime = newP.arrivalTime;
        q->pr[q->size].runTime = newP.runTime;
        // Increase the size
        q->size++;
        return true;
    }
    return false;
}

// Function to check the top element
int peek(PriorityQueue *q, Process *it)
{
    int highestPriority = INT_MAX;
    int ind = -1;

    // Check for the element with
    // highest priority
    for (int i = 0; i < q->size; i++)
    {
        // If priority is same choose
        // the element with the
        // highest value
        if (highestPriority == q->pr[i].priority && ind > -1 && q->pr[ind].arrivalTime > q->pr[i].arrivalTime)
        { // if 2 processes have the same priority take the first one came as the process with higher priority
            highestPriority = q->pr[i].priority;
            ind = i;
        }
        else if (highestPriority > q->pr[i].priority) // for first time
        {
            highestPriority = q->pr[i].priority;
            ind = i;
        }
    }
    if (ind != -1)
    {
        it->priority = q->pr[ind].priority;
        it->id = q->pr[ind].id;
        it->arrivalTime = q->pr[ind].arrivalTime;
        it->runTime = q->pr[ind].runTime;
    }
    // Return position of the element
    return ind;
}

// Function to remove the element with
// the highest priority
bool dequeue(PriorityQueue *q, Process *it)
{
    if (!isPriorityQueueEmpty(q))
    {
        // Find the position of the element
        // with highest priority
        int ind = peek(q, it);

        // Shift the element one index before
        // from the position of the element
        // with highest priority is found
        for (int i = ind; i < q->size; i++)
        {
            q->pr[i] = q->pr[i + 1];
        }

        // Decrease the size of the
        // priority queue by one
        q->size--;
        return true;
    }
    return false;
}

//  CIRCULAR QUEUE IMPLEMENTATION
typedef struct circQueue
{
    Process *items;
    int size;
    int front;
    int rear;
    int maxSize;
} CircularQueue;

void createCircularQueue(CircularQueue *q, int s)
{
    q->size = 0;
    q->front = -1;
    q->rear = -1;
    q->maxSize = s; // max size in the circular queue
    q->items = (Process *)malloc(q->maxSize * sizeof(Process));
}
// Check if the queue is full
bool isCircularQueueFull(CircularQueue *q)
{
    if ((q->front == q->rear + 1) || (q->front == 0 && q->rear == q->maxSize - 1))
        return true;
    return false;
}

// // Check if the queue is empty
bool isCircularQueueEmpty(CircularQueue *q)
{
    if (q->front == -1)
        return true;
    return false;
}

// Adding an element
bool enQueueCircularQueue(CircularQueue *q, Process element)
{
    printf("Enter Enqueue \n");
    if (!isCircularQueueFull(q))
    {
        if (q->front == -1)
            q->front = 0;
        q->rear = (q->rear + 1) % q->maxSize;
        q->items[q->rear].arrivalTime = element.arrivalTime;
        q->items[q->rear].id = element.id;
        q->items[q->rear].priority = element.priority;
        q->items[q->rear].runTime = element.runTime;
        q->size++;
        return true;
    }
    return false;
}

// Removing an element
bool deQueueCircularQueue(CircularQueue *q, Process *element)
{
    // int element;
    if (!isCircularQueueEmpty(q))
    {
        element->id = q->items[q->front].id;
        element->priority = q->items[q->front].priority;
        element->runTime = q->items[q->front].runTime;
        element->arrivalTime = q->items[q->front].arrivalTime;
        if (q->front == q->rear)
        {
            q->front = -1;
            q->rear = -1;
        }
        // Q has only one element, so we reset the
        // queue after dequeing it. ?
        else
        {
            q->front = (q->front + 1) % q->maxSize;
        }
        // printf("\n Deleted element -> %d \n", element->id);
        q->size--;
        return true;
    }
    return false;
}
// peek
bool peekCircularQueue(CircularQueue *q, Process *element)
{
    if (!isCircularQueueEmpty(q))
    {
        element->id = q->items[q->front].id;
        element->priority = q->items[q->front].priority;
        element->runTime = q->items[q->front].runTime;
        // printf("\n Peek element -> %d \n", element->id);
        return true;
    }
    return false;
}

// Display the queue
void displayCircularQueue(CircularQueue *q)
{
    int i;
    if (isCircularQueueEmpty(q))
        printf(" \n Empty Queue\n");
    else
    {
        printf("\n Front -> %d ", q->front);
        printf("\n Items -> ");
        for (i = q->front; i != q->rear; i = (i + 1) % q->maxSize)
        {
            printf("%d ", q->items[i].id);
        }
        printf("%d ", q->items[i].id);
        printf("\n Rear -> %d \n", q->rear);
    }
}
