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
N -  Number of clusters we want to get<br>
WB - Weight for totalBytes<br>
WT - Weight for flowDuration<br>
WD - Weight for averageInterTime<br>
WS - Weight for averageInterLength<br>
