@Command for compiling:
```
cc -std=c11 -Wall -Wextra -Werror -pedantic flows.c -o flows -lm
```
@Command for running the program:
```
./flows FILENAME N WB WT WD WS
```
Where:
@param FINENAME - Name of file where text data about flows is located
@param N -  Number of clusters we want to get
@param WB - Weight for totalBytes.
@param WT - Weight for flowDuration
@param WD - Weight for averageInterTime.
@param WS - Weight for averageInterLength
