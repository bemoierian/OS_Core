#include "headers.h"


int main(int argc, char * argv[])
{
    initClk();
    int sch_algo = atoi(argv[1]);
    printf("Schedule algorithm : %d\n",sch_algo);
    //TODO implement the scheduler :)
    //upon termination release the clock resources.
    sleep(10);
    destroyClk(true);
}
