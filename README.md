@Command for compiling:
```
cc -std=c11 -Wall -Wextra -Werror -pedantic flows.c -o flows -lm
```
@Command for running the program:
```
./flows FILENAME N WB WT WD WS
```
Where:  EOL
@param FINENAME - Name of file where text data about flows is located  EOL
@param N -  Number of clusters we want to get  EOL
@param WB - Weight for totalBytes  EOL
@param WT - Weight for flowDuration  EOL
@param WD - Weight for averageInterTime  EOL
@param WS - Weight for averageInterLength  EOL
