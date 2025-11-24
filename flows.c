#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>

typedef struct SFlow
{
    int flowID;
    int totalBytes;
    int flowDuration;
    double avgInterTime;
    double avgInterLength;
}flow;


typedef struct SCluster
{
    int flowCount;
    flow* flowArr;
}cluster;


//Mathematical custom functions
//-------------------------------------------------------------------------------------

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

//Functions for working with flows and clusters
//-------------------------------------------------------------------------------------

//function for qsort
int compareID(const flow* a, const flow* b);

//rearrange flow array by smallest flowID
void sortByID(flow* flowArr, int flowCount);

//calculates average interarrival length
double calculateAvgInterLength(int totalBytes, int packetCount)
{
    return (double)totalBytes/(double)packetCount;
}

//initialises flow with entered params
flow initFlow(int flowID, int totalBytes, int flowDuration, int packetCount, double avgInterarrivalTime)
{
    flow flow;
    flow.flowID = flowID;
    flow.totalBytes = totalBytes;
    flow.flowDuration = flowDuration;
    flow.avgInterTime = avgInterarrivalTime;
    flow.avgInterLength = calculateAvgInterLength(totalBytes, packetCount);

    return flow;
}

//creates cluster with given flows and given number
cluster initCluster(flow flows[], int flowCount)
{
    cluster cluster;

    cluster.flowCount = flowCount;
    flow* flowArr = malloc(sizeof(flow)*flowCount);

    //unsuccessful allocation check
    if (flowArr == NULL)
    {
        exit(1);
    }
    else
    {
        cluster.flowArr = flowArr;
        return cluster;
    }
}

