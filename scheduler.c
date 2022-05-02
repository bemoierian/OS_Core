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
} PCB;
////////////////////
bool cpuFree = true;  // indicates that the cpu is free to excute a process
Process currrentProc; // the currently running process
PCB *processTable;    // the process table of the OS
/////////////////////
FILE *ptr; // pointer to the output file
// Functions Declarations
void processTerminate(int sigID);
void WriteOutputLine(FILE *ptr, int time, int process_id, char *state, int arr, int total, int reamain, int wait);

int main(int argc, char *argv[])
{
    ptr = fopen("scheduler.log", "w");
    initClk();
    signal(SIGUSR1, processTerminate); // attach the function to SIGUSR1

    int sch_algo = atoi(argv[1]); // type of the algo
    int q_size = atoi(argv[2]);   // max no of processes
    // printf("Schedule algorithm : %d\n", sch_algo);

    // the process table of the OS
    processTable = (PCB *)malloc(sizeof(PCB) * q_size);

    // the queues used in scheduling depends on the type of the algorithm
    PriorityQueue *q1;
    CircularQueue *q2;

    // switch case to choose the algo
    switch (sch_algo)
    {
    case 1:
        // HPF();
        q1 = (PriorityQueue *)malloc(sizeof(PriorityQueue));
        createPriorityQ(q1, q_size);
        printf("The Q created\n");
        break;

    case 2:
        // SRTN();
        q1 = (PriorityQueue *)malloc(sizeof(PriorityQueue));
        createPriorityQ(q1, q_size);
        break;

    case 3:
        // RR();
        q2 = (CircularQueue *)malloc(sizeof(CircularQueue));
        createCircularQueue(q2, q_size);
        break;

    default:
        break;
    }
    int msgq_id = msgget(MSGKEY, 0666 | IPC_CREAT); // create message queue and return id
    struct my_msgbuff message_recieved;             // buffer to recieve the processes
    struct msqid_ds buf;
    int num_messages;
    int rc;
    while (1)
    {
        rc = msgctl(msgq_id, IPC_STAT, &buf); // check if there is a comming process
        num_messages = buf.msg_qnum;
        if (num_messages > 0)
        {
            int recv_Val = msgrcv(msgq_id, &message_recieved, sizeof(message_recieved.m_process), 7, IPC_NOWAIT);
            if (recv_Val == -1)
                perror("Error: Scheduler failed to receive  \n");
            printf("Proccess Scheduled Id : %d at Time : %d\n", message_recieved.m_process.id, message_recieved.m_process.arrivalTime);

            Process newProc = message_recieved.m_process;
            processTable[newProc.id - 1].priority = newProc.priority;
            processTable[newProc.id - 1].execTime = newProc.runTime;
            strcpy(processTable[newProc.id - 1].state, "ready");
            processTable[newProc.id - 1].remaingTime = newProc.runTime; // initail remaining Time

            // enqueue the new process
            if (sch_algo < 3)
            { // for HPF and SRTF
                enqueue(q1, message_recieved.m_process);
            }
            else
            {
                enQueueCircularQueue(q2, message_recieved.m_process);
            }
        }
        // TODO implement the scheduler :)
        if (cpuFree)
        {
            switch (sch_algo)
            {
            case 1:
                // Non-preemptive Highest Priority First (HPF)

                bool flag = dequeue(q1, &currrentProc);
                if (flag)
                {
                    int pid = fork();
                    if (pid == 0) // child
                    {
                        char processRunTime[2];                              // send the runTime of each
                        sprintf(processRunTime, "%d", currrentProc.runTime); // converts the int to string to sended in the arguments of the process
                        execl("process.out", "process", processRunTime, NULL);
                    }
                    else
                    {                                                               // parent
                        cpuFree = false;                                            // now the cpu is busy
                        strcpy(processTable[currrentProc.id - 1].state, "running"); // set the state running
                        processTable[currrentProc.id - 1].ID = pid;                 // set the actual pid
                        processTable[currrentProc.id - 1].startTime = getClk();     // set the start time of the process with the actual time
                        processTable[currrentProc.id - 1].responseTime = processTable[currrentProc.id - 1].startTime - currrentProc.arrivalTime;
                        processTable[currrentProc.id - 1].totWaitTime = processTable[currrentProc.id - 1].responseTime;
                        // setting the response and the waiting time with the same value as it's non-preemptive
                        WriteOutputLine(ptr, getClk(), currrentProc.id, processTable[currrentProc.id - 1].state, currrentProc.arrivalTime,
                                        currrentProc.runTime, processTable[currrentProc.id - 1].remaingTime, processTable[currrentProc.id - 1].totWaitTime);
                    }
                }
                break;
            case 2:
                // SRTF
                break;
            case 3:
                break;
            default:
                break;
            }
        }
    }
    fclose(ptr); // close the file at the end
    // upon termination release the clock resources.
    destroyClk(true);
}

void processTerminate(int sigID)
{
    cpuFree = true;
    strcpy(processTable[currrentProc.id - 1].state, "finished");
    processTable[currrentProc.id - 1].remaingTime = 0; // remaining time = 0
    WriteOutputLine(ptr, getClk(), currrentProc.id, processTable[currrentProc.id - 1].state, currrentProc.arrivalTime,
                    currrentProc.runTime, processTable[currrentProc.id - 1].remaingTime, processTable[currrentProc.id - 1].totWaitTime);
    signal(SIGUSR1, processTerminate); // attach the function to SIGUSR1
}
void WriteOutputLine(FILE *ptr, int time, int process_id, char *state, int arr, int total, int reamain, int wait)
{
    fprintf(ptr, "At time %d process %d %s arr %d total %d remain %d wait %d\n", time, process_id, state, arr, total, reamain, wait);
}