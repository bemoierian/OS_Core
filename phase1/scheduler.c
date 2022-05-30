#include "headers.h"

typedef struct processControlBlock
{
    pid_t ID;       // the actaul ID of the process
    char state[10]; // state of the process ready - running - finished - stopped
    int execTime;   // Burst Time / Total Runtime
    int remainingTime;
    int totWaitTime;  // Total waiting time of the process
    int responseTime; // startTime - Arrival
    int startTime;    // Start time of Running
    int priority;
    int lastClk; // to represent the last clk the process was warking in
} PCB;
////////////////////
int pCount = 0;              // counter to know if all processes finished
bool cpuFree = true;         // indicates that the cpu is free to excute a process
Process *currentProc = NULL; // the currently running process
PCB **processTable;          // the process table of the OS
float *WTA;
float *Wait;
//-----SHARED MEMORY AND SEMAPHORE VARIABLES----
int sem1;
int ps_shmid;
int *ps_shmaddr;
// ---To recieve the processes using msg Q----
struct my_msgbuff message_recieved;
struct msqid_ds buf;
int msgq_id;
// variable indicates to finish time
int finishTime;
//----pointer to the output file----
FILE *ptr;

// Functions Declarations
void processTerminate(int sigID);
void WriteOutputLine(FILE *ptr, int time, int process_id, char *state, int arr, int total, int reamain, int wait, int TA, float WTA);
void WriteFinalOutput(float cpuUtil, float AvgWTA, float AvgWait, float StdWTA);
float StandardDeviation(float data[], int size);
float AVG(float data[], int size);
void TerminateCurrentProcess();
void StopCurrentProcess();
void ResumeCurrentProcess();
void StartCurrentProcess();
void decRemainingTimeOfCurrPs();
void initSharedMemory(int *shmID, int key, int **shmAdrr);
void initSemaphore(int *semID, int key);
void setSemaphoreValue(int sem, int value);
void receiveNewProcess();
void destroyPCB(int numberOfProcesses);

int main(int argc, char *argv[])
{
    ptr = fopen("scheduler.log", "w");

    initClk();
    printf("Entered schedular\n");
    signal(SIGUSR1, processTerminate); // attach the function to SIGUSR1

    int sch_algo = atoi(argv[1]);          // type of the algo
    int Quantum = atoi(argv[2]);           // Quantum for RR
    int numberOfProcesses = atoi(argv[3]); // max no of processes
    int total_runtime = atoi(argv[4]);     // total runtime of processes
    printf("runTime Time = %d \n", total_runtime);

    // the process table of the OS
    processTable = (PCB **)malloc(sizeof(PCB *) * numberOfProcesses);
    WTA = (float *)malloc(sizeof(float) * numberOfProcesses);
    Wait = (float *)malloc(sizeof(float) * numberOfProcesses);
    // the queues used in scheduling depends on the type of the algorithm
    PriorityQueue q1;
    CircularQueue q2;
    //-----------SHARED MEMORY BETWEEN SCHEDULER AND RUNNING PROCESS-----------
    initSharedMemory(&ps_shmid, PS_SHM_KEY, &ps_shmaddr);
    //--------------------------------------------------------------------------
    //----------------SEMPAHORES------------------
    initSemaphore(&sem1, SEM1_KEY);
    //--------------------------------------------------------------------------
    // create message queue and return id
    msgq_id = msgget(MSGKEY, 0666 | IPC_CREAT);
    int rc;
    int num_messages;
    // switch case to choose the algo
    int prevClk = getClk() - 1; // variables to know if the clk changed
    int currClk;
    // bool firstTime = false; // to indicate that it's the first time to enter the algo
    printf("number of PS = %d \n", numberOfProcesses);
    switch (sch_algo)
    {
    case 1:; // HPF
        printf("Entered case 1\n");
        createPriorityQ(&q1, numberOfProcesses); // create the priority Q
        while (1)                                // this replaces the while(1) by mark to make the scheduler terminate upon finishing all processes to continue after the while loop and terminate itself (process generator does not treminate schedular)
        {
            for (int i = 0; i < numberOfProcesses; i++) // to recieve all process sent at this time
            {
                rc = msgctl(msgq_id, IPC_STAT, &buf); // check if there is a comming process
                num_messages = buf.msg_qnum;
                if (num_messages > 0)
                {
                    printf("New Process Recieved \n");
                    receiveNewProcess();
                    enqueue(&q1, message_recieved.m_process);
                }
            }
            currClk = getClk();
            if (prevClk != currClk)
            {
                printf("pCount = %d \n", pCount);
                printf("clk %d\n", getClk());
                prevClk = currClk;
                if (currentProc != NULL)
                {
                    printf("PS %d, remaining time %d\n", currentProc->id, processTable[currentProc->id - 1]->remainingTime);
                    decRemainingTimeOfCurrPs();
                    // processTable[currentProc->id - 1]->remaingTime--;
                }
            }
            if (cpuFree)
            {
                // printf("cpu free\n");
                currentProc = (Process *)malloc(sizeof(Process));
                bool flag = dequeue(&q1, currentProc);
                // printf("flag is %d \n", flag);
                if (flag)
                {
                    int pid = fork();
                    if (pid == 0) // child
                    {
                        char processRunTime[2];                              // send the runTime of each
                        sprintf(processRunTime, "%d", currentProc->runTime); // converts the int to string to sended in the arguments of the process
                        execl("process.out", "process", processRunTime, NULL);
                    }
                    else
                    {                    // parent
                        cpuFree = false; // now the cpu is busy
                        StartCurrentProcess(pid);
                        printf("PS %d, remaining time %d\n\n", currentProc->id, processTable[currentProc->id - 1]->remainingTime);
                        *ps_shmaddr = processTable[currentProc->id - 1]->remainingTime;
                    }
                }
                else
                {
                    free(currentProc);
                    currentProc = NULL;
                }
            }
            if (pCount == numberOfProcesses)
            {
                finishTime = getClk();
                break; // if all processes finished break the loop
            }
        }
        break;

    case 2:; // SRTN
        createPriorityQ(&q1, numberOfProcesses);
        while (1)
        {
            for (size_t i = 0; i < numberOfProcesses; i++) // to recieve all process sent at this time
            {
                rc = msgctl(msgq_id, IPC_STAT, &buf); // check if there is a comming process
                num_messages = buf.msg_qnum;
                if (num_messages > 0)
                {
                    printf("New Process Recieved \n");
                    receiveNewProcess();
                    message_recieved.m_process.priority = message_recieved.m_process.runTime;
                    enqueue(&q1, message_recieved.m_process);
                }
            }
            if (cpuFree)
            {
                currentProc = (Process *)malloc(sizeof(Process));
                bool flag = dequeue(&q1, currentProc);
                if (flag)
                {
                    printf("flag is %d \n", flag);
                    printf("PS %d, remaining time %d\n\n", currentProc->id, processTable[currentProc->id - 1]->remainingTime);
                    //  Set remaining time in shared memory
                    *ps_shmaddr = processTable[currentProc->id - 1]->remainingTime;
                    // Set remaining time in semaphore
                    setSemaphoreValue(sem1, processTable[currentProc->id - 1]->remainingTime - 1);

                    if (processTable[currentProc->id - 1]->ID == -1) // new process -> not forked yet
                    {
                        int pid = fork();
                        if (pid == 0) // child
                        {
                            char processRunTime[2];                              // send the runTime of each
                            sprintf(processRunTime, "%d", currentProc->runTime); // converts the int to string to sended in the arguments of the process
                            execl("process.out", "process", processRunTime, NULL);
                        }
                        else
                        { // parent
                            // processTable[currentProc->id - 1] = (PCB *)malloc(sizeof(PCB));
                            printf("we forked an new process and now it's running \n");
                            StartCurrentProcess(pid);
                        }
                    }
                    else // old process -> forked before
                    {
                        // resume
                        ResumeCurrentProcess();
                    }
                    cpuFree = false; // if we resumed the next process or started a new process the cpu is not free
                }
                else
                {
                    free(currentProc);
                    currentProc = NULL;
                }
            }

            currClk = getClk();
            if (prevClk != currClk) // each cycle
            {
                printf("%d -> %d \n", prevClk, currClk);
                prevClk = currClk;
                if (!cpuFree)
                { // if there is a running process decrement the remaining time then decide if you will stop it or not
                    // in the transition from prevClk to currClk
                    printf("PS %d, remaining time %d\n", currentProc->id, processTable[currentProc->id - 1]->remainingTime);
                    decRemainingTimeOfCurrPs();
                    currentProc->priority = processTable[currentProc->id - 1]->remainingTime;
                    printf("before down \n");
                    down(sem1); // sem to sync the remaining time between the scheduler and the process
                    printf("sem down \n");
                    // receive any comming process if any to be checked on it
                    for (size_t i = 0; i < numberOfProcesses; i++) // to recieve all process sent at this time
                    {
                        rc = msgctl(msgq_id, IPC_STAT, &buf); // check if there is a comming process
                        printf("waiting for a process \n");
                        num_messages = buf.msg_qnum;
                        if (num_messages > 0)
                        {
                            receiveNewProcess();
                            message_recieved.m_process.priority = message_recieved.m_process.runTime;
                            enqueue(&q1, message_recieved.m_process);
                            printf("New Process Recieved %d \n", message_recieved.m_process.id);
                        }
                    }
                    Process *pTemp = (Process *)malloc(sizeof(Process));
                    int ind = peek(&q1, pTemp);
                    if (ind != -1) // processTable[pTemp.id - 1].remaingTime < processTable[currentProc->id - 1]->remaingTime
                    {
                        if (currentProc != NULL && pTemp->priority < currentProc->priority)
                        {
                            printf("The peeked process with prio = %d \n", pTemp->priority);
                            cpuFree = true; // if the process is stopped ...excute another one
                            printf("Stopping, id: %d at clk %d \n", currentProc->id, getClk());
                            StopCurrentProcess(); // stop the curr process
                            printf("enqueue, id: %d\n", currentProc->id);
                            enqueue(&q1, *currentProc);

                            free(currentProc); // free currentProcess after enqueuing it

                            // if (cpuFree)
                            // if there is no currently a running process
                            // currentProc = (Process *)malloc(sizeof(Process));
                            // int flag = peek(&q1, currentProc);
                            // if (flag >= 0) // if there is a process in the Q
                            // {
                            // printf("before resume\n");
                            // if (processTable[pTemp->id - 1]->ID != -1) // if this process forked before
                            // {
                            //     currentProc = (Process *)malloc(sizeof(Process));
                            //     dequeue(&q1, currentProc);
                            //     printf("after resume\n");
                            //     //  Set remaining time in shared memory
                            //     *ps_shmaddr = processTable[currentProc->id - 1]->remainingTime;
                            //     // Set remaining time in semaphore
                            //     setSemaphoreValue(sem1, processTable[currentProc->id - 1]->remainingTime - 1);
                            //     printf("Resume, id: %d\n", currentProc->id);
                            //     ResumeCurrentProcess();
                            //     cpuFree = false; // now we are running a process started or resumed
                            //     printf(" now we are running a process resumed \n");
                            // }
                            // }
                            // else
                            // {
                            //     free(currentProc);
                            //     currentProc = NULL;
                            // }
                        }
                    }
                    free(pTemp); // free the pTemp anyway
                }
                printf("pCount = %d \n", pCount);
            }
            if (pCount == numberOfProcesses)
            {
                finishTime = getClk();
                printf("pCount break= %d \n", pCount);
                break; // if all processes finished break the loop
            }
        }
        break;

    case 3:; // RR
        printf("Q = %d\n", Quantum);
        createCircularQueue(&q2, numberOfProcesses);
        int quantumStartTime;
        while (1)
        {
            for (size_t i = 0; i < numberOfProcesses; i++) // to recieve all process sent at this time
            {
                rc = msgctl(msgq_id, IPC_STAT, &buf); // check if there is a comming process
                num_messages = buf.msg_qnum;
                if (num_messages > 0)
                {
                    printf("New Process Recieved \n");
                    receiveNewProcess(); // add the process to processTable
                    enQueueCircularQueue(&q2, message_recieved.m_process);
                    printf("new process enqueued \n");
                }
            }
            if (cpuFree)
            {
                currentProc = (Process *)malloc(sizeof(Process));
                bool flag = deQueueCircularQueue(&q2, currentProc);
                if (flag)
                {
                    // printf("entered flag to start\n");
                    // printf("PS %d, remaining time %d\n\n", currentProc->id, processTable[currentProc->id - 1]->remaingTime);
                    if (processTable[currentProc->id - 1]->ID == -1) // new Process -> not forked yet
                    {
                        // Set remaining time in shared memory
                        *ps_shmaddr = processTable[currentProc->id - 1]->remainingTime;
                        // Set remaining time in semaphore
                        setSemaphoreValue(sem1, processTable[currentProc->id - 1]->remainingTime - 1);
                        // setting the sem with remaining time -1 because we want to wait when the remaining time becomes 0 not when it becomes -1
                        // so we want to wait after decrementing it and it becomes zero
                        int pid = fork();
                        if (pid == 0) // child
                        {
                            char processRunTime[2];                              // send the runTime of each
                            sprintf(processRunTime, "%d", currentProc->runTime); // converts the int to string to sended in the arguments of the process
                            execl("process.out", "process", processRunTime, NULL);
                        }
                        else // parent
                        {
                            cpuFree = false;             // we forked an new process so cpu it's not free
                            quantumStartTime = getClk(); // if there is a new process change the quantumStart as your starting point must change
                            StartCurrentProcess(pid);
                            printf("Start, id: %d\n", currentProc->id);
                        }
                    }
                    else
                    {
                        // resume the process
                        *ps_shmaddr = processTable[currentProc->id - 1]->remainingTime;
                        // Set remaining time in semaphore
                        setSemaphoreValue(sem1, processTable[currentProc->id - 1]->remainingTime - 1);
                        printf("resume, id: %d\n", currentProc->id);
                        quantumStartTime = getClk(); // if there is a new process change the quantumStart as your starting point must change
                        ResumeCurrentProcess();      // resume
                        cpuFree = false;             // if we resumed the next process the cpu is not free
                    }
                }
                else
                {
                    free(currentProc);
                }
            }

            currClk = getClk();
            if (prevClk != currClk)
            { // if the clk changed
                printf("%d -> %d \n", prevClk, currClk);
                displayCircularQueue(&q2);
                prevClk = currClk;
                if (!cpuFree) // there is a running process -> dec its running time
                {
                    decRemainingTimeOfCurrPs(); // dec the remaining time in process Table
                    down(sem1);                 // down the sem
                    // printf("Dec remaining time %d \n", currentProc->id);
                }
                if (!cpuFree && q2.size == 0 && (getClk() == Quantum + quantumStartTime))
                {
                    // if cpu is not free and the size = 0 that means there is only one running process
                    for (size_t i = 0; i < numberOfProcesses; i++) // to recieve all process sent at this time
                    {
                        rc = msgctl(msgq_id, IPC_STAT, &buf); // check if there is a comming process
                        num_messages = buf.msg_qnum;
                        printf("number of received msgs = %ld \n", buf.msg_qnum);
                        if (num_messages > 0)
                        {
                            // printf("New Process Recieved \n");
                            receiveNewProcess(); // add the process to processTable
                            enQueueCircularQueue(&q2, message_recieved.m_process);
                            printf("new process enqueued \n");
                        }
                    }
                    if (q2.size == 0)
                    {
                        // so we need to set its start time to take the quantum
                        quantumStartTime = getClk();
                    }
                    // printf("cpu is not free and the size = 0 that means there is only one running process\n");
                }
                // this must be if condition not else if because after receiving the msgs in the previous if we shoulf check if we received more msgs
                if (!cpuFree && (getClk() == Quantum + quantumStartTime) && q2.size != 0)
                { // if the quantum passed and (there isn't only one running process) then stop and resume
                    quantumStartTime = getClk();
                    Process *pTemp = (Process *)malloc(sizeof(Process));
                    bool flag = peekCircularQueue(&q2, pTemp);
                    if (flag)
                    {
                        // here we will stop the currProcess and resume the next one so the cpu is not free
                        printf("entered flag to resume\n");
                        StopCurrentProcess(); // stop the current process
                        cpuFree = true;       // we stopped the curr process so the cpu is free
                        // receive any sent process
                        for (size_t i = 0; i < numberOfProcesses; i++) // to recieve all process sent at this time
                        {
                            rc = msgctl(msgq_id, IPC_STAT, &buf); // check if there is a comming process
                            num_messages = buf.msg_qnum;
                            printf("number of received msgs = %ld \n", buf.msg_qnum);
                            if (num_messages > 0)
                            {
                                // printf("New Process Recieved \n");
                                receiveNewProcess(); // add the process to processTable
                                enQueueCircularQueue(&q2, message_recieved.m_process);
                                printf("new process enqueued \n");
                            }
                        }
                        enQueueCircularQueue(&q2, *currentProc);   // enqueue the the stopped process to the ready queue
                        if (processTable[pTemp->id - 1]->ID != -1) // old process -> forked before
                        {
                            // Set remaining time in shared memory
                            *ps_shmaddr = processTable[pTemp->id - 1]->remainingTime;
                            // Set remaining time in semaphore
                            setSemaphoreValue(sem1, processTable[pTemp->id - 1]->remainingTime - 1);
                            printf("resume, id: %d\n", currentProc->id);
                            currentProc = (Process *)malloc(sizeof(Process));
                            deQueueCircularQueue(&q2, currentProc);
                            free(pTemp);
                            ResumeCurrentProcess(); // resume
                            cpuFree = false;        // if we resumed the next process the cpu is not free
                        }
                    }
                    else
                    {
                        free(pTemp);
                    }
                }
            }
            if (pCount == numberOfProcesses)
            {
                finishTime = getClk();
                break; // if all processes finished break the loop
            }
        }
        break;

    default:
        break;
    }

    printf("scheduler is finishing \n");
    fclose(ptr); // close the file at the end
    ptr = fopen("scheduler.perf", "w");
    printf("finish Time = %d \n", finishTime);
    printf("runTime Time = %d \n", total_runtime);
    float CPUutilisation = ((float)total_runtime / finishTime) * 100;
    float AvgWTA = AVG(WTA, numberOfProcesses);
    float AvgWait = AVG(Wait, numberOfProcesses);
    float StdWTA = StandardDeviation(WTA, numberOfProcesses);
    WriteFinalOutput(CPUutilisation, AvgWTA, AvgWait, StdWTA);
    fclose(ptr); // close the file at the end
    // deattach shared memory
    shmdt(ps_shmaddr);
    // free wta
    free(WTA);
    free(Wait);
    // free process table
    destroyPCB(numberOfProcesses);
    // upon termination release the clock resources.
    destroyClk(true);
    return 0;
}

//----------------------------------------------------Utility functions-------------------------------------------------
void processTerminate(int sigID)
{
    cpuFree = true; // if process terminates so the cpu is free
    pCount++;       // to count the finished processes
    strcpy(processTable[currentProc->id - 1]->state, "finished");

    int TA = getClk() - currentProc->arrivalTime;
    WTA[currentProc->id - 1] = (float)TA / processTable[currentProc->id - 1]->execTime; // Array of WTA of each process
    Wait[currentProc->id - 1] = processTable[currentProc->id - 1]->totWaitTime;         // Array of WTA of each process
    // processTable[currrentProc.id - 1].totWaitTime = TA - processTable[currrentProc.id - 1].execTime;
    WriteOutputLine(ptr, getClk(), currentProc->id, processTable[currentProc->id - 1]->state, currentProc->arrivalTime,
                    currentProc->runTime, processTable[currentProc->id - 1]->remainingTime, processTable[currentProc->id - 1]->totWaitTime, TA, WTA[currentProc->id - 1]);
    TerminateCurrentProcess();
    signal(SIGUSR1, processTerminate); // attach the function to SIGUSR1
}
void TerminateCurrentProcess()
{
    if (currentProc != NULL)
    {
        if (processTable[currentProc->id - 1] != NULL)
        {
            free(processTable[currentProc->id - 1]); // free the data of the process from the process table after terminating
        }
        processTable[currentProc->id - 1] = NULL;
        free(currentProc);
    }
    currentProc = NULL;
}

void WriteOutputLine(FILE *ptr, int time, int process_id, char *state, int arr, int total, int reamain, int wait, int TA, float WTA)
{
    if (!strcmp(state, "finished"))
        fprintf(ptr, "At time %d process %d %s arr %d total %d remain %d wait %d TA %d WTA %.2f\n", time, process_id, state, arr, total, reamain, wait, TA, WTA);
    else
        fprintf(ptr, "At time %d process %d %s arr %d total %d remain %d wait %d\n", time, process_id, state, arr, total, reamain, wait);
    fflush(ptr);
}

void WriteFinalOutput(float cpuUtil, float AvgWTA, float AvgWait, float StdWTA)
{
    fprintf(ptr, "CPU utilisation = %.2f %c\nAvg WTA = %.2f\nAvg Waiting = %.2f\nStd WTA = %.2f\n", cpuUtil, '%', AvgWTA, AvgWait, StdWTA);
    fflush(ptr);
}

void ResumeCurrentProcess()
{
    processTable[currentProc->id - 1]->totWaitTime += (getClk() - processTable[currentProc->id - 1]->lastClk);
    strcpy(processTable[currentProc->id - 1]->state, "resumed"); // set the state running
    WriteOutputLine(ptr, getClk(), currentProc->id, processTable[currentProc->id - 1]->state, currentProc->arrivalTime,
                    currentProc->runTime, processTable[currentProc->id - 1]->remainingTime, processTable[currentProc->id - 1]->totWaitTime, 0, 0);
    kill(processTable[currentProc->id - 1]->ID, SIGCONT);
}
void StopCurrentProcess()
{
    processTable[currentProc->id - 1]->lastClk = getClk();
    kill(processTable[currentProc->id - 1]->ID, SIGSTOP);        // to stop the running process
    strcpy(processTable[currentProc->id - 1]->state, "stopped"); // set the state running
    WriteOutputLine(ptr, getClk(), currentProc->id, processTable[currentProc->id - 1]->state, currentProc->arrivalTime,
                    currentProc->runTime, processTable[currentProc->id - 1]->remainingTime, processTable[currentProc->id - 1]->totWaitTime, 0, 0);
}
void StartCurrentProcess(int pid)
{
    strcpy(processTable[currentProc->id - 1]->state, "started"); // set the state running
    processTable[currentProc->id - 1]->ID = pid;                 // set the actual pid
    processTable[currentProc->id - 1]->startTime = getClk();     // set the start time of the process with the actual time
    processTable[currentProc->id - 1]->responseTime = processTable[currentProc->id - 1]->startTime - currentProc->arrivalTime;
    processTable[currentProc->id - 1]->totWaitTime = processTable[currentProc->id - 1]->responseTime; // initialised here and will be increased later
    // setting the response and the waiting time with the same value as it's non-preemptive
    WriteOutputLine(ptr, getClk(), currentProc->id, processTable[currentProc->id - 1]->state, currentProc->arrivalTime,
                    currentProc->runTime, processTable[currentProc->id - 1]->remainingTime, processTable[currentProc->id - 1]->totWaitTime, 0, 0);
    printf("Start, id: %d\n", currentProc->id);
}
void decRemainingTimeOfCurrPs()
{
    processTable[currentProc->id - 1]->remainingTime--;
    *ps_shmaddr = processTable[currentProc->id - 1]->remainingTime;
}
void initSharedMemory(int *shmID, int key, int **shmAdrr)
{
    *shmID = shmget(key, 4, IPC_CREAT | 0644);
    if (*shmID == -1)
    {
        perror("Scheduler: error in shared memory create\n");
        exit(-1);
    }
    // attach shared memory to process
    *shmAdrr = (int *)shmat(ps_shmid, (void *)0, 0);
    if ((long)*shmAdrr == -1)
    {
        perror("Scheduler: error in attach\n");
        exit(-1);
    }
}
void initSemaphore(int *semID, int key)
{
    *semID = semget(key, 1, 0666 | IPC_CREAT);
    if (*semID == -1)
    {
        perror("Error in create sem");
        exit(-1);
    }
}
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
void receiveNewProcess()
{
    int recv_Val = msgrcv(msgq_id, &message_recieved, sizeof(message_recieved.m_process), 7, IPC_NOWAIT);
    if (recv_Val == -1)
        perror("Error: Scheduler failed to receive  \n");
    // processTable[message_recieved.m_process.id - 1] = (PCB *)malloc(sizeof(Process));
    // printf("Proccess Scheduled Id : %d at Time : %d\n", message_recieved.m_process.id, message_recieved.m_process.arrivalTime);
    //  Process newProc = message_recieved.m_process;
    processTable[message_recieved.m_process.id - 1] = (PCB *)malloc(sizeof(PCB));
    processTable[message_recieved.m_process.id - 1]->priority = message_recieved.m_process.priority;
    processTable[message_recieved.m_process.id - 1]->execTime = message_recieved.m_process.runTime;
    processTable[message_recieved.m_process.id - 1]->ID = -1;
    // printf("Function receive \n");
    strcpy(processTable[message_recieved.m_process.id - 1]->state, "ready");
    processTable[message_recieved.m_process.id - 1]->remainingTime = message_recieved.m_process.runTime; // initail remaining Time
}

float StandardDeviation(float data[], int size)
{
    float sum = 0.0, mean, SD = 0.0;
    int i;
    for (i = 0; i < size; ++i)
    {
        sum += data[i];
    }
    mean = sum / size;
    for (i = 0; i < size; ++i)
    {
        SD += (data[i] - mean) * (data[i] - mean);
    }
    return sqrt(SD / size);
}
float AVG(float data[], int size)
{
    float sum = 0.0, mean;
    int i;
    for (i = 0; i < size; ++i)
    {
        sum += data[i];
    }
    mean = sum / size;
    return mean;
}
void destroyPCB(int numberOfProcesses)
{
    for (int i = 0; i < numberOfProcesses; i++)
    {
        if (processTable[i] != NULL)
        {
            free(processTable[i]);
        }
    }
    free(processTable);
}