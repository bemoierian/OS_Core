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
    int cumulativeTime; //  cumulative runtime of the process
    int lastClk;        // to represent the last clk the process was warking in
    int startAddres;    // start address and end address of the process
    int endAddress;
} PCB;
////////////////////
int pCount = 0;              // counter to know if all processes finished
bool cpuFree = true;         // indi    cates that the cpu is free to excute a process
Process *currentProc = NULL; // the currently running process
PCB **processTable;          // the process table of the OS
float *WTA;
float *Wait;
int sch_algo;
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
FILE *ptr, *ptrM;
// Global vector of vector of pairs to store address ranges available in free list
vector free_list; // we will use this vector as queue
// total memeory
const int totalMemo = 1024;
// queue to store the process that has no place in memo ...we will use single queue as it's better for utilization
List *waitQ;
// the queues used in scheduling depends on the type of the algorithm
PriorityQueue q1;
CircularQueue q2;
// Functions Declarations
void processTerminate(int sigID);
void WriteOutputLine(FILE *ptr, int time, int process_id, char *state, int arr, int total, int reamain, int wait, int TA, float WTA);
void WriteFinalOutput(FILE *ptr, float cpuUtil, float AvgWTA, float AvgWait, float StdWTA);
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
void WritMemoryLine(FILE *ptr, int time, int size, int proc, int start_Add, int end_Add, char *state);
bool allocate(Process);
void deallocate();
void AddToWaitQ(Process);
int main(int argc, char *argv[])
{
    // initialize the free_list of the memory
    vector_init(&free_list, 7);
    // create vector of lists indeces:
    // 0 - > 2^4 16
    // 1 - > 2^5 32
    // 2 - > 2^6 64
    // 3 - > 2^7 128
    // 4 - > 2^8 256
    // 5 - > 2^9 512
    // 6 - > 2^10 1024
    for (size_t i = 0; i < 7; i++)
    {
        List *temp = (List *)malloc(sizeof(List));
        CreateList(temp);
        vector_add(&free_list, temp);
    }
    // before allocating any process the starting address and ending address of the only of one block (0,1023)
    pair *myPair = (pair *)malloc(sizeof(pair));
    myPair->startingAdd = 0;
    myPair->size = 1024;
    InsertList_pair(myPair, vector_get(&free_list, 6));

    List *x = vector_get(&free_list, 6);
    printf("size initail %d\n ", x->size);

    ptrM = fopen("memory.log", "w");
    ptr = fopen("scheduler.log", "w");
    fprintf(ptr, "#At time x process y state arr w total z remain y wait k\n");
    fprintf(ptrM, "#At time x allocated y bytes for process z from i to j\n");

    initClk();

    signal(SIGUSR1, processTerminate); // attach the function to SIGUSR1

    sch_algo = atoi(argv[1]); // type of the algo
    int q_size = 64;
    // int q_size = atoi(argv[2]);            // max no of processes
    int Quantum = atoi(argv[2]);           // Quantum for RR
    int numberOfProcesses = atoi(argv[3]); // max no of processes
    int total_runtime = atoi(argv[4]);     // total runtime of processes
    printf("runTime Time = %d \n", total_runtime);

    // initialize the waiting queue for the processes that have no room in the memory
    waitQ = (List *)malloc(sizeof(List));
    CreateList(waitQ); // max number of processes regardless the sizes of the processes = numberOfProcesses - 1

    printf("create the waiting Q\n");
    // the process table of the OS
    processTable = (PCB **)malloc(sizeof(PCB *) * numberOfProcesses);
    WTA = (float *)malloc(sizeof(float) * numberOfProcesses);
    Wait = (float *)malloc(sizeof(float) * numberOfProcesses);
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
    printf("number of PS = %d \n", numberOfProcesses);
    switch (sch_algo)
    {
    case 1:; // HPF
        printf("Entered case 1\n");
        createPriorityQ(&q1, q_size); // create the priority Q
        while (1)                     // this replaces the while(1) by mark to make the scheduler terminate upon finishing all processes to continue after the while loop and terminate itself (process generator does not treminate schedular)
        {
            for (int i = 0; i < numberOfProcesses; i++) // to recieve all process sent at this time
            {
                rc = msgctl(msgq_id, IPC_STAT, &buf); // check if there is a comming process
                num_messages = buf.msg_qnum;
                if (num_messages > 0)
                {
                    printf("New Process Recieved \n");
                    receiveNewProcess();
                    // call allocate and check if allocated then enqueue
                    if (allocate(message_recieved.m_process))
                    {
                        printf("IN ALOCATE!!!!!!!!!!!\n");
                        enqueue(&q1, message_recieved.m_process);
                        int strtAdd = processTable[message_recieved.m_process.id - 1]->startAddres;
                        int endAdd = processTable[message_recieved.m_process.id - 1]->endAddress;
                        int s = endAdd - strtAdd + 1;
                        WritMemoryLine(ptrM, getClk(), s, message_recieved.m_process.id, strtAdd, endAdd, processTable[message_recieved.m_process.id - 1]->state);
                    }
                    else
                    {
                        AddToWaitQ(message_recieved.m_process);
                    }
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
                // printf("flag is %d ________________\n", flag);
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
                    {
                        // printf("IN START ________________\n"); // parent
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
        createPriorityQ(&q1, q_size);
        while (1)
        {
            for (size_t i = 0; i < numberOfProcesses; i++) // to recieve all process sent at this time
            {
                rc = msgctl(msgq_id, IPC_STAT, &buf); // check if there is a comming process
                num_messages = buf.msg_qnum;
                if (num_messages > 0)
                {
                    printf("New Process Recieved \n");
                    // allocate
                    receiveNewProcess();
                    message_recieved.m_process.priority = message_recieved.m_process.runTime;
                    // call allocate and check if allocated then enqueue
                    if (allocate(message_recieved.m_process))
                    {
                        printf("IN ALOCATE!!!!!!!!!!!\n");
                        enqueue(&q1, message_recieved.m_process);
                        int strtAdd = processTable[message_recieved.m_process.id - 1]->startAddres;
                        int endAdd = processTable[message_recieved.m_process.id - 1]->endAddress;
                        int s = endAdd - strtAdd + 1;
                        WritMemoryLine(ptrM, getClk(), s, message_recieved.m_process.id, strtAdd, endAdd, processTable[message_recieved.m_process.id - 1]->state);
                    }
                    else
                    {
                        AddToWaitQ(message_recieved.m_process);
                    }
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
                            // call allocate and check if allocated then enqueue
                            if (allocate(message_recieved.m_process))
                            {
                                printf("IN ALOCATE!!!!!!!!!!!\n");
                                enqueue(&q1, message_recieved.m_process);
                                int strtAdd = processTable[message_recieved.m_process.id - 1]->startAddres;
                                int endAdd = processTable[message_recieved.m_process.id - 1]->endAddress;
                                int s = endAdd - strtAdd + 1;
                                WritMemoryLine(ptrM, getClk(), s, message_recieved.m_process.id, strtAdd, endAdd, processTable[message_recieved.m_process.id - 1]->state);
                            }
                            else
                            {
                                AddToWaitQ(message_recieved.m_process);
                            }
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
        createCircularQueue(&q2, q_size);
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
                                         // call allocate and check if allocated then enqueue
                    if (allocate(message_recieved.m_process))
                    {
                        printf("IN ALOCATE!!!!!!!!!!!\n");
                        enQueueCircularQueue(&q2, message_recieved.m_process);
                        int strtAdd = processTable[message_recieved.m_process.id - 1]->startAddres;
                        int endAdd = processTable[message_recieved.m_process.id - 1]->endAddress;
                        int s = endAdd - strtAdd + 1;
                        WritMemoryLine(ptrM, getClk(), s, message_recieved.m_process.id, strtAdd, endAdd, processTable[message_recieved.m_process.id - 1]->state);
                    }
                    else
                    {
                        AddToWaitQ(message_recieved.m_process);
                    }
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
                            if (allocate(message_recieved.m_process))
                            {
                                printf("IN ALOCATE!!!!!!!!!!!\n");
                                enQueueCircularQueue(&q2, message_recieved.m_process);
                                int strtAdd = processTable[message_recieved.m_process.id - 1]->startAddres;
                                int endAdd = processTable[message_recieved.m_process.id - 1]->endAddress;
                                int s = endAdd - strtAdd + 1;
                                WritMemoryLine(ptrM, getClk(), s, message_recieved.m_process.id, strtAdd, endAdd, processTable[message_recieved.m_process.id - 1]->state);
                            }
                            else
                            {
                                AddToWaitQ(message_recieved.m_process);
                            }
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
                                if (allocate(message_recieved.m_process))
                                {
                                    printf("IN ALOCATE!!!!!!!!!!!\n");
                                    enQueueCircularQueue(&q2, message_recieved.m_process);
                                    int strtAdd = processTable[message_recieved.m_process.id - 1]->startAddres;
                                    int endAdd = processTable[message_recieved.m_process.id - 1]->endAddress;
                                    int s = endAdd - strtAdd + 1;
                                    WritMemoryLine(ptrM, getClk(), s, message_recieved.m_process.id, strtAdd, endAdd, processTable[message_recieved.m_process.id - 1]->state);
                                }
                                else
                                {
                                    AddToWaitQ(message_recieved.m_process);
                                }
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
    // printing the scheduler.pref
    ptr = fopen("scheduler.perf", "w");
    printf("finish Time = %d \n", finishTime);
    printf("runTime Time = %d \n", total_runtime);
    float CPUutilisation = ((float)total_runtime / finishTime) * 100;
    float AvgWTA = AVG(WTA, numberOfProcesses);
    float AvgWait = AVG(Wait, numberOfProcesses);
    float StdWTA = StandardDeviation(WTA, numberOfProcesses);
    WriteFinalOutput(ptr, CPUutilisation, AvgWTA, AvgWait, StdWTA);
    fclose(ptr); // close the file at the end
    // deattach shared memory
    shmdt(ps_shmaddr);
    // free wta
    free(WTA);
    free(Wait);
    // free the waiting Q
    DestroyList(waitQ);
    // free each list in the buddy algo
    for (size_t i = 0; i < 7; i++)
    {
        DestroyList(vector_get(&free_list, i));
    }
    // free the vector holdig the list
    vector_free(&free_list);
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

    deallocate(); // deallocate the currprocess from the memo
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
void WritMemoryLine(FILE *ptr, int time, int size, int proc, int start_Add, int end_Add, char *state)
{
    if (!strcmp(state, "finished"))
        fprintf(ptr, "At time %d freed %d bytes for process %d from %d to %d \n", time, size, proc, start_Add, end_Add);
    else
        fprintf(ptr, "At time %d allocated %d bytes for process %d from %d to %d \n", time, size, proc, start_Add, end_Add);
    fflush(ptr);
}
void WriteFinalOutput(FILE *ptr, float cpuUtil, float AvgWTA, float AvgWait, float StdWTA)
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
    // printf("IN STARTCURRENT^^^^^^^^^^^^^^^^^^^^^\n");
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
bool allocate(Process proces)
{
    bool isAllocated = false;
    int n = ceil(log(proces.size) / log(2)); // nearest power of 2 to the passed size
    if (n <= 4)
    {
        n = 0; // first list
    }
    else
    {
        n -= 4; // 34an azbt dal index bta3 al vector 3la al min ali na 7ato
    }
    int i = n;
    while (i < 7 && ListEmpty(vector_get(&free_list, i))) // there is no holes
    {
        i++;
    }
    if (i >= 7) // insert in waitQ if there is no holes available
    {
        isAllocated = false; // you can't add
    }
    else // there is a hole available
    {
        pair *hole = (pair *)malloc(sizeof(pair));
        if (i == n) // no dividing is needed
        {
            DeleteList_pair(0, hole, vector_get(&free_list, i));
        }
        else // need to divide
        {
            int j;
            for (j = i; j > n; j--)
            {
                printf("bene2sem!!!!!!!!!!\n");
                DeleteList_pair(0, hole, vector_get(&free_list, j));
                List *list = vector_get(&free_list, j - 1);

                pair *h1 = (pair *)malloc(sizeof(pair));
                h1->startingAdd = hole->startingAdd;
                h1->size = hole->size / 2;
                InsertList_pair(h1, list);

                pair *h2 = (pair *)malloc(sizeof(pair));
                h2->startingAdd = hole->startingAdd + hole->size / 2;
                h2->size = hole->size / 2;
                InsertList_pair(h2, list);
            }
            DeleteList_pair(0, hole, vector_get(&free_list, j));
            printf("add of hole = %d\n", hole->startingAdd);
        }
        processTable[proces.id - 1]->startAddres = hole->startingAdd;
        processTable[proces.id - 1]->endAddress = hole->startingAdd + hole->size - 1;
        isAllocated = true;
    }
    printf("isAllocated = %d\n", isAllocated);
    return isAllocated;
}
void AddToWaitQ(Process p)
{
    Process *newp = (Process *)malloc(sizeof(Process));
    newp->id = p.id;
    newp->arrivalTime = p.arrivalTime;
    newp->priority = p.priority;
    newp->runTime = p.runTime;
    newp->size = p.size;
    InsertList(waitQ->size, newp, waitQ); // insert the process in the wait queue
    printf("waitQ->size = %d\n", waitQ->size);
}
void deallocate() // no param passed as we access the pcb using the id of currProcess
{
    int sz = processTable[currentProc->id - 1]->endAddress - processTable[currentProc->id - 1]->startAddres + 1;
    int i = log2(sz); // this size reserved in os 2^n
    i -= 4;           // to map to our threshold which 16 kB
    pair *newHole = (pair *)malloc(sizeof(pair));
    newHole->startingAdd = processTable[currentProc->id - 1]->startAddres;
    newHole->size = sz;
    for (int m = i; m < 7; m++)
    {
        int j = InsertList_pair(newHole, vector_get(&free_list, m)); // add the hole to the list
        Display(vector_get(&free_list, m));
        Node *pre = NULL, *curr = NULL, *nxt = NULL;
        RetrieveList_Node(j - 1, pre, vector_get(&free_list, m));
        RetrieveList_Node(j, curr, vector_get(&free_list, m));
        RetrieveList_Node(j + 1, nxt, vector_get(&free_list, m));
        if (pre) // if there is a prev node aslun check 3laha
        {
            if ((((pair *)pre->entry)->startingAdd / sz) % 2 == 0)                        // if even
            {                                                                             // 0 1 2 3
                                                                                          //  merge
                DeleteList_pair(j - 1, ((pair *)pre->entry), vector_get(&free_list, m));  // remove pre
                DeleteList_pair(j - 1, ((pair *)curr->entry), vector_get(&free_list, m)); // remove the curr
                newHole = (pair *)malloc(sizeof(pair));                                   // don't free this pair
                newHole->startingAdd = ((pair *)pre->entry)->startingAdd;                 // the start address is the one of the pre
                sz *= 2;                                                                  // sum of 2 holes
                newHole->size = sz;

                continue; // lo 3mlt merge m3 ali abli m4 ha3ml m3 ali b3di
            }
        }
        if (nxt) // lo fe nxt node check 3laha
        {
            if ((((pair *)curr->entry)->startingAdd / sz) % 2 == 0)
            { // 0 1 2 3
                // 0  16  32 64
                //  merge
                DeleteList_pair(j, ((pair *)curr->entry), vector_get(&free_list, m)); // remove curr
                DeleteList_pair(j, ((pair *)nxt->entry), vector_get(&free_list, m));  // remove nxt
                newHole = (pair *)malloc(sizeof(pair));
                newHole->startingAdd = ((pair *)curr->entry)->startingAdd; // the start address is the one of the curr now
                sz *= 2;
                newHole->size = sz;
                continue;
            }
        }
        // lo m3rft4 a3ml merge m3 ai 7aga break
        break;
    }
    int s = processTable[currentProc->id - 1]->endAddress - processTable[currentProc->id - 1]->startAddres + 1;
    WritMemoryLine(ptrM, getClk(), s, currentProc->id, processTable[currentProc->id - 1]->startAddres, processTable[currentProc->id - 1]->endAddress, processTable[currentProc->id - 1]->state);
    Node *tempo = waitQ->head;
    int m = 0;
    Process newProc;
    while (m < waitQ->size)
    {
        RetrieveList_process(m, &newProc, waitQ);
        if (allocate(newProc))
        {
            DeleteList_process(m, &newProc, waitQ);
            m--;
            if (sch_algo < 3)
            {
                enqueue(&q1, newProc);
            }
            else
            {
                enQueueCircularQueue(&q2, newProc);
            }
            int strtAdd = processTable[newProc.id - 1]->startAddres;
            int endAdd = processTable[newProc.id - 1]->endAddress;
            int s = endAdd - strtAdd + 1;
            WritMemoryLine(ptrM, getClk(), s, newProc.id, strtAdd, endAdd, processTable[newProc.id - 1]->state);
        }
        m++;
    }
}