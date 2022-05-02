#include "headers.h"

/* Modify this file as needed*/
int remainingtime;
void processContinue(int sigID);
int prevClk;
int currClk;
int main(int agrc, char *argv[])
{
    initClk();
    signal(SIGCONT, processContinue); // attach the function to SIGUSR1
    int startTime = getClk();    // get the start time of the process
    int runTime = atoi(argv[1]); // RuntTime of the process
    remainingtime = runTime;
    prevClk = getClk();
    printf("RunTime = %d \n", runTime);
    // TODO it needs to get the remaining time from somewhere
    // remainingtime = ??;
    while (remainingtime > 0)
    {
        currClk = getClk();
        if (prevClk != currClk)
        {
            remainingtime--;
            prevClk = currClk;
        }
        // remainingtime = ??;
    }
    kill(getppid(), SIGUSR1); // send the signal to the schedular (parent)
    destroyClk(false);
    return 0;
}
void processContinue(int sigID)
{
    prevClk = getClk();
    signal(SIGCONT, processContinue); // attach the function to SIGUSR1
}