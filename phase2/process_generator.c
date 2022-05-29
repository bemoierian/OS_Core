#include "headers.h"

void clearResources(int);
void GetAllResources();
int msgq_id, sem1, ps_shmid;
int *ps_shmaddr;
Process *processes;
int pid1, pid2;
int main(int argc, char *argv[])
{
    signal(SIGINT, clearResources);
    // DESTROY RESOURCES

    // TODO Initialization
    // 1. Read the input files.
    FILE *ptr;
    ptr = fopen("processes.txt", "r");
    int processes_number = 0;
    int total_runtime = 0;
    char a;
    char str[40];
    while (fgets(str, 40, ptr) != NULL)
    {
        if (str[0] >= '0' && str[0] <= '9')
        {
            processes_number++;
        }
    }
    printf("number of processes = %d\n", processes_number);
    fclose(ptr);
    processes = (Process *)malloc(processes_number * sizeof(Process));
    ptr = fopen("processes.txt", "r");
    int k = 0;
    while (fgets(str, 40, ptr) != NULL)
    {
        // printf("in while\n");
        char *line = strtok(str, "\t");
        // printf("line = %s\n", line);
        if (line[0] == '#') // ignore comment lines
            continue;
        else if (str[0] >= '0' && str[0] <= '9')
        {
            int x[5]; // store the values of each process
            for (int i = 0; i < 5; i++)
            {
                x[i] = atoi(line);
                line = strtok(NULL, "\t");
            }
            processes[k].id = x[0];
            processes[k].arrivalTime = x[1];
            processes[k].runTime = x[2];
            processes[k].priority = x[3];
            processes[k].size = x[4];
            total_runtime += processes[k].runTime;
            k++;
        }
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
        char Q[4];
        char PsNumebr[4];
        char RUN[4];
        // sprintf(sendedSize, "%d", q_size);
        sprintf(Q, "%d", Quantum);
        sprintf(PsNumebr, "%d", processes_number);
        sprintf(RUN, "%d", total_runtime);
        sprintf(algo, "%d", sch_algo); // converts the int to string to sended in the arguments of the process
        execl("scheduler.out", "scheduler", algo, Q, PsNumebr, RUN, NULL);
    }
    GetAllResources();
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
            message_send.m_process = processes[i];
            message_send.mtype = 7;
            send_val = msgsnd(msgq_id, &message_send, sizeof(message_send.m_process), !IPC_NOWAIT);

            if (send_val == -1)
                perror("Error: process_generator failed to send the input message \n");
            processes[i].arrivalTime = -1;
            i++;
        }

        if (i == processes_number)
        {
            break;
        }
    }

    free(processes);

    waitpid(pid2, NULL, 0); // wait for the schedular till it finishes
    // 7. Clear clock resources
    printf("process generator destroying clock\n");
    destroyClk(true);
    return 0;
}

void clearResources(int signum)
{
    // TODO Clears all resources in case of interruption
    msgctl(msgq_id, IPC_RMID, (struct msqid_ds *)0);
    // deattach shared memory
    shmdt(ps_shmaddr);
    // destroy shared memory
    shmctl(ps_shmid, IPC_RMID, (struct shmid_ds *)0);
    // destory semaphore
    semctl(sem1, 0, IPC_RMID, (union Semun)0);
    // Clear clock resources
    printf("process generator destroying clock\n");
    signal(SIGINT, SIG_DFL); // Clock will send SIGINT again to terminate process generator
    destroyClk(true);        // if your press ctrl+c you have to kill other processes so we need to call destroy clk
}
void GetAllResources()
{
    ps_shmid = shmget(PS_SHM_KEY, 4, IPC_CREAT | 0644);
    if (ps_shmid == -1)
    {
        perror("Process_generator: error in shared memory create\n");
        exit(-1);
    }
    // attach shared memory to process
    ps_shmaddr = (int *)shmat(ps_shmid, (void *)0, 0);
    if ((long)ps_shmaddr == -1)
    {
        perror("Process_generator: error in attach\n");
        exit(-1);
    }
    sem1 = semget(SEM1_KEY, 1, 0666 | IPC_CREAT);
    if (sem1 == -1)
    {
        perror("Error in create sem");
        exit(-1);
    }
}