#include "headers.h"
#include <string.h>
void clearResources(int);

int msgq_id;
Item *processes;

int main(int argc, char *argv[])
{
    signal(SIGINT, clearResources);
    // TODO Initialization
    // 1. Read the input files.
    FILE *ptr;
    ptr = fopen("processes.txt", "r");
    int processes_number = 0;
    int total_runtime = 0;
    char a;
    while (!feof(ptr))
    {
        a = fgetc(ptr);
        if (a == '\n')
        {
            processes_number++;
        }
    }
    fclose(ptr);
    ptr = fopen("processes.txt", "r");
    processes = (Item *)malloc(processes_number * sizeof *processes);
    int k = 0;
    char str[40];
    while (fgets(str, 40, ptr) != NULL)
    {
        char *line = strtok(str, " ");
        if (line[0] == '#') // ignore comment lines
            continue;
        int x[4]; // store the values of each process
        for (int i = 0; i < 4; i++)
        {
            x[i] = atoi(line);
            line = strtok(NULL, " ");
        }
        processes[k].id = x[0];
        processes[k].arrivalTime = x[1];
        processes[k].runTime = x[2];
        processes[k].priority = x[3];
        total_runtime += processes[k].runTime;
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
        execl("clk.out", "clk", NULL);
    }
    int pid2 = fork();
    if (pid2 == 0)
    {
        char algo[2];
        char sendedSize[2] = {q_size, '\0'}; // send the max
        sprintf(algo, "%d", sch_algo);       // converts the int to string to sended in the arguments of the process
        execl("scheduler.out", "scheduler", algo, sendedSize, NULL);
    }
    // 4. Use this function after creating the clock process to initialize clock
    initClk();

    // To get time use this
    int curr_time = getClk();
    printf("current time is %d\n", curr_time);
    printf("total runtime is %d\n", total_runtime);
    // TODO Generation Main Loop
    msgq_id = msgget(MSGKEY, 0666 | IPC_CREAT); // create message queue and return id
    int send_val;
    struct my_msgbuff message_send;
    while (curr_time < total_runtime)
    {
        printf("current time : %d\n", curr_time);
        for (int i = 0; i < processes_number; i++)
        {
            // if one process come at a time : then sleep 1 sec to avoid redundent clock reading
            // 5. Create a data structure for processes and provide it with its parameters.
            // 6. Send the information to the scheduler at the appropriate time.
            if (curr_time == processes[i].arrivalTime)
            {
                // send to scheduler
                message_send.m_process = processes[i];
                message_send.mtype = 7;
                send_val = msgsnd(msgq_id, &message_send, sizeof(message_send.m_process), !IPC_NOWAIT);
                if (send_val == -1)
                    perror("Error: process_generator failed to send the input message \n");
            }
        }

        sleep(1);
        curr_time = getClk();
    }
    printf("process generator destroying clock\n");

    free(processes);
    msgctl(msgq_id, IPC_RMID, (struct msqid_ds *)0);
    // 7. Clear clock resources
    destroyClk(true);
}

void clearResources(int signum)
{
    // TODO Clears all resources in case of interruption
    free(processes);
    msgctl(msgq_id, IPC_RMID, (struct msqid_ds *)0);
    destroyClk(true);
}

// void InitiateAlgorithm(int sch_algo,int q_size)
// {
//     switch (sch_algo)
//     {
//     case 1:
//         //HPF();
//         PriorityQueue *q;
//         createPriorityQ(q, q_size);
//         break;

//     case 2:
//         //SRTN();
//         PriorityQueue *q;
//         createPriorityQ(q, q_size);
//         break;

//     case 3:
//         // RR();
//         CircularQueue *q;
//         createCircularQueue(q, q_size);
//         // q->pr[0].arrivalTime;
//         // createCircularQueue(q, s);
//         // read the file
//         // enQueueCircularQueue(q, )
//         break;

//     default:
//         break;
//     }
// }
