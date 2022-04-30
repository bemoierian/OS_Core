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

typedef short bool;
#define true 1
#define false 0

#define SHKEY 300

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

// PRIORITY QUEUE
typedef struct item
{
    int value; // ID of the process
    int arrivalTime;
    int runTime;
    int priority;
} Item;

// max number of process in the priority queue

typedef struct Pqueue
{
    int max;  // max size of the priority queue
    int size; // current size
    Item *pr;
} PriorityQueue;

// initialize the priority queue
void createPriorityQ(PriorityQueue *q, int m)
{
    q->size = 0;
    q->max = m;
    q->pr = (Item *)malloc(sizeof(Item) * q->max);
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
void enqueue(PriorityQueue *q, int value, int priority)
{
    if (!isPriorityQueueFull(q))
    {
        // Increase the size
        q->size++;

        // Insert the element
        q->pr[q->size].value = value;
        q->pr[q->size].priority = priority;
    }
}

// Function to check the top element
int peek(PriorityQueue *q, Item *it)
{
    int highestPriority = INT_MIN;
    int ind = -1;

    // Check for the element with
    // highest priority
    for (int i = 0; i <= q->size; i++)
    {

        // If priority is same choose
        // the element with the
        // highest value
        if (highestPriority == q->pr[i].priority && ind > -1 && q->pr[ind].value < q->pr[i].value)
        {
            highestPriority = q->pr[i].priority;
            ind = i;
        }
        else if (highestPriority < q->pr[i].priority)
        {
            highestPriority = q->pr[i].priority;
            ind = i;
        }
    }
    it->priority = q->pr[ind].priority;
    it->value = q->pr[ind].value;
    // Return position of the element
    return ind;
}

// Function to remove the element with
// the highest priority
void dequeue(PriorityQueue *q, Item *it)
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
    }
}

//  CIRCULAR QUEUE IMPLEMENTATION
typedef struct circQueue
{
    int *items;
    int size;
    int front;
    int rear;
} CircularQueue;

void createCircularQueue(CircularQueue *q, int s)
{
    q->front = -1;
    q->rear = -1;
    q->size = s; // max size in the circular queue
    q->items = (int *)malloc(q->size * sizeof(int));
}
// Check if the queue is full
bool isCircularQueueFull(CircularQueue *q)
{
    if ((q->front == q->rear + 1) || (q->front == 0 && q->rear == q->size - 1))
        return true;
    return false;
}

// Check if the queue is empty
int isCircularQueueEmpty(CircularQueue *q)
{
    if (q->front == -1)
        return 1;
    return 0;
}

// Adding an element
int enQueueCircularQueue(CircularQueue *q, int element)
{
    if (isCircularQueueFull(q))
    {
        printf("\n Queue is full!! \n");
        return -1;
    }
    else
    {
        if (q->front == -1)
            q->front = 0;
        q->rear = (q->rear + 1) % q->size;
        q->items[q->rear] = element;
        printf("\n Inserted -> %d", element);
        return 0;
    }
}

// Removing an element
int deQueueCircularQueue(CircularQueue *q)
{
    int element;
    if (isCircularQueueEmpty(q))
    {
        printf("\n Queue is empty !! \n");
        return (-1);
    }
    else
    {
        element = q->items[q->front];
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
        printf("\n Deleted element -> %d \n", element);
        return (element);
    }
}
// peek
int peekCircularQueue(CircularQueue *q)
{
    int element;
    if (isCircularQueueEmpty(q))
    {
        printf("\n Peek: Queue is empty \n");
        return (-1);
    }
    else
    {
        element = q->items[q->front];
        printf("\n Peek element -> %d \n", element);
        return (element);
    }
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
        for (i = q->front; i != q->rear; i = (i + 1) % q->size)
        {
            printf("%d ", q->items[i]);
        }
        printf("%d ", q->items[i]);
        printf("\n Rear -> %d \n", q->rear);
    }
}
