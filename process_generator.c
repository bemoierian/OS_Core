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
    int processes_number=0;
    int total_runtime = 0;
    char a ;
    while(!feof(ptr))
    {
        a = fgetc(ptr);
        if(a == '\n')
        {
            processes_number++;
        }
    }
    fclose(ptr);
    ptr = fopen("processes.txt", "r");
    Item* processes = malloc(processes_number * sizeof * processes);  
    int k = 0;
    char* str = (char*)malloc(40);
    while(fgets(str,40, ptr) != NULL)
    {
        char* line = strtok(str, " ");
        if(line[0] == '#') // ignore comment lines 
            continue;
        int x[4]; //store the values of each process
        for(int i=0;i<4;i++)
        {
            x[i] = atoi(line);
            line = strtok(NULL, " ");
        }
        processes[k].id = x[0];       
        processes[k].arrivalTime = x[1];  
        processes[k].runTime = x[2];  
        processes[k].priority = x[3];
        total_runtime +=  processes[k].runTime;    
        k++;
    }
    fclose(ptr);
    // 2. Ask the user for the chosen scheduling algorithm and its parameters, if there are any.
    printf("Choose the scheduling algorithm\n");
    printf("Non-preemptive Highest Priority First (HPF) : 1\n");
    printf("Shortest Remaining time Next (SRTN) : 2\n");
    printf("Round Robin (RR) : 3\n");
    int sch_algo;
    scanf("%d", &sch_algo);
    printf("Choose the size of the queue\n");
    int q_size;
    scanf("%d", &q_size);
    printf("\n");
    // 3. Initiate and create the scheduler and clock processes.
    int pid1 = fork();
    if (pid1 == 0)
    {
        execl("clk.out","clk", NULL);
    }
    int pid2 = fork();
    if (pid2 == 0)
    {
        char algo[2];
        sprintf(algo, "%d", sch_algo);  
        execl("scheduler.out","scheduler",algo, NULL);
    }
    // 4. Use this function after creating the clock process to initialize clock
    initClk();

    // To get time use this
    int curr_time = getClk();
    printf("current time is %d\n", curr_time);
    printf("total runtime is %d\n", total_runtime);
    //TODO Generation Main Loop
    while (curr_time < total_runtime)
    {
        printf("current time : %d\n",curr_time);
        for (int i = 0; i < processes_number; i++)
        {
            if(curr_time == processes[i].arrivalTime)
            {
                //send to scheduler
                printf("Proccess Scheduled %d at Time : %d\n",processes[i].id,curr_time);
            }
        }
        // if one process come at a time : then sleep 1 sec to avoid redundent clock reading
        // 5. Create a data structure for processes and provide it with its parameters.
        // 6. Send the information to the scheduler at the appropriate time.
        // 7. Clear clock resources
        sleep(1);
        curr_time = getClk();
    }
    printf("process generator destroying clock\n");
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
