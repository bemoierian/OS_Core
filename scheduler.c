#include "headers.h"

int main(int argc, char *argv[])
{
    initClk();
    int sch_algo = atoi(argv[1]); // type of the algo
    int q_size = atoi(argv[2]);   // max no of processes
    // printf("Schedule algorithm : %d\n", sch_algo);

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
                perror("Error: Scheduler failed to receive the reverced message \n");
            printf("Proccess Scheduled Id : %d at Time : %d\n", message_recieved.m_process.id, message_recieved.m_process.arrivalTime);
            // enqueue the new process
            if (sch_algo < 3)
            { // for HPF and SRTF
                enqueue(q1, message_recieved.m_process);
                int pid = fork();
                if (pid == 0)
                {
                    char processRunTime[2];                                            // send the runTime of each
                    sprintf(processRunTime, "%d", message_recieved.m_process.runTime); // converts the int to string to sended in the arguments of the process
                    execl("process.out", "process", processRunTime, NULL);
                }
            }
            else
            {
                enQueueCircularQueue(q2, message_recieved.m_process);
            }
        }
    }
    // TODO implement the scheduler :)

    // upon termination release the clock resources.
    destroyClk(true);
}
