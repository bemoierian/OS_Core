#include "headers.h"

/* Modify this file as needed*/
int remainingtime;

int main(int agrc, char *argv[])
{
    initClk();
    int startTime = getClk();    // get the start time of the process
    int runTime = atoi(argv[1]); // RuntTime of the process
    remainingtime = runTime;
    printf("RunTime = %d \n", runTime);
    // TODO it needs to get the remaining time from somewhere
    // remainingtime = ??;
    while (remainingtime > 0)
    {
        // remainingtime = ??;
        remainingtime = runTime - (getClk() - startTime);
    }
    kill(getppid(), SIGUSR1); // send the signal to the schedular (parent)
    destroyClk(false);
    return 0;
}
