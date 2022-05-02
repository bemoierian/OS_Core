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

typedef short bool;
#define true 1
#define false 0

#define SHKEY 300
#define MSGKEY 301

///==============================
// don't mess with this variable//
int *shmaddr; //
//===============================

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

/*
 * All process call this function at the end to release the communication
 * resources between them and the clock module.
 * Again, Remember that the clock is only emulation!
 * Input: terminateAll: a flag to indicate whether that this is the end of simulation.
 *                      It terminates the whole system and releases resources.
 */

void destroyClk(bool terminateAll)
{
    shmdt(shmaddr);
    if (terminateAll)
    {
        killpg(getpgrp(), SIGINT);
    }
}
// struct holds the process data
typedef struct processData
{
    int id; // ID of the process
    int arrivalTime;
    int runTime;
    int priority;
} Process;

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
    return q->size == -1;
}
// check if the Prio_Q is full
bool isPriorityQueueFull(PriorityQueue *q)
{
    return q->size == q->max - 1;
}
// Function to insert a new element
// into priority queue
void enqueue(PriorityQueue *q, Process newP)
{
    if (!isPriorityQueueFull(q))
    {
        // Increase the size
        q->size++;

        // Insert the element
        q->pr[q->size].id = newP.id;
        q->pr[q->size].priority = newP.priority;
        q->pr[q->size].arrivalTime = newP.arrivalTime;
        q->pr[q->size].runTime = newP.runTime;
    }
}

// Function to check the top element
int peek(PriorityQueue *q, Process *it)
{
    it = (Process *)malloc(sizeof(Process) * 1);
    int highestPriority = INT_MAX;
    int ind = -1;

    // Check for the element with
    // highest priority
    for (int i = 0; i <= q->size; i++)
    {

        // If priority is same choose
        // the element with the
        // highest value
        if (highestPriority == q->pr[i].priority && ind > -1 && q->pr[ind].arrivalTime > q->pr[i].arrivalTime)
        {
            highestPriority = q->pr[i].priority;
            ind = i;
        }
        else if (highestPriority > q->pr[i].priority)
        {
            highestPriority = q->pr[i].priority;
            ind = i;
        }
    }
    it->priority = q->pr[ind].priority;
    it->id = q->pr[ind].id;
    it->arrivalTime = q->pr[ind].arrivalTime;
    it->runTime = q->pr[ind].runTime;
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
} CircularQueue;

void createCircularQueue(CircularQueue *q, int s)
{
    q->front = -1;
    q->rear = -1;
    q->size = s; // max size in the circular queue
    q->items = (Process *)malloc(q->size * sizeof(Process));
}
// Check if the queue is full
bool isCircularQueueFull(CircularQueue *q)
{
    if ((q->front == q->rear + 1) || (q->front == 0 && q->rear == q->size - 1))
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
void enQueueCircularQueue(CircularQueue *q, Process element)
{
    if (!isCircularQueueFull(q))
    {
        if (q->front == -1)
            q->front = 0;
        q->rear = (q->rear + 1) % q->size;
        q->items[q->rear] = element;
        // printf("\n Inserted process ID -> %d", element.id);
    }
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
        if (q->front == q->rear)
        {
            q->front = -1;
            q->rear = -1;
        }
        // Q has only one element, so we reset the
        // queue after dequeing it. ?
        else
        {
            q->front = (q->front + 1) % q->size;
        }
        // printf("\n Deleted element -> %d \n", element->id);
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
// void displayCircularQueue(CircularQueue *q)
// {
//     int i;
//     if (isCircularQueueEmpty(q))
//         printf(" \n Empty Queue\n");
//     else
//     {
//         printf("\n Front -> %d ", q->front);
//         printf("\n Items -> ");
//         for (i = q->front; i != q->rear; i = (i + 1) % q->size)
//         {
//             printf("%d ", q->items[i].id);
//         }
//         printf("%d ", q->items[i]);
//         printf("\n Rear -> %d \n", q->rear);
//     }
// }
