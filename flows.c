//
// Created by danjok on 21 11 2025.
//
#include <stdio.h>
#include <stdlib.h>

typedef struct SNetDot
{
    int flowID;
    int totalBytes;
    int flowDuration;
    double avgInterarrivalTime;
    double avgInterarrivalLength;
}netDot;


typedef struct SNetDotGroup
{
    int dotCount;
    netDot* dotArr;
}netDotCluster;


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

netDotCluster* initCluster(netDot* dots, int dotCount)
{
    netDotCluster *cluster = malloc(sizeof(netDotCluster));
    *cluster.dotCount = dotCount;
    *cluster->dotArr = malloc(sizeof(netDot)*dotCount);

    for (int i = 0; i < dotCount; i++)
    {
        *cluster.dotArr[i] = dots[i];
    }

    return cluster;
}

void addNetDotsSorted(netDot* dots, netDot dotsToAdd[], int dotCount, int filledCount)
{
    int lastBiggerInx = 0;
    for (int i = filledCount; i =< 0; i--)
    {
        if (dots[i] > dotsToAdd[filledCount])
            lastBiggerInx = i;
    }
    for (int i = filledCount; i =< lastBiggerInx; i--)
    {
        dots[i+1] = dots[i];
    }
    dots[lastBiggerInx] = dotsToAdd[filledCount];
    if (dotCount > filledCount)
    {
        addNetDotsSorted(dots, dotsToAdd, dotCount, filledCount + 1);
    }
}

netDotCluster* uniteClusters(netDotCluster *netDotCluster1, netDotCluster *netDotCluster2)
{
    int dotCount = *netDotCluster1.dotCount + *netDotCluster2.dotCount;
    netDot dotsToAdd[arrLength];

    for (int i = 0; i < netDotCluster1.dotCount; i++)
        dotsToAdd[i] = netDotCluster1->dotArr[i];

    for (int i = netDotCluster2.dotCount; i < dotCount; i++)
        dotsToAdd[i+netDotCluster2.dotCount] = netDotCluster2.dotArr[i];

    netDot dots[dotCount];
    addNetDotsSorted(dots, dotsToAdd, dotCount, 0);

    free(*netDotCluster1->dotArr);
    free(*netDotCluster2->dotArr);
    free(*netDotCluster1);
    free(*netDotCluster2);
    return initCluster(dots, dotCount)
}

double findRange(netDot netDot1, netDot netDot2);
double findClosestRange(netDotCluster netDotCluster1, netDotCluster netDotCluster2);
netDotCluster* findClosestAndUnite(netDotCluster* netDotClusterArr);
netDotCluster** appendNetDotClusterSorted(netDotCluster** netDotClusterArr, netDotCluster cluster);
netDotCluster** uniteToNGroups(int destGroupCount, int currGroupCount, netDotCluster* netDotGroupArr);
void clusterOut(netDotCluster cluster);
void infoOut(int currGroupCount, netDotCluster* netDotGroupArr);


int main(int argc, char* argv[])
{

    return 0;
}
