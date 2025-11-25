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
int compareFlowsID(const void* a, const void* b)
{
    int arg1 = ((const flow*)a)->flowID;
    int arg2 = ((const flow*)b)->flowID;

    if (arg1 < arg2) return -1;
    if (arg1 > arg2) return 1;
    return 0;
}

//rearrange flow array by smallest flowID
void sortFlowsByID(flow* flowArr, int flowCount)
{
    qsort(flowArr, flowCount, sizeof(flow), compareFlowsID);
}


//function for qsort
int compareClustersID(const void* a, const void* b)
{
    int arg1 = ((const cluster*)a)->flowArr[0].flowID;
    int arg2 = ((const cluster*)b)->flowArr[0].flowID;

    if (arg1 < arg2) return -1;
    if (arg1 > arg2) return 1;
    return 0;
}
//rearrange flow array by smallest flowID
void sortClustersByID(cluster* clusters, int clusterCount)
{
    qsort(clusters, clusterCount, sizeof(cluster), compareClustersID);
}


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
        fprintf(stderr, "Error alloc failed\n");
        exit(1);
    }
    else
    {
        cluster.flowArr = tmp;
        sortFlowsByID(flows, flowCount);
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
        fprintf(stderr, "Error alloc failed\n");
        exit(1);
    }
    else
    {
        for (int i = 0; i < clusterCount; i++)
        {
            tmp[i] = clusters[i];
        }
        storage.clusters = tmp;
        return storage;
    }
}


//unites 2 clusters
cluster uniteClusters(cluster clusterA, cluster clusterB)
{
    flow flows[clusterA.flowCount + clusterB.flowCount];
    for (int i = 0; i < clusterA.flowCount; i++)
    {
        flows[i] = clusterA.flowArr[i];
    }
    for (int i = 0; i < clusterB.flowCount; i++)
    {
        flows[i+clusterA.flowCount] = clusterB.flowArr[i];
    }
    return initCluster(flows, clusterA.flowCount + clusterB.flowCount);
}

//prepares cluster for delete
void prepareForDelete(cluster* cluster)
{
    free(cluster->flowArr);
    cluster->flowArr = NULL;
    cluster->flowCount = -1;
}

//unites 2 clusters and deletes originals
void uniteAndDelete(clusterStorage* storage, cluster *clusterA, cluster *clusterB)
{
    cluster unitedCluster = uniteClusters(*clusterA, *clusterB);
    prepareForDelete(clusterA);
    prepareForDelete(clusterB);

    for (int i = (storage->clusterCount)-1; i >= 0; i--)
    {
        if (storage->clusters[i].flowCount == -1)
        {
            for (int n = i; n < (storage->clusterCount)-1; n++)
            {
                storage->clusters[n] = storage->clusters[n+1];
            }
        }
    }
    (storage->clusterCount)--;
    storage->clusters[(storage->clusterCount)-1] = unitedCluster;
    cluster *tmp = realloc(storage->clusters, sizeof(cluster)*storage->clusterCount);
    //unsuccessful allocation check
    if (tmp == NULL)
    {
        fprintf(stderr, "Error alloc failed\n");
        exit(1);
    }
    else
    {
        storage->clusters = tmp;
    }
    storage->clusters[storage->clusterCount-1] = unitedCluster;
}

//finds range between 2 netDots
double findRange(flow flowA, flow flowB, weights weights)
{
    return sqrt(
    weights.bytes*squareInt(flowA.totalBytes - flowB.totalBytes) +
    weights.duration*squareInt(flowA.flowDuration - flowB.flowDuration) +
    weights.interTime*squareFloat(flowA.avgInterTime - flowB.avgInterTime) +
    weights.interLength* squareFloat(flowA.avgInterLength - flowB.avgInterLength)
    );
}

//finds closest range between 2 clusters
double findClosestRange(cluster netDotClusterA, cluster netDotClusterB, weights weights)
{
    double closestFoundRange;
    for (int i = 0; i < netDotClusterA.flowCount; i++)
    {
        for (int j = 0; j < netDotClusterB.flowCount; j++)
        {
            if(closestFoundRange > findRange(netDotClusterA.flowArr[i],netDotClusterB.flowArr[j], weights))
            {
                closestFoundRange = findRange(netDotClusterA.flowArr[i],netDotClusterB.flowArr[j], weights);
            }
        }
    }
    return closestFoundRange;
}

//finds closest pair of clusters and returns array with united cluster
void findClosestAndUnite(clusterStorage* storage, weights weights1)
{
    int closestInx[2] = {0, 1};
    double closestFoundRange = findClosestRange(storage->clusters[0], storage->clusters[1], weights1);

    for (int i = 0; i < storage->clusterCount-1; i++)
    {
        for (int j = i+1; j < storage->clusterCount; j++)
        {
            if (storage->clusters[i].flowArr[1].flowID == storage->clusters[j].flowArr[1].flowID)
            {
                continue;
            }
            if(closestFoundRange > findClosestRange(storage->clusters[i], storage->clusters[j], weights1))
            {
                closestFoundRange = findClosestRange(storage->clusters[i], storage->clusters[j], weights1);;
                closestInx[0] = i;
                closestInx[1] = j;
            }
        }
    }
    uniteAndDelete(storage, &storage->clusters[closestInx[0]], &storage->clusters[closestInx[1]]);
}

//finds and unites clusters until their number reaches wanted count
void uniteToNGroups(int destClusterCount, clusterStorage* storage, weights weights)
{
    if (destClusterCount == storage->clusterCount)
    {
        exit(0);
    }
    do
    {
        findClosestAndUnite(storage, weights);
    }
    while (destClusterCount != storage->clusterCount);
    sortClustersByID(storage->clusters, storage->clusterCount);
}

//prints info about exact cluster
void clusterOut(cluster cluster, int clusterInx)
{
    printf("cluster %i: ", clusterInx);

    for (int i = 0; i < cluster.flowCount; i++)
    {
        printf("%i ", cluster.flowArr[i].flowID);
    }
    printf("\n");
}

//prints info about all clusters
void infoOut(clusterStorage storage)
{
    printf("Clusters:\n");

    for (int i = 0; i < storage.clusterCount; i++)
    {
        clusterOut(storage.clusters[i], i);
    }
}

int main(int argc, char* argv[])
{
    if (argc != 7)
    {
        fprintf(stderr, "ERROR: Wrong arguments\n");
        return 1;
    }

    FILE* srcFile = fopen(argv[1], "r");

    if (srcFile == NULL)
    {
        fprintf(stderr, "ERROR: Failed to open the file\n");
        return 1;
    }

    weights weights;
    char *endptr;
    weights.bytes = strtod(argv[3], &endptr);
    weights.duration = strtod(argv[4], &endptr);
    weights.interTime = strtod(argv[5], &endptr);
    weights.interLength = strtod(argv[6], &endptr);

    int currClusterCount;
    int destClusterCount = strtol(argv[2], &endptr, 10);

    int flowID;
    int totalBytes;
    int flowDuration;
    int packetCount;
    double avgInterarrivalTime;

    fscanf(srcFile, "count=%i\n", &currClusterCount);

    cluster tmpClusterArr[currClusterCount];
    flow flows[1];


    for (int i = 0; i < currClusterCount; i++)
    {
        fscanf(srcFile, "%i %*s %*s %i %i %i %lf\n",
            &flowID, &totalBytes, &flowDuration,
            &packetCount, &avgInterarrivalTime);

        flow flow = initFlow(flowID, totalBytes, flowDuration, packetCount, avgInterarrivalTime);
        flows[0] = flow;
        tmpClusterArr[i] = initCluster(flows, 1);
    }
    fclose(srcFile);

    clusterStorage clusterStorage = initClusterStorage(tmpClusterArr, currClusterCount);

    uniteToNGroups(destClusterCount, &clusterStorage, weights);
    // findClosestAndUnite(&clusterStorage,weights);
    // findClosestAndUnite(&clusterStorage,weights);

    infoOut(clusterStorage);

    for (int i = 0; i < clusterStorage.clusterCount; i++)
    {
        prepareForDelete(&(clusterStorage.clusters[i]));
    }
    free(clusterStorage.clusters);

    return 0;
}