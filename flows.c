//
// Created by danjok on 21 11 2025.
//
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
netDotCluster* initCluster(netDot dots[], int dotCount)
{
    netDotCluster *cluster = malloc(sizeof(netDotCluster));
    cluster->dotCount = dotCount;
    cluster->dotArr = malloc(sizeof(netDot)*dotCount);

    for (int i = 0; i < dotCount; i++)
    {
        cluster->dotArr[i] = dots[i];
    }

    return cluster;
}

//frees memory allocated for netDots in cluster and changes dotCount to 0
//in order to remove cluster from array entirely
void prepareClusterRemoval(netDotCluster cluster)
{
    free(cluster.dotArr);
    cluster.dotArr = NULL;
    cluster.dotCount = 0;
}

//deletes all empty clusters(marked by having 0 dotCount) without leaving any blank spaces in array
void deleteEmptyClusters(netDotCluster* clusterArr, int *currClusterCount)
{
    for (int i = (*currClusterCount)-1; i >= 0; i--)
    {
        if(clusterArr[i].dotCount == 0)
        {
            (*currClusterCount)--;
            for (int n = (*currClusterCount)-1; n >= i; n--)
            {
                clusterArr[n] = clusterArr[n+1];
            }
            clusterArr = realloc(clusterArr, sizeof(netDotCluster)*(*currClusterCount));
        }
    }
}

//adds netDots from array to cluster in sorted order by their flowID
void addNetDotsSorted(netDot* dots, netDot dotsToAdd[], int dotCount, int filledCount)
{
    int lastBiggerInx = 0;
    for (int i = filledCount; i <= 0; i--)
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
        addNetDotsSorted(dots, dotsToAdd, dotCount, filledCount + 1);
    }
}

//creates 1 united cluster from 2 and deletes them
netDotCluster* uniteClusters(netDotCluster netDotCluster1, netDotCluster netDotCluster2)
{
    int dotCount = netDotCluster1.dotCount + netDotCluster2.dotCount;
    netDot dotsToAdd[dotCount];

    for (int i = 0; i < netDotCluster1.dotCount; i++)
        dotsToAdd[i] = netDotCluster1.dotArr[i];

    for (int i = netDotCluster2.dotCount; i < dotCount; i++)
        dotsToAdd[i+netDotCluster2.dotCount] = netDotCluster2.dotArr[i];

    netDot dots[dotCount];
    addNetDotsSorted(dots, dotsToAdd, dotCount, 0);

    prepareClusterRemoval(netDotCluster1);
    prepareClusterRemoval(netDotCluster2);

    return initCluster(dots, dotCount);
}

//founds range between 2 netDots
double findRange(netDot netDot1, netDot netDot2, weights weights)
{
    return sqrt(
    weights.bytes*squareInt(netDot1.totalBytes - netDot2.totalBytes) +
    weights.duration*squareInt(netDot1.flowDuration - netDot2.flowDuration) +
    weights.interTime*squareFloat(netDot1.avgInterarrivalTime - netDot2.avgInterarrivalTime) +
    weights.interLength* squareFloat(netDot1.avgInterarrivalLength - netDot2.avgInterarrivalLength)
    );
}

//finds closest range between 2 clusters
double findClosestRange(netDotCluster netDotCluster1, netDotCluster netDotCluster2, weights weights)
{
    double closestFoundRange = findRange(netDotCluster1.dotArr[0],netDotCluster2.dotArr[0], weights);
    for (int i = 0; i < netDotCluster1.dotCount; i++)
    {
        for (int j = 0; j < netDotCluster2.dotCount; j++)
        {
            if(closestFoundRange > findRange(netDotCluster1.dotArr[i],netDotCluster2.dotArr[j], weights))
                closestFoundRange = findRange(netDotCluster1.dotArr[i],netDotCluster2.dotArr[j], weights);
        }
    }
    return closestFoundRange;
}

//finds closest pair of clusters and returns united cluster
netDotCluster* findClosestAndUnite(netDotCluster* clusterArr, weights weights, int *currClusterCount)
{
    int closestInx[2] = {0, 0};
    double closestFoundRange = findClosestRange(clusterArr[0], clusterArr[0], weights);

    for (int i = 0; i < *currClusterCount-1; i++)
    {
        for (int j = i+1; j < *currClusterCount; j++)
        {

            if(closestFoundRange > findClosestRange(clusterArr[i], clusterArr[j], weights))
            {
                closestFoundRange = findClosestRange(clusterArr[i], clusterArr[j], weights);;
                closestInx[0] = i;
                closestInx[1] = j;
            }
        }}
    netDotCluster *unitedCluster = uniteClusters(clusterArr[closestInx[0]], clusterArr[closestInx[0]]);

    deleteEmptyClusters(clusterArr, currClusterCount);

    return unitedCluster;
}

//appends cluster to the cluster array in sorted order
netDotCluster* appendNetDotClusterSorted(netDotCluster* netDotClusterArr, netDotCluster cluster, int *currClusterCount);
netDotCluster* uniteToNGroups(int destGroupCount, int *currClusterCount, netDotCluster* netDotGroupArr, weights weights);
void clusterOut(netDotCluster cluster);
void infoOut(int *currClusterCount, netDotCluster* netDotGroupArr);


int main(int argc, char* argv[])
{

    return 0;
}
