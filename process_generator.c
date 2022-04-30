#include "headers.h"
#include <string.h>
void clearResources(int);

int main(int argc, char *argv[])
{
    signal(SIGINT, clearResources);
    // TODO Initialization
    // 1. Read the input files.
    
    FILE *ptr;
    ptr = fopen("processes.txt", "r");
    // 2. Ask the user for the chosen scheduling algorithm and its parameters, if there are any.
    printf("Choose the scheduling algorithm\n");
    printf("Non-preemptive Highest Priority First (HPF) : 1\n");
    printf("Shortest Remaining time Next (SRTN) : 2\n");
    printf("Round Robin (RR) : 3\n");
    int sch_algo;
    scanf("%d", &sch_algo);
    int q_size;
    scanf("%d", &q_size);
    Item* processes = malloc(q_size * sizeof * processes);
    int k = 0;
    char str[40];
    while(fgets(str,40, ptr) != NULL)
    {
        char* line = strtok(str, " ");
        if(line[0] == '#') // ignore comment lines 
            continue;
        int x[4]; //store the values of each process
        for(int i=0;i<4;i++)
        {
            int a = atoi(line);
            line = strtok(NULL, " ");
        }
        processes[k].id = x[0];  
        processes[k].arrivalTime = x[1];  
        processes[k].runTime = x[2];  
        processes[k].priority = x[3];  
        printf("\n");
    }
    fclose(ptr);
    // 3. Initiate and create the scheduler and clock processes.
    // 4. Use this function after creating the clock process to initialize clock
    initClk();

    // To get time use this
    int curr_time = getClk();
    printf("current time is %d\n", curr_time);
    // TODO Generation Main Loop
    // 5. Create a data structure for processes and provide it with its parameters.
    // 6. Send the information to the scheduler at the appropriate time.
    // 7. Clear clock resources
    destroyClk(true);
}

void clearResources(int signum)
{
    // TODO Clears all resources in case of interruption
}

void InitiateAlgorithm(int sch_algo)
{
    switch (sch_algo)
    {
    case 1:
        //HPF();
        break;

    case 2:
        //SRTN();
        break;

    case 3:
        // RR();
        // PriorityQueue *q;
        // q->pr[0].arrivalTime;
        // createCircularQueue(q, s);
        // read the file
        // enQueueCircularQueue(q, )
        break;

    default:
        break;
    }
}
