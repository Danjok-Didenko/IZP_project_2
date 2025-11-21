//
// Created by danjok on 21 11 2025.
//
#include <stdio.h>

typedef struct SNetDot
{
    int flowID;
    int totalBytes;
    int flowDuration;
    float avgInterarrivalTime;
    float avgInterarrivalLength;
}netDot;


typedef struct SNetDotGroup
{
    int dotCount;
    netDot* dotArr;
}netDotCluster;

//Function declarations
void fillNetDotData(int flowID, int totalBytes, int flowDuration, float avgInterarrivalTime, float avgInterarrivalLength);
netDotCluster* initGroup(netDot dot);
netDotCluster* uniteGroups(netDotCluster netDotGroup1, netDotCluster netDotGroup2);
double findRange(netDot netDot1, netDot netDot2);
double findClosestRange(netDotCluster netDotGroup1, netDotCluster netDotGroup2);
netDotCluster* findClosestAndUnite(netDotCluster* netDotGroupArr);
netDotCluster** uniteToNGroups(int destGroupCount, int currGroupCount, netDotCluster* netDotGroupArr);
void clusterOut(netDotCluster cluster);
void infoOut(int currGroupCount, netDotCluster* netDotGroupArr);


int main(int argc, char* argv[])
{

    return 0;
}
