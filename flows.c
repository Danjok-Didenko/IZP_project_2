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

typedef struct SClusterStorage
{
    int clusterCount;
    cluster* clusters;
}clusterStorage;

typedef struct SWeights
{
    double bytes;
    double duration;
    double interTime;
    double interLength;
}weights;

//Functions for sorting
//-------------------------------------------------------------------------------------

//function for qsort
int compareID(const flow* a, const flow* b);

//rearrange flow array by smallest flowID
void sortByID(flow* flowArr, int flowCount);


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
    flow* tmp = malloc(sizeof(flow)*flowCount);

    //unsuccessful allocation check
    if (tmp == NULL)
    {
        fprintf(stderr, "Error alloc failed");
        exit(1);
    }
    else
    {
        cluster.flowArr = tmp;
        for (int i = 0; i < flowCount; i++)
        {
            cluster.flowArr[i] = flows[i];
        }
        return cluster;
    }
}

clusterStorage initClusterStorage(cluster clusters[], int clusterCount)
{
    clusterStorage storage;
    storage.clusterCount = clusterCount;
    cluster* tmp = malloc(sizeof(cluster)*clusterCount);

    //unsuccessful allocation check
    if (tmp == NULL)
    {
        fprintf(stderr, "Error alloc failed");
        exit(1);
    }
    else
    {
        storage.clusters = tmp;
        for (int i = 0; i < clusterCount; i++)
        {
            storage.clusters[i] = clusters[i];
        }
        return storage;
    }
}


//unites 2 clusters
cluster uniteClusters(cluster clusterA, cluster clusterB)
{
    flow flows[clusterA.flowCount + clusterB.flowCount];
    return initCluster(flows, clusterA.flowCount + clusterB.flowCount);
}

//prepares cluster for delete
void prepareForDelete(cluster* cluster)
{
    free(cluster->flowArr);
    cluster->flowArr = NULL;
    cluster->flowCount = 0;
}

//unites 2 clusters and deletes originals
void uniteAndDelete(clusterStorage* storage, cluster clusterA, cluster clusterB)
{
    cluster unitedCluster = uniteClusters(clusterA, clusterB);
    prepareForDelete(&clusterA);
    prepareForDelete(&clusterB);

    for (int i = (storage->clusterCount)-1; i >= 0; i--)
    {
        if (storage->clusters[i].flowCount == 0)
        {
            for (int n = i; i < (storage->clusterCount)-2; i++)
            {
                storage->clusters[n] = storage->clusters[n+1];
            }
        }
    }
    (storage->clusterCount)--;
    cluster *tmp = realloc(storage->clusters, sizeof(cluster)*storage->clusterCount);

    //unsuccessful allocation check
    if (tmp == NULL)
    {
        fprintf(stderr, "Error alloc failed");
        exit(1);
    }
    else
    {
        storage->clusters = tmp;
    }
    storage->clusters[storage->clusterCount-1] = unitedCluster;
}


double findRange(flow flowA, flow flowB, weights weights)
{
    return sqrt(
    weights.bytes*squareInt(flowA.totalBytes - flowB.totalBytes) +
    weights.duration*squareInt(flowA.flowDuration - flowB.flowDuration) +
    weights.interTime*squareFloat(flowA.avgInterTime - flowB.avgInterTime) +
    weights.interLength* squareFloat(flowA.avgInterLength - flowB.avgInterLength)
    );
}
