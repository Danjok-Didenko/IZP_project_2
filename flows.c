#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

typedef struct SNetDot
{
    int flowID;
    int totalBytes;
    int flowDuration;
    double avgInterarrivalTime;
    double avgInterarrivalLength;
}netDot;


typedef struct SNetDotCluster
{
    int dotCount;
    netDot* dotArr;
}netDotCluster;

typedef struct SWeights
{
    double bytes;
    double duration;
    double interTime;
    double interLength;
}weights;

//calculates square number for double type variable
double squareFloat(double a)
{
    return a*a;
}

//calculates square number for int type variable
int squareInt(int a)
{
    return a*a;
}

void allocClusterCheck(netDotCluster* cluster)
{
    if (cluster == NULL)
    {
        free(cluster);
        exit(1);
    }
}

void allocDotArrCheck(netDot *dotArr)
{
    if (dotArr == NULL)
    {
        free(dotArr);
        exit(1);
    }
}

//initialises netDot with entered params
netDot initNetDot(int flowID, int totalBytes, int flowDuration, double avgInterarrivalTime, double avgInterarrivalLength)
{
    netDot dot;
    dot.flowID = flowID;
    dot.totalBytes = totalBytes;
    dot.flowDuration = flowDuration;
    dot.avgInterarrivalTime = avgInterarrivalTime;
    dot.avgInterarrivalLength = avgInterarrivalLength;

    return dot;
}

//initialises cluster using prepared array of netDots
netDotCluster initCluster(netDot dots[], int dotCount)
{
    netDotCluster cluster;
    cluster.dotCount = dotCount;
    cluster.dotArr = malloc(sizeof(netDot)*dotCount);

    allocDotArrCheck(cluster.dotArr);

    for (int i = 0; i < dotCount; i++)
    {
        cluster.dotArr[i] = dots[i];
    }

    return cluster;
}

netDotCluster initSingleDotCluster(netDot dot)
{
    netDot dots[1];
    dots[0] = dot;
    return initCluster(dots, 1);
}

//frees memory allocated for netDots in cluster and changes dotCount to 0
//in order to remove cluster from array entirely
void prepareClusterRemoval(netDotCluster* cluster)
{
    free(cluster->dotArr);
    cluster->dotArr = NULL;
    cluster->dotCount = 0;
}

//deletes all empty clusters(marked by having 0 dotCount) without leaving any blank spaces in array
netDotCluster* deleteEmptyClusters(netDotCluster* clusterArr, int *currClusterCount)
{
    for (int i = (*currClusterCount)-1; i >= 0; i--)
    {
        if(clusterArr[i].dotCount == 0)
        {
            (*currClusterCount)--;
            for (int n = i; n < (*currClusterCount); n++)
            {
                clusterArr[n] = clusterArr[n+1];
            }
            clusterArr = realloc(clusterArr, sizeof(netDotCluster)*(*currClusterCount));

            allocClusterCheck(clusterArr);
        }
    }
    return clusterArr;
}

//adds netDots from array to cluster in sorted order by their flowID
void appendNetDotsSorted(netDot* dots, netDot dotsToAdd[], int dotCount, int filledCount)
{
    int lastBiggerInx = 0;
    for (int i = filledCount-1; i >= 0; i--)
    {
        if (dots[i].flowID > dotsToAdd[filledCount].flowID)
            lastBiggerInx = i;
    }
    for (int i = filledCount; i <= lastBiggerInx; i--)
    {
        dots[i+1] = dots[i];
    }
    dots[lastBiggerInx] = dotsToAdd[filledCount];
    if (dotCount > filledCount)
    {
        appendNetDotsSorted(dots, dotsToAdd, dotCount, filledCount + 1);
    }
}

//creates 1 united cluster from 2 and deletes them
netDotCluster uniteClusters(netDotCluster netDotCluster1, netDotCluster netDotCluster2)
{
    int dotCount = netDotCluster1.dotCount + netDotCluster2.dotCount;
    netDot dotsToAdd[dotCount];

    for (int i = 0; i < netDotCluster1.dotCount; i++)
        dotsToAdd[i] = netDotCluster1.dotArr[i];

    for (int i = netDotCluster2.dotCount; i < dotCount; i++)
        dotsToAdd[i+netDotCluster1.dotCount] = netDotCluster2.dotArr[i];

    netDot dots[dotCount];
    appendNetDotsSorted(dots, dotsToAdd, dotCount, 0);

    prepareClusterRemoval(&netDotCluster1);
    prepareClusterRemoval(&netDotCluster2);

    return initCluster(dots, dotCount);
}

//founds range between 2 netDots
double findRange(netDot netDot1, netDot netDot2, weights weights1)
{
    return sqrt(
    weights1.bytes*squareInt(netDot1.totalBytes - netDot2.totalBytes) +
    weights1.duration*squareInt(netDot1.flowDuration - netDot2.flowDuration) +
    weights1.interTime*squareFloat(netDot1.avgInterarrivalTime - netDot2.avgInterarrivalTime) +
    weights1.interLength* squareFloat(netDot1.avgInterarrivalLength - netDot2.avgInterarrivalLength)
    );
}

//finds closest range between 2 clusters
double findClosestRange(netDotCluster netDotCluster1, netDotCluster netDotCluster2, weights weights1)
{
    double closestFoundRange = findRange(netDotCluster1.dotArr[0],netDotCluster2.dotArr[1], weights1);
    for (int i = 0; i < netDotCluster1.dotCount; i++)
    {
        for (int j = 0; j < netDotCluster2.dotCount; j++)
        {
            if(closestFoundRange > findRange(netDotCluster1.dotArr[i],netDotCluster2.dotArr[j], weights1))
                closestFoundRange = findRange(netDotCluster1.dotArr[i],netDotCluster2.dotArr[j], weights1);
        }
    }
    return closestFoundRange;
}

//appends cluster to the cluster array in sorted order
netDotCluster* appendNetDotClusterSorted(netDotCluster* clusterArr, netDotCluster clusterToAdd, int *currClusterCount)
{
    int lastBiggerInx = 0;

    (*currClusterCount)++;

    clusterArr = realloc(clusterArr, sizeof(netDotCluster)*(*currClusterCount));

    allocClusterCheck(clusterArr);

    for (int i = (*currClusterCount)-1; i <= 0; i--)
    {
        if (clusterArr[lastBiggerInx].dotArr[0].flowID > clusterArr[i].dotArr[0].flowID)
            lastBiggerInx = i;
    }
    for (int i = (*currClusterCount)-1; i <= lastBiggerInx; i--)
    {
        clusterArr[i+1] = clusterArr[i];
    }
    clusterArr[lastBiggerInx] = clusterToAdd;

    return clusterArr;
}

//finds closest pair of clusters and returns array with united cluster
netDotCluster* findClosestAndUnite(netDotCluster* clusterArr, weights weights1, int *currClusterCount)
{
    int closestInx[2] = {0, 0};
    double closestFoundRange = findClosestRange(clusterArr[0], clusterArr[0], weights1);

    for (int i = 0; i < *currClusterCount-1; i++)
    {
        for (int j = i+1; j < *currClusterCount; j++)
        {

            if(closestFoundRange > findClosestRange(clusterArr[i], clusterArr[j], weights1))
            {
                closestFoundRange = findClosestRange(clusterArr[i], clusterArr[j], weights1);;
                closestInx[0] = i;
                closestInx[1] = j;
            }
        }}
    netDotCluster unitedCluster = uniteClusters(clusterArr[closestInx[0]], clusterArr[closestInx[0]]);

    clusterArr = deleteEmptyClusters(clusterArr, currClusterCount);

    clusterArr = appendNetDotClusterSorted(clusterArr, unitedCluster, currClusterCount);

    return clusterArr;
}

netDotCluster* uniteToNGroups(int destClusterCount, int *currClusterCount, netDotCluster* clusterArr, weights weights1)
{
    do
    {
        clusterArr = findClosestAndUnite(clusterArr, weights1, currClusterCount);
    }
    while (destClusterCount != *currClusterCount);

    return clusterArr;
}

void clusterOut(netDotCluster cluster, int clusterInx)
{
    printf("cluster %i ", clusterInx);

    for (int i = 0; i < cluster.dotCount; i++)
    {
        printf("%i", cluster.dotArr[i].flowID);
    }
    printf("\n");
}

void infoOut(int *currClusterCount, netDotCluster* clusterArr)
{
    printf("Clusters:\n");

    for (int i = 0; i < *currClusterCount; i++)
    {
        clusterOut(clusterArr[i], i);
    }
}


int main(int argc, char* argv[])
{
    if (argc != 7)
    {
        fprintf(stderr, "ERROR: Wrong arguments");
        return 1;
    }

    FILE* srcFile = fopen(argv[1], "r");

    if (srcFile == NULL)
    {
        fprintf(stderr, "ERROR: Failed to open the file");
        return 1;
    }

    weights weights1;
    char *endptr;
    weights1.bytes = strtod(argv[3], &endptr);
    weights1.duration = strtod(argv[4], &endptr);
    weights1.interTime = strtod(argv[5], &endptr);
    weights1.interLength = strtod(argv[6], &endptr);

    printf("%lf", weights1.bytes);

    int currClusterCount;
    int destClusterCount = atoi(argv[2]);

    int flowID;
    int totalBytes;
    int flowDuration;
    int packetCount;
    double avgInterarrivalTime;

    double avgInterarrivalLength;

    fscanf(srcFile, "count=%i\n", &currClusterCount);

    netDotCluster* clusterArr = malloc(sizeof(netDotCluster)*currClusterCount);
    allocClusterCheck(clusterArr);


    for (int i = 0; i < currClusterCount; i++)
    {
        fscanf(srcFile, "%i %*s %*s %i %i %i %lf\n",
            &flowID, &totalBytes, &flowDuration,
            &packetCount, &avgInterarrivalTime);
        avgInterarrivalLength = (double)totalBytes/packetCount;
        netDot dot = initNetDot(flowID, totalBytes, flowDuration, avgInterarrivalTime, avgInterarrivalLength);
        clusterArr[i] = initSingleDotCluster(dot);
    }

    uniteToNGroups(destClusterCount, &currClusterCount, clusterArr, weights1);

    infoOut(&currClusterCount, clusterArr);

    for (int i = 0; i < currClusterCount; i++)
    {
        prepareClusterRemoval(&clusterArr[i]);
    }
    free(clusterArr);
    return 0;
}