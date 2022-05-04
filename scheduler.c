#include "headers.h"

typedef struct processControlBlock
{
    pid_t ID;       // the actaul ID of the process
    char state[10]; // state of the process ready - running - finished - stopped
    int execTime;   // Burst Time / Total Runtime
    int remaingTime;
    int totWaitTime;  // Total waiting time of the process
    int responseTime; // startTime - Arrival
    int startTime;    // Start time of Running
    int priority;
    int cumulativeTime; //  cumulative runtime of the process
} PCB;
////////////////////
bool cpuFree = true;         // indicates that the cpu is free to excute a process
Process *currentProc = NULL; // the currently running process
PCB *processTable;           // the process table of the OS
float *WTA;
float *Wait;
/////////////////////
FILE *ptr; // pointer to the output file

// Functions Declarations
void processTerminate(int sigID);
void WriteOutputLine(FILE *ptr, int time, int process_id, char *state, int arr, int total, int reamain, int wait, int TA, float WTA);
//void WriteFinalOutput(float cpuUtil, float AvgWTA, float AvgWait, float StdWTA);
//float StandardDeviation(float data[], int size);
//float AVG(float data[], int size);
void TerminateCurrentProcess();
void StopCurrentProcess();
void ResumeCurrentProcess();
void StartCurrentProcess();

int main(int argc, char *argv[])
{
    ptr = fopen("scheduler.log", "w");

    initClk();
    signal(SIGUSR1, processTerminate); // attach the function to SIGUSR1

    int sch_algo = atoi(argv[1]);          // type of the algo
    int q_size = atoi(argv[2]);            // max no of processes
    int Quantum = atoi(argv[3]);           // Quantum for RR
    int numberOfProcesses = atoi(argv[4]); // max no of processes
    int total_runtime = atoi(argv[5]); // total runtime of processes
    // printf("Schedule algorithm : %d\n", sch_algo);
    // the process table of the OS
    processTable = (PCB *)malloc(sizeof(PCB) * q_size);
    WTA = (float *)malloc(sizeof(float) * numberOfProcesses);
    Wait = (float *)malloc(sizeof(float) * numberOfProcesses);
    // the queues used in scheduling depends on the type of the algorithm
    PriorityQueue q1;
    CircularQueue q2;
    //-----------SHARED MEMORY BETWEEN SCHEDULER AND RUNNING PROCESS-----------
    int ps_shmid = shmget(PS_SHM_KEY, 4, IPC_CREAT | 0644);
    if (ps_shmid == -1)
    {
        perror("Scheduler: error in shared memory create\n");
        exit(-1);
    }
    // attach shared memory to process
    int *ps_shmaddr = (int *)shmat(ps_shmid, (void *)0, 0);
    if ((long)ps_shmaddr == -1)
    {
        perror("Scheduler: error in attach\n");
        exit(-1);
    }
    //--------------------------------------------------------------------------
    //----------------SEMPAHORES------------------
    union Semun semun;
    int sem1 = semget(SEM1_KEY, 1, 0666 | IPC_CREAT);
    if (sem1 == -1)
    {
        perror("Error in create sem");
        exit(-1);
    }
    //--------------------------------------------------------------------------
    // switch case to choose the algo
    switch (sch_algo)
    {
    case 1:
        // HPF();
        // q1 = (PriorityQueue *)malloc(sizeof(PriorityQueue));
        createPriorityQ(&q1, q_size);
        break;

    case 2:
        // SRTN();
        // q1 = (PriorityQueue *)malloc(sizeof(PriorityQueue));
        createPriorityQ(&q1, q_size);
        break;

    case 3:
        // RR();
        // q2 = (CircularQueue *)malloc(sizeof(CircularQueue));
        createCircularQueue(&q2, q_size);
        break;

    default:
        break;
    }
    // create message queue and return id
    int msgq_id = msgget(MSGKEY, 0666 | IPC_CREAT);
    // buffer to recieve the processes
    struct my_msgbuff message_recieved;
    struct msqid_ds buf;
    int num_messages;
    int rc;
    int prevClk = getClk() - 1;
    // to indicate that circular queue received a new process for the first time after being empty
    // so that we enter round robin without waiting for the quantum
    bool firstTime = false;
    int prevArr = 0;
    while (getClk() < total_runtime + 1) // this replaces the while(1) by mark to make the scheduler terminate upon finishing all processes to continue after the while loop and terminate itself (process generator does not treminate schedular)
    {
        rc = msgctl(msgq_id, IPC_STAT, &buf); // check if there is a comming process
        num_messages = buf.msg_qnum;
        if (num_messages > 0)
        {
            int recv_Val = msgrcv(msgq_id, &message_recieved, sizeof(message_recieved.m_process), 7, IPC_NOWAIT);
            if (recv_Val == -1)
                perror("Error: Scheduler failed to receive  \n");

            // printf("Proccess Scheduled Id : %d at Time : %d\n", message_recieved.m_process.id, message_recieved.m_process.arrivalTime);
            //  Process newProc = message_recieved.m_process;
            processTable[message_recieved.m_process.id - 1].priority = message_recieved.m_process.priority;
            processTable[message_recieved.m_process.id - 1].execTime = message_recieved.m_process.runTime;
            processTable[message_recieved.m_process.id - 1].ID = -1;

            strcpy(processTable[message_recieved.m_process.id - 1].state, "ready");
            processTable[message_recieved.m_process.id - 1].remaingTime = message_recieved.m_process.runTime; // initail remaining Time

            // enqueue the new process
            if (sch_algo == 1)
            { // for HPF
                printf("enqueued at time %d  proc id = %d \n", getClk(), message_recieved.m_process.id);
                enqueue(&q1, message_recieved.m_process);
            }
            else if (sch_algo == 2) // SRTF
            {
                printf("enqueued at time %d  proc id = %d \n", getClk(), message_recieved.m_process.id);
                if (isPriorityQueueEmpty(&q1))
                {
                    firstTime = true; // to know if this process is the first process entered the Q
                }
                message_recieved.m_process.priority = message_recieved.m_process.runTime;
                // set the priority to the run time as a initial value because in SRTN the priority is the remaining time which is initally equal the run time
                enqueue(&q1, message_recieved.m_process);
            }
            else
            {
                printf("enqueued at time %d\n", getClk());
                if (isCircularQueueEmpty(&q2))
                {
                    firstTime = true;
                }
                enQueueCircularQueue(&q2, &message_recieved.m_process);
            }
            // initialise prevArr with the value of the first process
            if (prevArr == 0)
            {
                prevArr = message_recieved.m_process.arrivalTime;
            }
            // keep receiving processes that arrived at the same time to fill the queue
            // before scheduling
            if (prevArr == message_recieved.m_process.arrivalTime)
            {
                prevArr = message_recieved.m_process.arrivalTime;
                continue;
            }
        }
        // TODO implement the scheduler :)

        int currClk;
        switch (sch_algo)
        {
        case 1:;
            // Non-preemptive Highest Priority First (HPF)
            currClk = getClk();
            if (prevClk != currClk)
            {
                prevClk = currClk;
                if (currentProc != NULL)
                {
                    printf("PS %d, remaining time %d\n", currentProc->id, processTable[currentProc->id - 1].remaingTime);
                    *ps_shmaddr = --processTable[currentProc->id - 1].remaingTime;
                    // processTable[currentProc->id - 1].remaingTime--;
                }
            }
            if (cpuFree)
            {
                currentProc = (Process *)malloc(sizeof(Process));
                bool flag = dequeue(&q1, currentProc);
                // printf("flag is %d \n", flag);
                if (flag)
                {
                    printf("PS %d, remaining time %d\n\n", currentProc->id, processTable[currentProc->id - 1].remaingTime);
                    *ps_shmaddr = processTable[currentProc->id - 1].remaingTime;
                    int pid = fork();
                    if (pid == 0) // child
                    {
                        char processRunTime[2];                              // send the runTime of each
                        sprintf(processRunTime, "%d", currentProc->runTime); // converts the int to string to sended in the arguments of the process
                        execl("process.out", "process", processRunTime, NULL);
                    }
                    else
                    {                                                               // parent
                        cpuFree = false;                                            // now the cpu is busy
                        StartCurrentProcess(pid);
                    }
                }
            }

            break;
        case 2:;
            // SRTF
            currClk = getClk();
            if (prevClk != currClk || firstTime)
            {
                printf("\n\nEntered clk %d\n", getClk());
                if (currentProc != NULL) // if there is a running process (which means first time there is no a running process)
                {
                    Process pTemp;
                    if (peek(&q1, &pTemp) != -1 && processTable[pTemp.id - 1].remaingTime < processTable[currentProc->id - 1].remaingTime)
                    {
                        cpuFree = true; // if the signal is stopped ...excute another one
                        //printf("Stopping, id: %d\n", currentProc->priority);
                        StopCurrentProcess();
                        // processTable[currrentProc.id - 1].cumulativeTime += Quantum;
                        printf("enqueue, id: %d\n", currentProc->id);
                        enqueue(&q1, *currentProc);
                        free(currentProc); // free currentProcess after enqueuing it
                    }
                    else
                    { // Handling Remaining time
                        printf("PS %d, remaining time %d\n", currentProc->id, processTable[currentProc->id - 1].remaingTime);
                        processTable[currentProc->id - 1].remaingTime--;
                        *ps_shmaddr = processTable[currentProc->id - 1].remaingTime;
                        currentProc->priority = processTable[currentProc->id - 1].remaingTime;
                        down(sem1); // sem to sych the remaining time between the scheduler and the process
                    }
                }
                
                prevClk = currClk;
                if (cpuFree)
                {   // if there is no currently a running process
                    currentProc = (Process *)malloc(sizeof(Process));
                    // Mark : the problem here is that at time 3 process 1 started before process 3 was enqueued so we need to wait until process 3 is enqueued 
                    bool flag = dequeue(&q1, currentProc);
                    if (flag) // if there is a process in the Q
                    {
                         printf("dequeue, id: %d\n", currentProc->id);
                        //printf("entered flag\n");
                        // cpuFree = false; //??
                        //printf("PS %d, remaining time %d\n\n", currentProc->id, processTable[currentProc->id - 1].remaingTime);
                        // Set remaining time in shared memory
                        *ps_shmaddr = processTable[currentProc->id - 1].remaingTime;
                        // Set remaining time in semaphore
                        semun.val = processTable[currentProc->id - 1].remaingTime - 1;
                        if (semctl(sem1, 0, SETVAL, semun) == -1)
                        {
                            perror("Scheduler: Error in semctl");
                            exit(-1);
                        }
                        if (processTable[currentProc->id - 1].ID == -1) // if this process isn't forked yet
                        {
                            int pid = fork();
                            if (pid == 0) // child
                            {
                                char processRunTime[2];                              // send the runTime of each
                                sprintf(processRunTime, "%d", currentProc->runTime); // converts the int to string to sended in the arguments of the process
                                execl("process.out", "process", processRunTime, NULL);
                            }
                            else
                            { // Parent
                                printf("Start, id: %d\n", currentProc->id);
                                StartCurrentProcess(pid);
                            }
                        }
                        else // if the process has been in the queue before
                        {
                            printf("Resume, id: %d\n", currentProc->id);
                            ResumeCurrentProcess();
                        }
                        cpuFree = false; // now we are running a process
                    }
                    else
                    {
                        free(currentProc);
                        currentProc = NULL;
                    }
                }
                firstTime = false;
            }
            break;
        case 3:;
            int quantumStartTime;
            // bool currProcessFinished = false;
            currClk = getClk();
            if (prevClk != currClk || firstTime)
            {
                if (firstTime)
                {
                    quantumStartTime = getClk();
                }

                printf("\n\nEntered clk %d\n", getClk());
                prevClk = currClk;
                firstTime = false;
                if (currentProc != NULL)
                {
                    printf("PS %d, remaining time %d\n", currentProc->id, processTable[currentProc->id - 1].remaingTime);
                    processTable[currentProc->id - 1].remaingTime--;
                    *ps_shmaddr = processTable[currentProc->id - 1].remaingTime;
                    // sem =remaining
                    down(sem1);
                }

                if (getClk() == Quantum + quantumStartTime || currentProc == NULL)
                {
                    quantumStartTime = getClk();
                    printf("entered mod\n");
                    // currProcessFinished = false;
                    if(isCircularQueueEmpty(&q2)) // this was added by mark to solve the problem of stop-resume-stop-resume
                        continue;
                    if (currentProc != NULL) // if there is a running process (first time no process is running)
                    {
                        printf("Stopping, id: %d\n", currentProc->priority);
                        StopCurrentProcess();
                        // processTable[currrentProc.id - 1].cumulativeTime += Quantum;
                        printf("enqueue, id: %d\n", currentProc->id);
                        enQueueCircularQueue(&q2, currentProc);
                        free(currentProc);
                    }
                    currentProc = (Process *)malloc(sizeof(Process));
                    bool flag = deQueueCircularQueue(&q2, currentProc);
                    if (flag)
                    {
                        printf("entered flag\n");
                        cpuFree = false;
                        printf("PS %d, remaining time %d\n\n", currentProc->id, processTable[currentProc->id - 1].remaingTime);
                        // Set remaining time in shared memory
                        *ps_shmaddr = processTable[currentProc->id - 1].remaingTime;
                        // Set remaining time in semaphore
                        semun.val = processTable[currentProc->id - 1].remaingTime - 1;
                        if (semctl(sem1, 0, SETVAL, semun) == -1)
                        {
                            perror("Scheduler: Error in semctl");
                            exit(-1);
                        }
                        if (processTable[currentProc->id - 1].ID == -1)
                        {
                            int pid = fork();
                            if (pid == 0) // child
                            {
                                char processRunTime[2];                              // send the runTime of each
                                sprintf(processRunTime, "%d", currentProc->runTime); // converts the int to string to sended in the arguments of the process
                                execl("process.out", "process", processRunTime, NULL);
                            }
                            else
                            {
                                printf("Start, id: %d\n", currentProc->id);
                                StartCurrentProcess(pid);
                            }
                        }
                        else
                        {
                            printf("resume, id: %d\n", currentProc->id);
                            ResumeCurrentProcess();
                        }
                    }
                    else
                    {
                        free(currentProc);
                        currentProc = NULL;
                    }
                }
            }
            break;
        default:
            break;
        }
    }
    printf("scheduler is finishing");
    fclose(ptr); // close the file at the end
    ptr = fopen("scheduler.perf", "w");
    float CPUutilisation = 1;
    //float AvgWTA = AVG(WTA,numberOfProcesses);
    //float AvgWait = AVG(Wait,numberOfProcesses);
    //float StdWTA = StandardDeviation(WTA,numberOfProcesses);
    //WriteFinalOutput(CPUutilisation,AvgWTA,AvgWait,StdWTA);
    fclose(ptr); // close the file at the end
    // deattach shared memory
    shmdt(ps_shmaddr);
    // destroy shared memory
    shmctl(ps_shmid, IPC_RMID, (struct shmid_ds *)0);
    // destory semaphore
    semctl(sem1, 0, IPC_RMID, (union Semun)0);
    // upon termination release the clock resources.
    destroyClk(true);
    return 0;
}









//----------------------------------------------------Utility functions-------------------------------------------------
void processTerminate(int sigID)
{
    cpuFree = true;
    strcpy(processTable[currentProc->id - 1].state, "finished");

    int TA = getClk() - processTable[currentProc->id - 1].startTime;
    WTA[currentProc->id - 1] = (float)TA / processTable[currentProc->id - 1].execTime; // Array of WTA of each process
    Wait[currentProc->id - 1] = processTable[currentProc->id - 1].totWaitTime; // Array of WTA of each process
    // processTable[currrentProc.id - 1].totWaitTime = TA - processTable[currrentProc.id - 1].execTime;
    WriteOutputLine(ptr, getClk(), currentProc->id, processTable[currentProc->id - 1].state, currentProc->arrivalTime,
                    currentProc->runTime, processTable[currentProc->id - 1].remaingTime, processTable[currentProc->id - 1].totWaitTime, TA, WTA[currentProc->id - 1]);
    TerminateCurrentProcess();
    signal(SIGUSR1, processTerminate); // attach the function to SIGUSR1
}
void TerminateCurrentProcess()
{
    if (currentProc != NULL)
    {
        free(currentProc);
    }
    currentProc = NULL;
}

void WriteOutputLine(FILE *ptr, int time, int process_id, char *state, int arr, int total, int reamain, int wait, int TA, float WTA)
{
    if (!strcmp(state, "finished"))
        fprintf(ptr, "At time %d process %d %s arr %d total %d remain %d wait %d TA %d WTA %f\n", time, process_id, state, arr, total, reamain, wait, TA, WTA);
    else
        fprintf(ptr, "At time %d process %d %s arr %d total %d remain %d wait %d\n", time, process_id, state, arr, total, reamain, wait);
    fflush(ptr);
}

void WriteFinalOutput(float cpuUtil, float AvgWTA, float AvgWait, float StdWTA)
{
    fprintf(ptr, "CPU utilisation = %f %c\nAvg WTA = = %f\nAvg Waiting = %f\nStd WTA = %f\n", cpuUtil*100,'%', AvgWTA, AvgWait, StdWTA);
    fflush(ptr);
}

void ResumeCurrentProcess()
{
    strcpy(processTable[currentProc->id - 1].state, "resumed"); // set the state running
    WriteOutputLine(ptr, getClk(), currentProc->id, processTable[currentProc->id - 1].state, currentProc->arrivalTime,
                    currentProc->runTime, processTable[currentProc->id - 1].remaingTime, processTable[currentProc->id - 1].totWaitTime, 0, 0);
    kill(processTable[currentProc->id - 1].ID, SIGCONT);
}
void StopCurrentProcess()
{
    kill(processTable[currentProc->id - 1].ID, SIGSTOP);        // to stop the running process
    strcpy(processTable[currentProc->id - 1].state, "stopped"); // set the state running
    WriteOutputLine(ptr, getClk(), currentProc->id, processTable[currentProc->id - 1].state, currentProc->arrivalTime,
                    currentProc->runTime, processTable[currentProc->id - 1].remaingTime, processTable[currentProc->id - 1].totWaitTime, 0, 0);
}
void StartCurrentProcess(int pid)
{
    strcpy(processTable[currentProc->id - 1].state, "started"); // set the state running
    processTable[currentProc->id - 1].ID = pid;                 // set the actual pid
    processTable[currentProc->id - 1].startTime = getClk();     // set the start time of the process with the actual time
    processTable[currentProc->id - 1].responseTime = processTable[currentProc->id - 1].startTime - currentProc->arrivalTime;
    processTable[currentProc->id - 1].totWaitTime = processTable[currentProc->id - 1].responseTime; // initialised here and will be increased later
    // setting the response and the waiting time with the same value as it's non-preemptive
    WriteOutputLine(ptr, getClk(), currentProc->id, processTable[currentProc->id - 1].state, currentProc->arrivalTime,
                    currentProc->runTime, processTable[currentProc->id - 1].remaingTime, processTable[currentProc->id - 1].totWaitTime, 0, 0);
}


// float StandardDeviation(float data[], int size)
// {
//     float sum = 0.0, mean, SD = 0.0;
//     int i;
//     for (i = 0; i < size; ++i)
//     {
//         sum += data[i];
//     }
//     mean = sum / size;
//     for (i = 0; i < size; ++i)
//     {
//         SD += (data[i] - mean)*(data[i] - mean);
//     }
//     return sqrt(SD / size);
// }
// float AVG(float data[], int size)
// {
//     float sum = 0.0, mean;
//     int i;
//     for (i = 0; i < size; ++i)
//     {
//         sum += data[i];
//     }
//     mean = sum / size;
//     return mean;
// }