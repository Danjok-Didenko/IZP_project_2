#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>
#include <stdbool.h>

/** Flows v1LRS.1 (version with local range storing)
 *  Created by Daniil Didenko
 *  xlogin: xdidend00
 *
 *  *   *   *   *   *   *   *   USAGE   *   *   *   *   *   *   *   *   *
 *                                                                      *
 *  cc -std=c11 -Wall -Wextra -Werror -pedantic flows.c -o flows -lm    *
 *  $ ./flows FILENAME N WB WT WD WS                                    *
 *                                                                      *
 *  *   *   *   *   *   *   *   *   *   *   *   *   *   *   *   *   *   *
 *
 *  @param FINENAME - Name of file where text data about flows is located
 *  @param N -  Number of clusters we want to get
 *  @param WB - Weight for totalBytes.
 *  @param WT - Weight for flowDuration.
 *  @param WD - Weight for averageInterTime.
 *  @param WS - Weight for averageInterLength
 *
 */

//program stages written in text for easier use
enum programStage
{
    inputProcessing,
    fileOpen,
    fileRead,
    clusterWrite,
    afterRead
};

typedef struct SRange
{
    int flowID;
    double range;
}Range;

// structure for storing all flow's arguments
typedef struct SFlow
{
    int flowID;
    int totalBytes;
    int flowDuration;
    double avgInterTime;
    double avgInterLength;
}Flow;

// structure for storing all clusters flows as well as flow's count for more convenient use
typedef struct SCluster
{
    int flowCount;
    int rangeCount;
    Range* ranges;
    Flow* flows;
}Cluster;

// structure for storing all clusters as well as cluster count for more convenient use in functions
typedef struct SClusterStorage
{
    int clusterCount;
    Cluster* clusters;
}ClusterStorage;

// structure for storing all user-entered weights in one place
// for more convenient usage in functions
typedef struct SWeights
{
    double bytes;
    double duration;
    double interTime;
    double interLength;
}Weights;

// function declaration (used only here for 1 purpose)
void prepareForDelete(Cluster* cluster, bool rangesCalculated);

void freeAll(ClusterStorage* storage, bool rangesCalculated)
{
    // prepares all clusters in cluster storage for deletion
    for (int i = 0; i < storage->clusterCount; i++)
    {
        prepareForDelete(&(storage->clusters[i]), rangesCalculated);
    }

    // frees cluster storage if it was inited
    if (storage->clusterCount !=-1)
    {
        free(storage->clusters);
    }
}

// function for finishing program
void finishProgram(int programStage, bool isError, FILE* srcFile, ClusterStorage *storage, bool rangesCalculated)
{
    // was we need to do to finish the program depends on program stage
    // at which program should be finished
    switch (programStage)
    {
        case inputProcessing:
            fprintf(stderr, "ERROR: Something is wrong with entered arguments\n");
            break;
        case fileOpen:
            fprintf(stderr, "ERROR: File failed to read\n");
            break;
        case fileRead:
            fclose(srcFile);
            fprintf(stderr, "ERROR: Something is wrong with input file\n");
            break;
        case afterRead:
            // since it is the final stage we need to clarify
            // that program just have finished or some ERROR appeared
            if (isError)
            {
                fprintf(stderr, "ERROR: Some allocation failed\n");
            }
            freeAll(storage, rangesCalculated);
            break;

        default:
            fprintf(stderr, "ERROR: Something is wrong\n");
    }
}

// functions for qsort compare
int compareFlowsID(const void* a, const void* b)
{
    // we are sorting flows by flowIDs, so we compare them
    int arg1 = ((const Flow*)a)->flowID;
    int arg2 = ((const Flow*)b)->flowID;

    if (arg1 < arg2) return -1;
    if (arg1 > arg2) return 1;
    return 0;
}

int compareClustersID(const void* a, const void* b)
{
    // we are sorting clusters by smallest first flow's ID
    // (we assume, that flows in cluster were sorted)
    int arg1 = ((const Cluster*)a)->flows[0].flowID;
    int arg2 = ((const Cluster*)b)->flows[0].flowID;

    if (arg1 < arg2) return -1;
    if (arg1 > arg2) return 1;
    return 0;
}

int compareClustersFlowCount(const void* a, const void* b)
{
    // we are sorting clusters by biggest first flow's ID
    // (we assume, that flows in cluster were sorted)
    int arg1 = ((const Cluster*)a)->flowCount;
    int arg2 = ((const Cluster*)b)->flowCount;

    if (arg1 < arg2) return 1;
    if (arg1 > arg2) return -1;
    return 0;
}

int compareRanges(const void* a, const void* b)
{
    // comparing ranges
    double arg1 = ((const Range*)a)->range;
    double arg2 = ((const Range*)b)->range;

    if (arg1 < arg2) return -1;
    if (arg1 > arg2) return 1;
    return 0;
}

int compareClustersByRange(const void* a, const void* b)
{
    // comparing clusters by smallest ranges
    double arg1 = ((const Cluster*)a)->ranges[0].range;
    double arg2 = ((const Cluster*)b)->ranges[0].range;

    if (arg1 < arg2) return -1;
    if (arg1 > arg2) return 1;
    return 0;
}

// sort flow array by smallest flowID
void sortFlowsByID(Flow* flowArr, int flowCount)
{
    // just use qsort function
    qsort(flowArr, flowCount, sizeof(Flow), compareFlowsID);
}

// sort flow array by smallest flowID
void sortClustersByID(Cluster* clusters, int clusterCount)
{
    // just use qsort function
    qsort(clusters, clusterCount, sizeof(Cluster), compareClustersID);
}

// sort flow array by biggest flowCount
void sortClustersFlowCount(Cluster* clusters, int clusterCount)
{
    // just use qsort function
    qsort(clusters, clusterCount, sizeof(Cluster), compareClustersFlowCount);
}

// sort ranges from smallest to biggest
void sortRanges(Range* ranges, int rangeCount)
{
    qsort(ranges, rangeCount, sizeof(Range), compareRanges);
}

// sorting ranges in cluster
void sortRangesInCluster(Cluster* cluster)
{
    qsort(cluster->ranges, cluster->rangeCount, sizeof(Range), compareRanges);
}

// sorting clusters by smallest range
// (we assume that ranges inside clusters were already sorted)
void sortClustersByRange(ClusterStorage* storage)
{
    qsort(storage->clusters, storage->clusterCount, sizeof(Cluster), compareClustersByRange);
}

// Mathematical custom functions
// -------------------------------------------------------------------------------------

// calculates square number for double type variable
double squareFloat(double a)
{
    return a*a;
}

// calculates square number for int type variable
int squareInt(int a)
{
    return a*a;
}

// Functions for working with flows and clusters
// -------------------------------------------------------------------------------------

// calculates average interarrival length
double calculateAvgInterLength(int totalBytes, int packetCount)
{
    // conversion of one of the arguments to double is essential,
    // since if not we will receive integer,
    // but there is absolutely no sense to store any of these as double
    return (double)totalBytes/packetCount;
}

// initialises flow with entered params
Flow initFlow(int flowID, int totalBytes, int flowDuration, int packetCount, double avgInterarrivalTime)
{
    // just creating new flow type variable and placing values in it
    Flow flow;
    flow.flowID = flowID;
    flow.totalBytes = totalBytes;
    flow.flowDuration = flowDuration;
    flow.avgInterTime = avgInterarrivalTime;
    flow.avgInterLength = calculateAvgInterLength(totalBytes, packetCount);

    return flow;
}

// creates cluster with given flows and given number
Cluster initCluster(Flow flows[], int flowCount)
{
    // create cluster type variable
    Cluster cluster;

    // alloc memory for given flow count
    cluster.flowCount = flowCount;
    Flow* tmp = malloc(sizeof(Flow)*flowCount);

    // unsuccessful allocation check
    if (tmp == NULL)
    {
        fprintf(stderr, "ERROR alloc failed\n");
        cluster.flowCount = -1;
    }
    else
    {
        // if successful store pointer and place all data from
        // given flow array to flow array inside the cluster
        cluster.flows = tmp;

        // sorts flows by their ID
        sortFlowsByID(flows, flowCount);
        for (int i = 0; i < flowCount; i++)
        {
            cluster.flows[i] = flows[i];
        }
    }
    return cluster;
}

// creates cluster storage from given clusters and their count
ClusterStorage initClusterStorage(Cluster clusters[], int clusterCount)
{
    // create empty cluster storage
    ClusterStorage storage;

    // allocate memory for given cluster count
    storage.clusterCount = clusterCount;
    Cluster* tmp = malloc(sizeof(Cluster)*clusterCount);

    // unsuccessful allocation check
    if (tmp == NULL)
    {
        fprintf(stderr, "ERROR alloc failed\n");
        storage.clusterCount = -1;
    }
    else
    {
        // if successful store pointer and store all clusters given in array
        storage.clusters = tmp;
        for (int i = 0; i < clusterCount; i++)
        {
            storage.clusters[i] = clusters[i];
        }
    }
    return storage;
}

// creates range with user-entered parameters
Range initRange(int flowID, double rangeTo)
{
    Range r;
    r.flowID = flowID;
    r.range = rangeTo;
    return r;
}

// unites 2 sets of ranges from 2 clusters and records them to new united cluster
int uniteRangesInClusters(Cluster* clusterA, Cluster* clusterB, Cluster* unitedCluster)
{
    // init tmp variable used inside the function
    int writtenCount = 0;

    // allocating tmp range array for united cluster list
    Range* tmp1 = malloc(sizeof(Range)*(clusterA->rangeCount + clusterB->rangeCount));

    // unsuccessful allocation check
    if (tmp1 == NULL)
    {
        return 1;
    }

    // recording only shortest ranges
    for (int i = 1; i < clusterA->rangeCount; i++)
    {
        for (int j = 1; j < clusterB->rangeCount; j++)
        {
            if (clusterA->ranges[i].flowID == clusterB->ranges[j].flowID)
            {
                if (clusterA->ranges[i].range > clusterB->ranges[j].range)
                {
                    tmp1[writtenCount] = initRange(clusterA->ranges[i].flowID, clusterB->ranges[j].range);
                }
                else
                {
                    tmp1[writtenCount] = initRange(clusterA->ranges[i].flowID, clusterA->ranges[i].range);
                }
                writtenCount++;
                break;
            }
        }
    }

    if (writtenCount == 0)
    {
        unitedCluster->rangeCount = 1;
    }
    else
    {
        unitedCluster->rangeCount = writtenCount;
    }

    // allocating tmp range array for united cluster list
    Range* tmp = realloc(tmp1, sizeof(Range)*(unitedCluster->rangeCount));



    // unsuccessful allocation check
    if (tmp == NULL)
    {
        free(tmp1);
        return 1;
    }

    // putting new range array to united cluster
    unitedCluster->ranges = tmp;

    // sot ranges inside cluster
    sortRangesInCluster(unitedCluster);

    return 0;
}

// unites 2 clusters
Cluster uniteClusters(Cluster clusterA, Cluster clusterB)
{
    // create array for storing flows by flowCounts from both clusters
    Flow flows[clusterA.flowCount + clusterB.flowCount];

    // placing all flows from clusterA to united flow array
    for (int i = 0; i < clusterA.flowCount; i++)
    {
        flows[i] = clusterA.flows[i];
    }
    // placing all flows from clusterB to united flow array
    for (int i = 0; i < clusterB.flowCount; i++)
    {
        flows[i+clusterA.flowCount] = clusterB.flows[i];
    }
    return initCluster(flows, clusterA.flowCount + clusterB.flowCount);
}

// prepares cluster for delete
void prepareForDelete(Cluster* cluster, bool rangesCalculated)
{
    // frees flow array of cluster
    free(cluster->flows);

    // replaces pointer with NULL
    cluster->flows = NULL;

    // marks cluster as empty changing its flowCount with -1
    cluster->flowCount = -1;

    //if ranges were calculated, free range array
    if (rangesCalculated)
    {
        free(cluster->ranges);
        cluster->ranges = NULL;
    }
}

// unites 2 clusters and deletes originals
int uniteAndDelete(ClusterStorage* storage, Cluster *clusterA, Cluster *clusterB)
{
    // call function which creates united cluster
    Cluster unitedCluster = uniteClusters(*clusterA, *clusterB);

    // if united cluster is marked with -1 - break
    if (unitedCluster.flowCount == -1)
    {
        return 1;
    }

    // if error appeared while uniting ranges - break
    if (uniteRangesInClusters(clusterA, clusterB, &unitedCluster) == 1)
    {
        return 1;
    }

    // prepares united clusters for deletion
    prepareForDelete(clusterA, 1);
    prepareForDelete(clusterB, 1);

    // sorting clusters by flowCount from biggest to smallest,
    // so empty clusters marked with flowCount -1 will be in the end
    sortClustersFlowCount(storage->clusters, storage->clusterCount);

    // update clusterCount
    (storage->clusterCount)--;

    // append united cluster to end of array
    storage->clusters[(storage->clusterCount)-1] = unitedCluster;

    // realloc to new address
    Cluster *tmp = realloc(storage->clusters, sizeof(Cluster)*storage->clusterCount);
    // unsuccessful allocation check
    if (tmp == NULL)
    {
        fprintf(stderr, "ERROR alloc failed\n");
        return 1;
    }
    else
    {
        storage->clusters = tmp;
    }
    return 0;
}

// finds range between 2 netDots
double findRange(Flow flowA, Flow flowB, Weights weights)
{
    return sqrt(
    weights.bytes*squareInt(flowA.totalBytes - flowB.totalBytes) +
    weights.duration*squareInt(flowA.flowDuration - flowB.flowDuration) +
    weights.interTime*squareFloat(flowA.avgInterTime - flowB.avgInterTime) +
    weights.interLength* squareFloat(flowA.avgInterLength - flowB.avgInterLength)
    );
}

// calculates and records ranges to dedicated structures for all clusters in given storage
int calculateAndRecordRanges(ClusterStorage* storage, Weights weights)
{
    for (int i = 0; i < storage->clusterCount; i++)
    {
        storage->clusters[i].rangeCount = storage->clusterCount-1;

        // allocating tmp range array
        Range* tmp = malloc(sizeof(Range)*(storage->clusterCount-1));

        // allocation check
        if (tmp == NULL)
        {
            return 1;
        }

        // introducing tmp variable
        int writtenCount = 0;
        for (int n = 0; n < storage->clusterCount; n++)
        {
            // we don't calculate range to self (it's pretty senseless)
            if (n == i)
            {
                continue;
            }
            tmp[writtenCount] = initRange(storage->clusters[n].flows[0].flowID,
                findRange(storage->clusters[n].flows[0], storage->clusters[i].flows[0], weights));
            writtenCount++;
        }

        //recording it to cluster itself
        storage->clusters[i].ranges = tmp;

        //sorting ranges inside cluster
        sortRangesInCluster(&storage->clusters[i]);
    }
    return 0;
}

// finds closest pair of clusters and returns array with united cluster
int findClosestAndUnite(ClusterStorage* storage)
{
    // sorting all clusters by shortest range so first 2 will be the nearest pair
    sortClustersByRange(storage);

    // unites found pair and appends it to cluster storage, and checks, if everything is ok
    if (uniteAndDelete(storage, &storage->clusters[0], &storage->clusters[1]) != 0)
    {
        return 1;
    }
    return 0;
}

// finds and unites clusters until their number reaches wanted count
int uniteToNGroups(int destClusterCount, ClusterStorage* storage, Weights weights)
{
    // checking if destination cluster count is smaller or equal too actual cluster count
    if (destClusterCount > storage->clusterCount)
    {
        finishProgram(afterRead, 0, 0, storage, 0);
        return 1;
    }
    // if start count of clusters and destinations ones are not same
    // starts cycle which finds and unites cluster
    // to the point when destination is reached
    if (destClusterCount != storage->clusterCount)
    {
        if (calculateAndRecordRanges(storage, weights) == 1)
        {
            return 1;
        }
        do
        {
            if (findClosestAndUnite(storage) != 0)
            {
                return 1;
            }
            // printf("%i\n", storage->clusterCount);
        }
        while (destClusterCount != storage->clusterCount);
    }

    // sorts clusters in storage
    sortClustersByID(storage->clusters, storage->clusterCount);
    return 0;
}

// prints info about exact cluster
void clusterOut(Cluster cluster, int clusterInx)
{
    printf("cluster %i: ", clusterInx);

    for (int i = 0; i < cluster.flowCount; i++)
    {
        printf("%i ", cluster.flows[i].flowID);
    }
    printf("\n");
}

// prints info about all clusters
void infoOut(ClusterStorage storage)
{
    printf("Clusters:\n");

    for (int i = 0; i < storage.clusterCount; i++)
    {
        clusterOut(storage.clusters[i], i);
    }
}

// controlls if IP is relevant
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

// prepares clusterArr for deletion
void prepareClusterArrForDeletion(Cluster *clusterArr, int allocClustersCount)
{
    for (int j = 0; j < allocClustersCount; j++)
    {
        prepareForDelete(&clusterArr[j], 0);
    }
}

int collectInfoFromInput(int argc, char* argv[], Weights* weights, int* destClusterCount)
{
    // check if argument number is correct, if not stops program with error
    if (argc != 7)
    {
        return 1;
    }
    *destClusterCount = atoi(argv[2]);

    // stores all given data in weights united storage
    char *endptr;
    weights->bytes = strtod(argv[3], &endptr);
    weights->duration = strtod(argv[4], &endptr);
    weights->interTime = strtod(argv[5], &endptr);
    weights->interLength = strtod(argv[6], &endptr);

    // weights control
    if (weights->bytes < 0 ||
        weights->duration < 0 ||
        weights->interTime < 0 ||
        weights->interLength < 0)
    {
        return 1;
    }

    // verifies if destination cluster count is legit
    if (*destClusterCount <= 0)
    {
        return 1;
    }
    return 0;
}

// creates cluster from source file
int collectInfoFromSourceFile(FILE* srcFile, ClusterStorage *clusterStorage)
{
    // init all essential variables for temporary storing data
    int currClusterCount;
    int flowID;
    int totalBytes;
    int flowDuration;
    int packetCount;
    double avgInterarrivalTime;

    // finds start cluster count
    if (fscanf(srcFile, "count=%i\n", &currClusterCount) != 1)
    {
        return 1;
    }

    // init tmp cluster storage
    Cluster tmpClusterArr[currClusterCount];

    // all clusters in start have only 1 flow, but intCluster function requires array,
    // so we just sore single number in array form
    Flow flows[1];

    // traverses left source file lines scanning every line
    // and storing important info to tmp variables
    for (int i = 0; i < currClusterCount; i++)
    {
        if (fscanf(srcFile, "%i ", &flowID) != 1 || flowID < 0 || controlIP(srcFile) == 1 ||
            fscanf(srcFile, "%i %i %i %lf\n", &totalBytes, &flowDuration, &packetCount, &avgInterarrivalTime) != 4)
        {
            finishProgram(fileRead, 1, srcFile, 0, 0);
            prepareClusterArrForDeletion(tmpClusterArr, i);
            return 1;
        }

        // inits flow from tmp variables
        flows[0] =  initFlow(flowID, totalBytes, flowDuration, packetCount, avgInterarrivalTime);

        // creates cluster and appends it to temporary array
        tmpClusterArr[i] = initCluster(flows, 1);

        if (tmpClusterArr[i].flowCount == -1)
        {
            finishProgram(fileRead, 1, srcFile, 0, 0);
            prepareClusterArrForDeletion(tmpClusterArr, i);
            return 1;
        }
    }
    // closes file
    fclose(srcFile);

    // returns results in form of cluster storage
    *clusterStorage = initClusterStorage(tmpClusterArr, currClusterCount);

    return 0;
}

// the place where every function's call starts
int main(int argc, char* argv[])
{
    // inits weights united storage
    Weights weights;

    // stores destination cluster count
    int destClusterCount = atoi(argv[2]);

    if (collectInfoFromInput(argc, argv, &weights, &destClusterCount) == 1)
    {
        finishProgram(inputProcessing, 1, 0, 0, 0);
        return 1;
    }

    // open file name of which was given
    FILE* srcFile = fopen(argv[1], "r");

    // if not opened not stops program with error
    if (srcFile == NULL)
    {
        finishProgram(fileOpen, 1, 0, 0, 0);
        return 1;
    }

    // forms cluster storage from source file
    ClusterStorage clusterStorage;

    if (collectInfoFromSourceFile(srcFile, &clusterStorage) == 1)
        return 1;

    // checking if clusterCStorage was properly allocated
    if (clusterStorage.clusterCount == -1)
    {
        finishProgram(afterRead, 1, srcFile, &clusterStorage, 0);
        return 1;
    }

    // records if destinationClusterCount is same to initial cluster count
    // for future memory management
    bool isEqualCount = destClusterCount == clusterStorage.clusterCount;

    // starts uniting process
    if (uniteToNGroups(destClusterCount, &clusterStorage, weights) != 0)
    {
        finishProgram(afterRead, 1, 0, &clusterStorage, !isEqualCount);
        return 1;
    }

    // prints out info about clusters
    infoOut(clusterStorage);

    // finishes program (does all frees and exc.)
    finishProgram(afterRead, 0, srcFile, &clusterStorage, !isEqualCount);

    return 0;
}