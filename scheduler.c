#include "headers.h"


int main(int argc, char * argv[])
{
    initClk();
    int sch_algo = atoi(argv[1]);
    printf("Schedule algorithm : %d\n",sch_algo);

    //switch case

    int msgq_id = msgget(MSGKEY, 0666 | IPC_CREAT); //create message queue and return id
    struct my_msgbuff message_recieved;
    struct msqid_ds buf;
    int num_messages;
    int rc;
    while (1)
    {
        rc = msgctl(msgq_id, IPC_STAT, &buf);
        num_messages = buf.msg_qnum;
        if(num_messages > 0)
        {
            int recv_Val = msgrcv(msgq_id, &message_recieved, sizeof(message_recieved.m_process), 7, IPC_NOWAIT);
            if (recv_Val == -1) perror("Error: Scheduler failed to receive the reverced message \n");
            printf("Proccess Scheduled Id : %d at Time : %d\n",message_recieved.m_process.id,message_recieved.m_process.arrivalTime);
            //enqueue
        }
            
    }
    //TODO implement the scheduler :)
    //upon termination release the clock resources.
    sleep(10);
    destroyClk(true);
}
