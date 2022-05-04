#include "headers.h"

void clearResources(int);

int msgq_id;
Process *processes;
int pid1, pid2;
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
    processes = (Process *)malloc(processes_number * sizeof *processes);
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
    // calculating total runtime (including waiting time )
    total_runtime += processes[0].arrivalTime;
    int start = processes[0].arrivalTime;
    for (int i = 1; i < processes_number; i++)
    {
        if (start + processes[i - 1].runTime < processes[i].arrivalTime)
        {
            total_runtime += processes[i].arrivalTime - (start + processes[i - 1].runTime);
        }
        start += processes[i].runTime;
    }
    fclose(ptr);
    // 2. Ask the user for the chosen scheduling algorithm and its parameters, if there are any.
    printf("Choose the scheduling algorithm\n");
    printf("Non-preemptive Highest Priority First (HPF) : 1\n");
    printf("Shortest Remaining time Next (SRTN) : 2\n");
    printf("Round Robin (RR) : 3\n");
    int sch_algo;
    scanf("%d", &sch_algo);
    int Quantum = 0;
    if (sch_algo == 3)
    {
        printf("Please enter the quatum of RR\n");
        scanf("%d", &Quantum);
    }
    printf("Choose the size of the queue\n");
    int q_size;
    scanf("%d", &q_size);
    printf("\n");
    // 3. Initiate and create the scheduler and clock processes.
    pid1 = fork(); // fork for the clk
    if (pid1 == 0)
    {
        execl("clk.out", "clk", NULL);
    }
    pid2 = fork(); // fork for the scheduler
    if (pid2 == 0)
    {
        char algo[2];
        char Q[2];
        char sendedSize[2]; // send the max of process
        char PsNumebr[2];
        char RUN[2];
        sprintf(sendedSize, "%d", q_size);
        sprintf(Q, "%d", Quantum);
        sprintf(PsNumebr, "%d", processes_number);
        sprintf(RUN, "%d", total_runtime);
        sprintf(algo, "%d", sch_algo); // converts the int to string to sended in the arguments of the process
        execl("scheduler.out", "scheduler", algo, sendedSize, Q, PsNumebr, RUN, NULL);
    }
    // 4. Use this function after creating the clock process to initialize clock
    initClk();

    // To get time use this
    int curr_time;
    printf("current time is %d\n", curr_time);
    printf("total runtime is %d\n", total_runtime);
    // TODO Generation Main Loop
    msgq_id = msgget(MSGKEY, 0666 | IPC_CREAT); // create message queue and return id
    int send_val;
    struct my_msgbuff message_send;
    int i = 0;
    while (1)
    {
        curr_time = getClk();
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
            processes[i].arrivalTime = -1;
            i++;
        }

        // sleep(1);
        if (i == processes_number)
        {
            break;
        }
    }

    free(processes);
    // DESTROY RESOURCES
    int ps_shmid = shmget(PS_SHM_KEY, 4, IPC_CREAT | 0644);
    if (ps_shmid == -1)
    {
        perror("Process_generator: error in shared memory create\n");
        exit(-1);
    }
    // attach shared memory to process
    int *ps_shmaddr = (int *)shmat(ps_shmid, (void *)0, 0);
    if ((long)ps_shmaddr == -1)
    {
        perror("Process_generator: error in attach\n");
        exit(-1);
    }
    int sem1 = semget(SEM1_KEY, 1, 0666 | IPC_CREAT);
    if (sem1 == -1)
    {
        perror("Error in create sem");
        exit(-1);
    }
    waitpid(pid2, NULL, 0); // wait for the schedular till it finishes
    msgctl(msgq_id, IPC_RMID, (struct msqid_ds *)0);
    // deattach shared memory
    shmdt(ps_shmaddr);
    // destroy shared memory
    shmctl(ps_shmid, IPC_RMID, (struct shmid_ds *)0);
    // destory semaphore
    semctl(sem1, 0, IPC_RMID, (union Semun)0);
    // 7. Clear clock resources
    printf("process generator destroying clock\n");
    destroyClk(true);
}

void clearResources(int signum)
{
    // TODO Clears all resources in case of interruption
    free(processes);
    msgctl(msgq_id, IPC_RMID, (struct msqid_ds *)0);
    destroyClk(true);
}