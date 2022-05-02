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
    int cumulativeTime;  //  cumulative runtime of the process
} PCB;
////////////////////
bool cpuFree = true;  // indicates that the cpu is free to excute a process
Process currrentProc; // the currently running process
PCB *processTable;    // the process table of the OS
/////////////////////
FILE *ptr; // pointer to the output file

// Functions Declarations
void processTerminate(int sigID);
void WriteOutputLine(FILE *ptr, int time, int process_id, char *state, int arr, int total, int reamain, int wait,int TA,float WTA);

int main(int argc, char *argv[])
{
    ptr = fopen("scheduler.log", "w");

    initClk();
    signal(SIGUSR1, processTerminate); // attach the function to SIGUSR1

    int sch_algo = atoi(argv[1]); // type of the algo
    int q_size = atoi(argv[2]);   // max no of processes
    int Quantum = atoi(argv[3]);   // max no of processes
    // printf("Schedule algorithm : %d\n", sch_algo);

    // the process table of the OS
    processTable = (PCB *)malloc(sizeof(PCB) * q_size);

    // the queues used in scheduling depends on the type of the algorithm
    PriorityQueue q1;
    CircularQueue q2;

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
    int msgq_id = msgget(MSGKEY, 0666 | IPC_CREAT); // create message queue and return id
    struct my_msgbuff message_recieved;             // buffer to recieve the processes
    struct msqid_ds buf;
    int num_messages;
    int rc;
    currrentProc.id = -1;
    while (1)
    {
        rc = msgctl(msgq_id, IPC_STAT, &buf); // check if there is a comming process
        num_messages = buf.msg_qnum;
        int prevArr = 0;
        if (num_messages > 0)
        {
            int recv_Val = msgrcv(msgq_id, &message_recieved, sizeof(message_recieved.m_process), 7, IPC_NOWAIT);
            if (recv_Val == -1)
                perror("Error: Scheduler failed to receive  \n");

            //printf("Proccess Scheduled Id : %d at Time : %d\n", message_recieved.m_process.id, message_recieved.m_process.arrivalTime);
            Process newProc = message_recieved.m_process;
            processTable[newProc.id - 1].priority = newProc.priority;
            processTable[newProc.id - 1].execTime = newProc.runTime;
            processTable[newProc.id - 1].ID = -1;
            processTable[newProc.id - 1].cumulativeTime = 0;

            strcpy(processTable[newProc.id - 1].state, "ready");
            processTable[newProc.id - 1].remaingTime = newProc.runTime; // initail remaining Time

            // enqueue the new process
            if (sch_algo < 3)
            { // for HPF and SRTF
                // printf("Run Time from scheduler = %d '\n", newProc.runTime);
                enqueue(&q1, message_recieved.m_process);
            }
            else
            {
                enQueueCircularQueue(&q2, message_recieved.m_process);
            }
            //initialise prevArr with the value of the first process
            if (prevArr == 0)
            {
                prevArr = message_recieved.m_process.arrivalTime;
            }
            //keep receiving processes that arrived at the same time to fill the queue
            //before scheduling
            if (prevArr == message_recieved.m_process.arrivalTime)
            {
                prevArr = message_recieved.m_process.arrivalTime;
                continue;
            }
            
        }
        // TODO implement the scheduler :)
        
            switch (sch_algo)
            {
            case 1:;
                // Non-preemptive Highest Priority First (HPF)
                if (cpuFree)
                {   
                    bool flag = dequeue(&q1, &currrentProc);
                    if (flag)
                    {
                        //printf("%d \n", currrentProc.runTime);
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
                            strcpy(processTable[currrentProc.id - 1].state, "started"); // set the state running
                            processTable[currrentProc.id - 1].ID = pid;                 // set the actual pid
                            processTable[currrentProc.id - 1].startTime = getClk();     // set the start time of the process with the actual time
                            processTable[currrentProc.id - 1].responseTime = processTable[currrentProc.id - 1].startTime - currrentProc.arrivalTime;
                            processTable[currrentProc.id - 1].totWaitTime = processTable[currrentProc.id - 1].responseTime;
                            // setting the response and the waiting time with the same value as it's non-preemptive
                            WriteOutputLine(ptr, getClk(), currrentProc.id, processTable[currrentProc.id - 1].state, currrentProc.arrivalTime,
                                            currrentProc.runTime, processTable[currrentProc.id - 1].remaingTime, processTable[currrentProc.id - 1].totWaitTime,0,0);
                        }
                    }
                }
                
                break;
            case 2:;
                // SRTF
                break;
            case 3:;
                
            
                if (getClk() % Quantum == 0)
                {

                    if (currrentProc.id != -1 && !cpuFree) // if there is a running process (first time no process is running)
                    {
                        kill(processTable[currrentProc.id - 1].ID,SIGSTOP); // to stop the running process
                        enQueueCircularQueue(&q2, currrentProc);
                        strcpy(processTable[currrentProc.id - 1].state, "stopped"); // set the state running
                        WriteOutputLine(ptr, getClk(), currrentProc.id, processTable[currrentProc.id - 1].state, currrentProc.arrivalTime,
                                                currrentProc.runTime, processTable[currrentProc.id - 1].remaingTime, processTable[currrentProc.id - 1].totWaitTime,0,0);
                        processTable[currrentProc.id - 1].cumulativeTime += Quantum;
                    }
                    bool flag = deQueueCircularQueue(&q2, &currrentProc);
                    if (flag)
                    {
                        cpuFree = false;
                        if (processTable[currrentProc.id - 1].ID == -1)
                        {
                            int pid = fork();
                            if (pid == 0) // child
                            {
                                char processRunTime[2];                              // send the runTime of each
                                sprintf(processRunTime, "%d", currrentProc.runTime); // converts the int to string to sended in the arguments of the process
                                execl("process.out", "process", processRunTime, NULL);
                            }
                            else
                            {                 
                                strcpy(processTable[currrentProc.id - 1].state, "started"); // set the state running
                                processTable[currrentProc.id - 1].ID = pid;                 // set the actual pid
                                processTable[currrentProc.id - 1].startTime = getClk();     // set the start time of the process with the actual time
                                processTable[currrentProc.id - 1].responseTime = processTable[currrentProc.id - 1].startTime - currrentProc.arrivalTime;
                                processTable[currrentProc.id - 1].totWaitTime = processTable[currrentProc.id - 1].responseTime;  //initialised here and will be increased later
                                // setting the response and the waiting time with the same value as it's non-preemptive
                                WriteOutputLine(ptr, getClk(), currrentProc.id, processTable[currrentProc.id - 1].state, currrentProc.arrivalTime,
                                                currrentProc.runTime, processTable[currrentProc.id - 1].remaingTime, processTable[currrentProc.id - 1].totWaitTime,0,0);
                            }
                        }
                        else
                        {
                            kill(processTable[currrentProc.id - 1].ID,SIGCONT);
                            strcpy(processTable[currrentProc.id - 1].state, "resumed"); // set the state running
                            WriteOutputLine(ptr, getClk(), currrentProc.id, processTable[currrentProc.id - 1].state, currrentProc.arrivalTime,
                                                currrentProc.runTime, processTable[currrentProc.id - 1].remaingTime, processTable[currrentProc.id - 1].totWaitTime,0,0);
                        }
                    }
                }
                
                break;
            default:
                break;
            }
        }
    
    fclose(ptr); // close the file at the end
    ptr = fopen("scheduler.perf", "w");
    // WriteFinalOutput();
    fclose(ptr); // close the file at the end
    // upon termination release the clock resources.
    destroyClk(true);
}

void processTerminate(int sigID)
{
    cpuFree = true;
    strcpy(processTable[currrentProc.id - 1].state, "finished");
    processTable[currrentProc.id - 1].remaingTime = 0; // remaining time = 0

    int TA =  getClk()-processTable[currrentProc.id - 1].startTime;
    float WTA = (float)TA / processTable[currrentProc.id - 1].execTime;
    processTable[currrentProc.id - 1].totWaitTime = TA - processTable[currrentProc.id - 1].execTime;
    WriteOutputLine(ptr, getClk(), currrentProc.id, processTable[currrentProc.id - 1].state, currrentProc.arrivalTime,
                    currrentProc.runTime, processTable[currrentProc.id - 1].remaingTime, processTable[currrentProc.id - 1].totWaitTime,TA,WTA);
    signal(SIGUSR1, processTerminate); // attach the function to SIGUSR1
}

void WriteOutputLine(FILE *ptr, int time, int process_id, char *state, int arr, int total, int reamain, int wait,int TA,float WTA)
{
    if(!strcmp(state,"finished"))
        fprintf(ptr, "At time %d process %d %s arr %d total %d remain %d wait %d TA %d WTA %f\n", time, process_id, state, arr, total, reamain, wait,TA,WTA);
    else
        fprintf(ptr, "At time %d process %d %s arr %d total %d remain %d wait %d\n", time, process_id, state, arr, total, reamain, wait);
    fflush(ptr);
}

void WriteFinalOutput()
{
    
}