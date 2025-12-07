@Command for compiling:
```
cc -std=c11 -Wall -Wextra -Werror -pedantic flows.c -o flows -lm
```
@Command for running the program:
```
./flows FILENAME N WB WT WD WS
```
**Where**:

FINENAME - Name of file where text data about flows is located<br>
N -  Number of clusters we want to get

WB - Weight for totalBytes

WT - Weight for flowDuration

WD - Weight for averageInterTime

WS - Weight for averageInterLength
