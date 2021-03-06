#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#define null 0

struct processData
{
    int arrivaltime;
    int priority;
    int runningtime;
    int id;
    int size;
};

int main(int argc, char *argv[])
{
    FILE *pFile;
    pFile = fopen("processes.txt", "w");
    int no;
    struct processData pData;
    printf("Please enter the number of processes you want to generate: ");
    scanf("%d", &no);
    srand(time(null));
    // fprintf(pFile,"%d\n",no);
    fprintf(pFile, "#id arrival runtime priority memsize\n");
    pData.arrivaltime = 0;
    for (int i = 1; i <= no; i++)
    {
        // generate Data Randomly
        //[min-max] = rand() % (max_number + 1 - minimum_number) + minimum_number
        pData.id = i;
        pData.arrivaltime += rand() % (11);        // processes arrives in order
        pData.runningtime = rand() % (30);         // max running time is 29
        pData.priority = rand() % (11);            // max number is 10
        pData.size = (rand() % (256 + 1 - 1)) + 1; // min size of a procss = 1 and max size for a process is 256
        fprintf(pFile, "%d\t%d\t%d\t%d\t%d\n", pData.id, pData.arrivaltime, pData.runningtime, pData.priority, pData.size);
    }
    fclose(pFile);
    return 0;
}
