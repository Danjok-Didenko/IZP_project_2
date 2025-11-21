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
}netDotGroup;

//Function declarations
void fillNetDotData(int flowID, int totalBytes, int flowDuration, float avgInterarrivalTime, float avgInterarrivalLength);
netDotGroup* initGroup(netDot dot);
netDotGroup* uniteGroups(netDotGroup netDotGroup1, netDotGroup netDotGroup2);
double findRange(netDot netDot1, netDot netDot2);
double findClosestRange(netDotGroup netDotGroup1, netDotGroup netDotGroup2);
netDotGroup* findClosestAndUnite(netDotGroup* netDotGroupArr);
netDotGroup** uniteToNGroups(int destGroupCount, int currGroupCount, netDotGroup* netDotGroupArr);
void infoOut(int currGroupCount, netDotGroup* netDotGroupArr);


int main(int argc, char* argv[])
{

    return 0;
}
