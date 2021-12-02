#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

/* constants defined for each possible command */
#define REQUEST 0
#define RELEASE 1
#define LIST_AVAILABLE 2
#define LIST_ASSIGNED 3
#define FIND 4

/* algorithms */
#define BESTFIT 0
#define FIRSTFIT 1
#define NEXTFIT 2
#define WORSTFIT 3

#define MAX_PROCESSES 50

typedef struct
{
    int command;
    char processName[10];
    int requiredMem;
} Command;

typedef struct
{
    char processName[10];
    int memoryUsed;
    int position;
} Process;

typedef struct
{
    Process processes[MAX_PROCESSES];
    int nextIndex;
    int algo;
    int totalSpace;
    int spaceAvailable;
    int nextFitAdress;
} Memory;

/* returns the corresponding int for the given algorithm string */
int strToAlgo(char *algoName)
{
    if(strcmp(algoName, "BESTFIT") == 0)
    {
        return 0;
    }
    else if(strcmp(algoName, "FIRSTFIT") == 0)
    {
        return 1;
    }
    else if(strcmp(algoName, "NEXTFIT") == 0)
    {
        return 2;
    }
    else if(strcmp(algoName, "WORSTFIT") == 0)
    {
        return 3;
    }

    return -1;
}

/* uses fscanf() to find the total number of commands (ignoring comments) so commandArray doesnt have garbage values */
int getNumCommands(char *fileName)
{
    int commandCount = 0;

    FILE *filePointer = fopen(fileName, "r");
    char word[10];

    while(fscanf(filePointer, "%s ", word) != EOF)
    {
        if(strcmp(word, "REQUEST") == 0)
        {
            commandCount++;

            /* skips next 2 words */
            fscanf(filePointer, "%s ", word);
            fscanf(filePointer, "%s ", word);
        }
        else if(strcmp(word, "RELEASE") == 0 || strcmp(word, "LIST") == 0 || strcmp(word, "FIND") == 0)
        {
            commandCount++;

            /* skips next word */
            fscanf(filePointer, "%s ", word);
        }
    }

    fclose(filePointer);
    return commandCount;
}

/* uses fscanf() with format "%s " to get each word in file, then creates the corresponding command object and adds to array */
void parseCommands(char *fileName, Command *commandArray)
{
    int commandCount = 0;

    FILE *filePointer = fopen(fileName, "r");
    char word[10];
    char nullWord[10] = "NULL"; // for commands that dont require a process name (to avoid uninitilized garbage ╨@)

    while(fscanf(filePointer, "%s ", word) != EOF)
    {
        if(strcmp(word, "REQUEST") == 0)
        {
            commandArray[commandCount].command = REQUEST;
            fscanf(filePointer, "%s ", word);
            for(int i = 0; i < 10; i++)
                commandArray[commandCount].processName[i] = word[i];
            fscanf(filePointer, "%s ", word);
            commandArray[commandCount].requiredMem = (int)strtol(word, (char **)NULL, 10); // strtol returns a long from the given string in base 10
            commandCount++;
        }
        else if(strcmp(word, "RELEASE") == 0)
        {
            commandArray[commandCount].command = RELEASE;
            fscanf(filePointer, "%s ", word);
            for(int i = 0; i < 10; i++)
                commandArray[commandCount].processName[i] = word[i];
            commandArray[commandCount].requiredMem = 0;
            commandCount++;
        }
        else if(strcmp(word, "LIST") == 0)
        {
            fscanf(filePointer, "%s ", word);

            if(strcmp(word, "AVAILABLE") == 0)
            {
                commandArray[commandCount].command = LIST_AVAILABLE;
                for(int i = 0; i < 10; i++)
                    commandArray[commandCount].processName[i] = nullWord[i];
                commandArray[commandCount].requiredMem = 0;
            }
            else
            {
                commandArray[commandCount].command = LIST_ASSIGNED;
                for(int i = 0; i < 10; i++)
                    commandArray[commandCount].processName[i] = nullWord[i];
                commandArray[commandCount].requiredMem = 0;
            }
            commandCount++;
        }
        else if(strcmp(word, "FIND") == 0)
        {
            commandArray[commandCount].command = FIND;
            fscanf(filePointer, "%s ", word);
            for(int i = 0; i < 10; i++)
                commandArray[commandCount].processName[i] = word[i];
            commandArray[commandCount].requiredMem = 0;
            commandCount++;
        }
    }
    fclose(filePointer);
}

/* returns a new hole with given parameters */
Process newHole(int mem, int pos)
{
    Process hole;
    memcpy(hole.processName, "HOLE", sizeof(hole.processName));
    hole.memoryUsed = mem;
    hole.position = pos;

    return hole;
}

void removeIndex(Memory *memory, int avoid)
{
    Process newProcesses[MAX_PROCESSES];
    int count = 0;

    for(int i = 0; i < memory->nextIndex; i++)
    {
        if(i != avoid)
        {
            newProcesses[count] = memory->processes[i];
            count++;
        }
    }

    memcpy(memory->processes, &newProcesses, sizeof(memory->processes));
    memory->nextIndex = count;

    combineHoles(memory);
}

void combineHoles(Memory *memory)
{
    for(int i = 0; i < memory->nextIndex; i++)
    {
        if(strcmp(memory->processes[i].processName, "HOLE") == 0 && strcmp(memory->processes[i+1].processName, "HOLE") == 0)
        {
            memory->processes[i].memoryUsed = memory->processes[i].memoryUsed + memory->processes[i+1].memoryUsed;
            removeIndex(memory, i+1);
        }
    }
}

/* bubble sort helper function */
void swap(Process *a, Process *b)
{
    Process temp = *a;
    *a = *b;
    *b = temp;
}

/* standard bubble sort */
void sort(Memory *memory) {
	for(int i = 0; i < memory->nextIndex -1; i++)
    {
		for(int j = 0; j < memory->nextIndex - i -1; j++)
        {
			if(memory->processes[j].position > memory->processes[j+1].position)
            {
				swap(&memory->processes[j], &memory->processes[j+1]);
			}
		}
	}
}

/* creates new process from hole and appends new hole to memory */
void holeToProcess(char newName[], int requiredMem, Memory *memory, int i)
{
    int memDifference = memory->processes[i].memoryUsed - requiredMem;

    memcpy(memory->processes[i].processName, newName, sizeof(memory->processes[i].processName));
    memory->processes[i].memoryUsed = requiredMem;

    if(memDifference == 0)
    {
        return;
    }
    // if hole had left over space, make new hole with memDifference at position (old position + memory taken up)
    memory->processes[memory->nextIndex] = newHole(memDifference, memory->processes[i].position + requiredMem);
    memory->nextIndex++;
    memory->spaceAvailable = memory->spaceAvailable - requiredMem;
}

/* finds smallest hole that will fit process */
void bestfit(char processName[], int requiredMem, Memory *memory)
{
    int smallestSize = memory->totalSpace+1; // largest possible size
    int smallestIndex = -1;

    for(int i = 0; i < memory->nextIndex; i++)
    {
        if(strcmp(memory->processes[i].processName, "HOLE") == 0)
        {
            if((memory->processes[i].memoryUsed >= requiredMem) && (memory->processes[i].memoryUsed < smallestSize))
            {
                smallestSize = memory->processes[i].memoryUsed;
                smallestIndex = i;
            }
        }
    }

    if(smallestIndex > -1)
    {
        holeToProcess(processName, requiredMem, memory, smallestIndex);
        printf("ALLOCATED %s %d\n", processName, memory->processes[smallestIndex].position);
        return;
    }

    printf("FAIL REQUEST %s %d\n", processName, requiredMem);
    return;
}

/* finds first hole that will fit process */
void firstfit(char processName[], int requiredMem, Memory *memory)
{
    for(int i = 0; i < memory->nextIndex; i++)
    {
        if(strcmp(memory->processes[i].processName, "HOLE") == 0)
        {
            if(memory->processes[i].memoryUsed >= requiredMem)
            {
                holeToProcess(processName, requiredMem, memory, i);
                printf("ALLOCATED %s %d\n", processName, memory->processes[i].position);
                return;
            }
        }
    }

    printf("FAIL REQUEST %s %d\n", processName, requiredMem);
    return;
}

/* determines if there is a hole big enough for nextfit, prevents infinite reccursion */
int isPossible(int requiredMem, Memory *memory)
{
    int possibleHoles = 0;

    for(int i = 0; i < memory->nextIndex; i++)
    {
        if(strcmp(memory->processes[i].processName, "HOLE") == 0 && memory->processes[i].memoryUsed >= requiredMem)
        {
            possibleHoles++;
        }
    }

    return possibleHoles;
}

/* helper function that returns index of process with given adress */
int getIndexFromAdress(int adress, Memory *memory)
{
    for(int i = 0; i < memory->nextIndex; i++)
    {
        if(memory->processes[i].position == adress)
        {
            return i;
        }
    }
    return -1;
}

/* finds first hole that will fit process, starting from given index */
void nextfit(char processName[], int requiredMem, Memory *memory)
{
    if(isPossible(requiredMem, memory) > 0)
    {
        for(int i = getIndexFromAdress(memory->nextFitAdress, memory); i < memory->nextIndex; i++)
        {
            if(strcmp(memory->processes[i].processName, "HOLE") == 0)
            {
                if(memory->processes[i].memoryUsed >= requiredMem)
                {
                    holeToProcess(processName, requiredMem, memory, i);
                    memory->nextFitAdress = memory->processes[i].position + requiredMem;
                    printf("ALLOCATED %s %d\n", processName, memory->processes[i].position);
                    return;
                }
            }
        }

        memory->nextFitAdress = 0;
        nextfit(processName, requiredMem, memory);
    }
    printf("FAIL REQUEST %s %d\n", processName, requiredMem);
    return;
}

/* finds largest hole that will fit process */
void worstfit(char processName[], int requiredMem, Memory *memory)
{
    int largestSize = -1;
    int largestIndex = -1;

    for(int i = 0; i < memory->nextIndex; i++)
    {
        if(strcmp(memory->processes[i].processName, "HOLE") == 0)
        {
            if((memory->processes[i].memoryUsed >= requiredMem) && (memory->processes[i].memoryUsed > largestSize))
            {
                largestSize = memory->processes[i].memoryUsed;
                largestIndex = i;
            }
        }
    }

    if(largestIndex > -1)
    {
        holeToProcess(processName, requiredMem, memory, largestIndex);
        printf("ALLOCATED %s %d\n", processName, memory->processes[largestIndex].position);
        return;
    }

    printf("FAIL REQUEST %s %d\n", processName, requiredMem);
    return;
}

/* calls the correct request function based on the given algorithm */
void request(char processName[], int requiredMem, Memory *memory)
{
    if(memory->spaceAvailable < requiredMem)
    {
        printf("FAIL REQUEST %s %d\n", processName, requiredMem);
        return;
    }

    switch(memory->algo)
    {
        case BESTFIT:
            bestfit(processName, requiredMem, memory);
            break;
        case FIRSTFIT:
            firstfit(processName, requiredMem, memory);
            break;
        case NEXTFIT:
            nextfit(processName, requiredMem, memory);
            break;
        case WORSTFIT:
            worstfit(processName, requiredMem, memory);
            break;
    }
}

/* turns process into a hole */
void release(char processName[], Memory *memory)
{
    for(int i = 0; i < memory->nextIndex; i++)
    {
        if(strcmp(memory->processes[i].processName, processName) == 0)
        {
            memcpy(memory->processes[i].processName, "HOLE", sizeof(memory->processes[i].processName));
            memory->spaceAvailable = memory->spaceAvailable + memory->processes[i].memoryUsed;
            printf("FREE %s %d %d\n", processName, memory->processes[i].memoryUsed, memory->processes[i].position);
            return;
        }
    }
    printf("FAIL RELEASE %s\n", processName);
    return;
}

/* lists all holes size and adress */
void listAvailable(Memory *memory)
{
    int available = 0;
    for(int i = 0; i < memory->nextIndex; i++)
    {
        if(strcmp(memory->processes[i].processName, "HOLE") == 0)
        {
            printf("(%d, %d) ", memory->processes[i].memoryUsed, memory->processes[i].position);
            available++;
        }
    }

    if(available > 0)
    {
        printf("\n");
        return;
    }
    else
    {
        printf("FULL\n");
        return;
    }
}

/* list all processes currently in memory */
void listAssigned(Memory *memory)
{
    int assigned = 0;
    for(int i = 0; i < memory->nextIndex; i++)
    {
        if(!(strcmp(memory->processes[i].processName, "HOLE") == 0))
        {
            printf("(%s, %d, %d) ", memory->processes[i].processName, memory->processes[i].memoryUsed, memory->processes[i].position);
            assigned++;
        }
    }

    if(assigned > 0)
    {
        printf("\n");
        return;
    }
    else
    {
        printf("NONE\n");
        return;
    }
}

/* prints specific process, pretty sure this is never called */
void find(char processName[], Memory *memory)
{
    for(int i = 0; i < memory->nextIndex; i++)
    {
        if(strcmp(memory->processes[i].processName, processName) == 0)
        {
            printf("(%s, %d, %d)\n", processName, memory->processes[i].memoryUsed, memory->processes[i].position);
            return;
        }
    }
    printf("FAULT\n");
    return;
}

/* calls corresponding function based on the command */
void run(Command command, Memory *memory)
{
    switch(command.command)
    {
        case REQUEST:
            request(command.processName, command.requiredMem, memory);
            break;
        case RELEASE:
            release(command.processName, memory);
            break;
        case LIST_AVAILABLE:
            listAvailable(memory);
            break;
        case LIST_ASSIGNED:
            listAssigned(memory);
            break;
        case FIND:
            find(command.processName, memory);
            break;
        default:
            printf("BAD COMMAND\n");
            break;
    }
    sort(memory);
    combineHoles(memory);
}

/* creates a hole process at the start of memory */
void initMemory(Memory *memory)
{
    memory->processes[0] = newHole(memory->totalSpace, 0);
    memory->nextIndex = 1;
    memory->spaceAvailable = memory->totalSpace;
    memory->nextFitAdress = 0;
}

int main(int argc, char **argv)
{
    if (argc != 4)
    {
        printf("Not enough arguments.\n./project2 <algorithm> <total memory> <script>\n");
    }

    Memory memory;
    memory.algo = strToAlgo(argv[1]);
    memory.totalSpace = (int)strtol(argv[2], (char **)NULL, 10);
    initMemory(&memory);

    char *fileName = argv[3];

    int numOfCommands = getNumCommands(fileName);

    Command commandArray[numOfCommands];
    parseCommands(fileName, commandArray);

    for(int i = 0; i < numOfCommands; i++)
    {
        run(commandArray[i], &memory);
    }

    return 0;
}
