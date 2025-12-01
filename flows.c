#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>
#include <stdbool.h>

enum programStage
{
    inputProcessing,
    fileOpen,
    fileRead,
    clusterWrite,
    afterRead
};

//structure for storing all flow's arguments
typedef struct SFlow
{
    int flowID;
    int totalBytes;
    int flowDuration;
    double avgInterTime;
    double avgInterLength;
}flow;

//structure for storing all clusters flows as well as flow's count for more convenient use
typedef struct SCluster
{
    int flowCount;
    flow* flowArr;
}cluster;

//structure for storing all clusters as well as cluster count for more convenient use in functions
typedef struct SClusterStorage
{
    int clusterCount;
    cluster* clusters;
}clusterStorage;

//structure for storing all user-entered weights in one place for more convenient usage in functions
typedef struct SWeights
{
    double bytes;
    double duration;
    double interTime;
    double interLength;
}weights;

//TODO - Range storage structure

//function declaration (used only here for 1 purpose)
void prepareForDelete(cluster* cluster);

//function for finishing program
void finishProgram(int programStage, bool isError, FILE* srcFile, clusterStorage *clusterStorage)
{
    switch (programStage)
    {
        case inputProcessing:
            fprintf(stderr, "Error: Something is wrong with entered arguments\n");
            break;
        case fileOpen:
            fprintf(stderr, "Error: File failed to read\n");
            break;
        case fileRead:
            fclose(srcFile);
            fprintf(stderr, "Error: Something is wrong with input file\n");
            break;
        case afterRead:
            if (isError)
            {
                fprintf(stderr, "Error: Some allocation failed\n");
            }
            //prepares all clusters in cluster storage for deletion
            for (int i = 0; i < clusterStorage->clusterCount; i++)
            {
                prepareForDelete(&(clusterStorage->clusters[i]));
            }

            //frees cluster storage if it was inited
            if (clusterStorage->clusterCount !=-1)
            {
                free(clusterStorage->clusters);
            }
            break;

        default:
            fprintf(stderr, "Error: Something is wrong\n");
    }
}

//functions for qsort compare
int compareFlowsID(const void* a, const void* b)
{
    //we are sorting flows by flowIDs, so we compare them
    int arg1 = ((const flow*)a)->flowID;
    int arg2 = ((const flow*)b)->flowID;

    if (arg1 < arg2) return -1;
    if (arg1 > arg2) return 1;
    return 0;
}

int compareClustersID(const void* a, const void* b)
{
    //we are sorting clusters by smallest first flow's ID (we assume, that flows in cluster were sorted)
    int arg1 = ((const cluster*)a)->flowArr[0].flowID;
    int arg2 = ((const cluster*)b)->flowArr[0].flowID;

    if (arg1 < arg2) return -1;
    if (arg1 > arg2) return 1;
    return 0;
}

//sort flow array by smallest flowID
void sortFlowsByID(flow* flowArr, int flowCount)
{
    //just use qsort function
    qsort(flowArr, flowCount, sizeof(flow), compareFlowsID);
}

//sort flow array by smallest flowID
void sortClustersByID(cluster* clusters, int clusterCount)
{
    //just use qsort function
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
    //conversion of one of the arguments to double is essential, since if not we will receive integer,
    //but there is absolutely no sense to store any of these as double
    return (double)totalBytes/packetCount;
}

//initialises flow with entered params
flow initFlow(int flowID, int totalBytes, int flowDuration, int packetCount, double avgInterarrivalTime)
{
    //just creating new flow type variable and placing values in it
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
    //create cluster type variable
    cluster cluster;

    //alloc memory for given flow count
    cluster.flowCount = flowCount;
    flow* tmp = malloc(sizeof(flow)*flowCount);

    //unsuccessful allocation check
    if (tmp == NULL)
    {
        fprintf(stderr, "Error alloc failed\n");
        cluster.flowCount = -1;
    }
    else
    {
        //if successful store pointer and place all data from given flow array to flow array inside the cluster
        cluster.flowArr = tmp;

        //sorts flows by their ID
        sortFlowsByID(flows, flowCount);
        for (int i = 0; i < flowCount; i++)
        {
            cluster.flowArr[i] = flows[i];
        }
    }
    return cluster;
}

//creates cluster storage from given clusters and their count
clusterStorage initClusterStorage(cluster clusters[], int clusterCount)
{
    //create empty cluster storage
    clusterStorage storage;

    //allocate memory for given cluster count
    storage.clusterCount = clusterCount;
    cluster* tmp = malloc(sizeof(cluster)*clusterCount);

    //unsuccessful allocation check
    if (tmp == NULL)
    {
        fprintf(stderr, "Error alloc failed\n");
        storage.clusterCount = -1;
    }
    else
    {
        //if successful store pointer and store all clusters given in array
        storage.clusters = tmp;
        for (int i = 0; i < clusterCount; i++)
        {
            storage.clusters[i] = clusters[i];
        }
    }
    return storage;
}

//TODO - Range storage init

//unites 2 clusters
cluster uniteClusters(cluster clusterA, cluster clusterB)
{
    //create array for storing flows by flowCounts from both clusters
    flow flows[clusterA.flowCount + clusterB.flowCount];

    //placing all flows from clusterA to united flow array
    for (int i = 0; i < clusterA.flowCount; i++)
    {
        flows[i] = clusterA.flowArr[i];
    }
    //placing all flows from clusterB to united flow array
    for (int i = 0; i < clusterB.flowCount; i++)
    {
        flows[i+clusterA.flowCount] = clusterB.flowArr[i];
    }
    return initCluster(flows, clusterA.flowCount + clusterB.flowCount);
}

//prepares cluster for delete
void prepareForDelete(cluster* cluster)
{
    //frees flow array of cluster
    free(cluster->flowArr);

    //replaces pointer with NULL
    cluster->flowArr = NULL;

    //marks cluster as empty changing its flowCount with -1
    cluster->flowCount = -1;
}

//unites 2 clusters and deletes originals
int uniteAndDelete(clusterStorage* storage, cluster *clusterA, cluster *clusterB)
{
    //call function which creates united cluster
    cluster unitedCluster = uniteClusters(*clusterA, *clusterB);

    if (unitedCluster.flowCount == -1)
    {
        return 1;
    }

    //prepares united clusters for deletion
    prepareForDelete(clusterA);
    prepareForDelete(clusterB);

    //traverses cluster storage
    for (int i = (storage->clusterCount)-1; i >= 0; i--)
    {
        //if finds empty cluster, deletes it leaving no empty places in array
        if (storage->clusters[i].flowCount == -1)
        {
            for (int n = i; n < (storage->clusterCount)-1; n++)
            {
                storage->clusters[n] = storage->clusters[n+1];
            }
        }
    }

    //update clusterCount
    (storage->clusterCount)--;

    //append united cluster to end of array
    storage->clusters[(storage->clusterCount)-1] = unitedCluster;

    //realloc to new address
    cluster *tmp = realloc(storage->clusters, sizeof(cluster)*storage->clusterCount);
    //unsuccessful allocation check
    if (tmp == NULL)
    {
        fprintf(stderr, "Error alloc failed\n");
        return 1;
    }
    else
    {
        storage->clusters = tmp;
    }
    return 0;
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


//TODO - Range storage fill and pull

//finds closest range between 2 clusters
double findClosestRange(cluster netDotClusterA, cluster netDotClusterB, weights weights)
{
    //finds closest possible range between 2 clusters by checking ranges between every flow pair
    double closestFoundRange = INFINITY;
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
int findClosestAndUnite(clusterStorage* storage, weights weights1)
{
    //preparing container for storing 2 indexes of clusters in cluster storage
    int closestInx[2];
    double closestFoundRange = INFINITY;

    //searches for closest pair
    for (int i = 0; i < storage->clusterCount-1; i++)
    {
        for (int j = i+1; j < storage->clusterCount; j++)
        {
            if(closestFoundRange > findClosestRange(storage->clusters[i], storage->clusters[j], weights1))
            {
                closestFoundRange = findClosestRange(storage->clusters[i], storage->clusters[j], weights1);;
                closestInx[0] = i;
                closestInx[1] = j;
            }
        }
    }
    //unites found closest pair and appends it to cluster storage, and checks, if everything is ok
    if (uniteAndDelete(storage, &storage->clusters[closestInx[0]], &storage->clusters[closestInx[1]]) != 0)
    {
        return 1;
    }
    return 0;
}

//finds and unites clusters until their number reaches wanted count
int uniteToNGroups(int destClusterCount, clusterStorage* storage, weights weights)
{
    //checking if destination cluster count is smaller or equal too actual cluster count
    if (destClusterCount > storage->clusterCount)
    {
        finishProgram(afterRead, 1, 0, storage);
        return 1;
    }
    //if start count of clusters and destinations ones are not same starts cycle which finds and unites cluster
    //to the point when destination is reached
    if (destClusterCount != storage->clusterCount)
    {
        do
        {
            if (findClosestAndUnite(storage, weights) != 0)
            {
                return 1;
            }
        }
        while (destClusterCount != storage->clusterCount);
    }

    //sorts clusters in storage
    sortClustersByID(storage->clusters, storage->clusterCount);
    return 0;
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

//controlls if IP is relevant
int controlIP(FILE* srcFile)
{
    int tmpIP[4];

    for (int i = 0; i < 2; i++)
    {
        if (fscanf(srcFile, "%i.%i.%i.%i ", &tmpIP[0], &tmpIP[1], &tmpIP[2], &tmpIP[3]) != 4)
            return 1;

        for (int n = 0; n < 4; n++)
        {
            if (!(tmpIP[n] >= 0 && tmpIP[n] < 256))
                return 1;
        }
    }
    return 0;
}

void prepareClusterArrForDeletion(cluster *clusterArr, int allocClustersCount)
{
    for (int j = 0; j < allocClustersCount; j++)
    {
        prepareForDelete(&clusterArr[j]);
    }
}

//creates cluster from source file
int collectInfoFromSourceFile(FILE* srcFile, clusterStorage *clusterStorage)
{
    //init all essential variables for temporary storing data
    int currClusterCount;
    int flowID;
    int totalBytes;
    int flowDuration;
    int packetCount;
    double avgInterarrivalTime;

    //finds start cluster count
    if (fscanf(srcFile, "count=%i\n", &currClusterCount) != 1)
    {
        return 1;
    }

    //init tmp cluster storage
    cluster tmpClusterArr[currClusterCount];

    //all clusters in start have only 1 flow, but intCluster function requires array,
    //so we just sore single number in array form
    flow flows[1];

    //traverses left source file lines scanning every line and storing important info to tmp variables
    for (int i = 0; i < currClusterCount; i++)
    {
        if (fscanf(srcFile, "%i ", &flowID) != 1)
        {
            finishProgram(fileRead, 1, srcFile, 0);
            prepareClusterArrForDeletion(tmpClusterArr, i);
            return 1;
        }

        if (flowID < 0)
        {
            finishProgram(fileRead, 1, srcFile, 0);
            prepareClusterArrForDeletion(tmpClusterArr, i);
            return 1;
        }

        if (controlIP(srcFile) == 1)
        {
            finishProgram(fileRead, 1, srcFile, 0);
            prepareClusterArrForDeletion(tmpClusterArr, i);
            return 1;
        }

        if (fscanf(srcFile, "%i %i %i %lf\n", &totalBytes, &flowDuration, &packetCount, &avgInterarrivalTime) != 4)
        {
            finishProgram(fileRead, 1, srcFile, 0);
            prepareClusterArrForDeletion(tmpClusterArr, i);
            return 1;
        }

        //inits flow from tmp variables
        flows[0] =  initFlow(flowID, totalBytes, flowDuration, packetCount, avgInterarrivalTime);

        //creates cluster and appends it to temporary array
        tmpClusterArr[i] = initCluster(flows, 1);

        if (tmpClusterArr[i].flowCount == -1)
        {
            finishProgram(fileRead, 1, srcFile, 0);
            prepareClusterArrForDeletion(tmpClusterArr, i);
            return 1;
        }
    }
    //closes file
    fclose(srcFile);

    //returns results in form of cluster storage
    *clusterStorage = initClusterStorage(tmpClusterArr, currClusterCount);

    return 0;
}

//the place where every function's call starts
int main(int argc, char* argv[])
{
    //check if argument number is correct, if not stops program with error
    if (argc != 7)
    {
        finishProgram(inputProcessing, 1, 0, 0);
        return 1;
    }

    //inits weights united storage and stores all given data in it
    weights weights;
    char *endptr;
    weights.bytes = strtod(argv[3], &endptr);
    weights.duration = strtod(argv[4], &endptr);
    weights.interTime = strtod(argv[5], &endptr);
    weights.interLength = strtod(argv[6], &endptr);

    //stores destination cluster count
    int destClusterCount = atoi(argv[2]);

    if (destClusterCount <= 0)
    {
        finishProgram(inputProcessing, 1, 0, 0);
        return 1;
    }

    //open file name of which was given
    FILE* srcFile = fopen(argv[1], "r");

    //if not opened not stops program with error
    if (srcFile == NULL)
    {
        finishProgram(fileOpen, 1, 0, 0);
        return 1;
    }

    //forms cluster storage from source file
    clusterStorage clusterStorage;

    if (collectInfoFromSourceFile(srcFile, &clusterStorage) == 1)
    {
        return 1;
    }

    //checking if clusterCStorage was properly allocated
    if (clusterStorage.clusterCount == -1)
    {
        finishProgram(afterRead, 1, srcFile, &clusterStorage);
        return 1;
    }

    //starts uniting process
    if (uniteToNGroups(destClusterCount, &clusterStorage, weights) != 0)
    {
        finishProgram(afterRead, 1, srcFile, &clusterStorage);
        return 1;
    }

    //prints out info about clusters
    infoOut(clusterStorage);

    finishProgram(afterRead, 0, srcFile, &clusterStorage);

    return 0;
}
