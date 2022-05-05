#include "headers.h"

/* Modify this file as needed*/
int remainingtime;
int main(int agrc, char *argv[])
{
    initClk();
    //-----------------SHARED MEMORY------------
    int shmid = shmget(PS_SHM_KEY, 4, IPC_CREAT | 0644);
    if (shmid == -1)
    {
        perror("Process: error in shared memory create\n");
        exit(-1);
    }
    // attach shared memory to process
    int *ps_shmaddr = (int *)shmat(shmid, (void *)0, 0);
    if ((long)ps_shmaddr == -1)
    {
        perror("Process: error in attach\n");
        exit(-1);
    }
    //-------------SEMPAHORE--------------
    int sem1 = semget(SEM1_KEY, 1, 0666 | IPC_CREAT);
    union Semun semun;

    int startTime = getClk();    // get the start time of the process
    int runTime = atoi(argv[1]); // RuntTime of the process
    remainingtime = runTime;
    printf("RunTime = %d \n", runTime);
    // TODO it needs to get the remaining time from somewhere
    // remainingtime = ??;
    while (remainingtime > 0)
    {
        remainingtime = *ps_shmaddr;
        // remainingtime = ??;
    }
    kill(getppid(), SIGUSR1); // send the signal to the schedular (parent)
    up(sem1);
    shmdt(ps_shmaddr);
    destroyClk(false);
    return 0;
}
