#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>
#include "queue.h" 

#define MEMORY 1024

// Represents the global variables
int avail_mem[MEMORY] = {0}; // Array to track available memory
queue_t *temp = NULL; // Temporary queue for incoming processes
queue_t *queues[4]; // Array of queues for each priority level
int currTime = 0; // Current time in the system

// Represents the function prototypes
void readDispatchList();
void checkArrival(int timeNow);
int findFreeMemory(int amountNeeded);
void clearMemory(int startPos, int amount);
void runPriority();
bool keepRunning(int queueLevel);
void runQueueOne(int queueLevel);

int main() {
    // Initializes the queues
    temp = (queue_t *)malloc(sizeof(queue_t));
    temp->next = NULL;

    for(int i = 0; i < 4; i++) {
        queues[i] = (queue_t *)malloc(sizeof(queue_t));
        queues[i]->next = NULL;
    }

    // Reads the dispatch list
    readDispatchList();
    
    // Start processing
    int startTime = temp->next->process.arrivalTime; // Get arrival time of first process
    checkArrival(startTime); // Move processes from temp queue to appropriate priority queue
    
    // Main processing loop
    while(queues[0]->next != NULL || queues[1]->next != NULL || queues[2]->next != NULL || queues[3]->next != NULL) {
        // Runs any existing priority processes
        if(queues[0]->next != NULL) {
            runPriority();
            continue;
        }

        // Run processes in other queues based on priority
        for(int i = 1; i < 4; i++) {
            if(queues[i]->next != NULL) {
                runQueueOne(i);
                break; // Break after processing one queue to ensure priority order
            }
        }
    }

    // Frees the allocated memory
    free(temp);
    for(int i = 0; i < 4; i++) {
        free(queues[i]);
    }

    return 0;
}

// Reads from the dispatch list and add to temp queue
void readDispatchList() {
    proc_t newProc;
    FILE *fptr;
    int i = 0;
    fptr = fopen("dispatchlist", "r"); // Open dispatch list file for reading
    char lines[10][256]; // Array to store lines from file
    char currline[256]; // Buffer to store current line
    
    // Read each line from file
    while (fgets(currline, 256, fptr) != NULL){
        strncpy(&lines[i], currline, sizeof(currline)); 
        i++;
    }
    fclose(fptr); // Close file after reading

    // Adds the items to a new proc, then add the new proc to the temp queue
    for (i = 0; i < 10; i++) {
        char *token;
        int arrivalTime;
        int priority;
        int processorTime;
        int memory;
        int printers;
        int scanners;
        int modems;
        int cds;
        memset(&newProc, 0, sizeof newProc); // Clears the newProc structure

        // Tokenize line to extract process attributes
        token = strtok(&lines[i], ", ");
        arrivalTime = atoi(token);
        token = strtok(NULL, ", ");
        priority = atoi(token);
        token = strtok(NULL, ", ");
        processorTime = atoi(token);
        token = strtok(NULL, ", ");
        memory = atoi(token);
        token = strtok(NULL, ", ");
        printers = atoi(token);
        token = strtok(NULL, ", ");
        scanners = atoi(token);
        token = strtok(NULL, ", ");
        modems = atoi(token);
        token = strtok(NULL, ", ");
        cds = atoi(token);

        // Assign values to newProc structure
        newProc.arrivalTime = arrivalTime;
        newProc.priority = priority;
        newProc.processorTime = processorTime;
        newProc.memory = memory;
        newProc.printers = printers;
        newProc.scanners = scanners;
        newProc.modems = modems;
        newProc.cds = cds;
        newProc.c_pid = -1; // Initialize child process ID to -1
        push(newProc, temp); // Push proc to temp queue
    }
}

// Moves the items out of temp queue to appropriate priority queue if arrival time has been reached
void checkArrival(int timeNow) {
    if (temp->next == NULL){
        return; // Return if temp queue is empty
    }
    printf("Arrival Time: %d\n\n", temp->next->process.arrivalTime);
    while(temp->next != NULL && temp->next->process.arrivalTime <= timeNow){
        proc_t arrivedProc = pop(temp); // Remove element from temp queue
        push(arrivedProc, queues[arrivedProc.priority]); // Add to appropriate priority queue
    }
}

// Finds free memory and returns the location of memory if none are available
int findFreeMemory(int amountNeeded) {
    int startPos = 0;
    int currPos = 0;
    bool found = false;

    while(found == false && startPos < MEMORY) {
        while(startPos < MEMORY && avail_mem[startPos] != 0) {
            startPos++;
        }

        for(currPos = startPos; currPos - startPos + 1 <= amountNeeded; currPos++) {
            if (currPos >= MEMORY) {
                break;
            }
            if(avail_mem[currPos] == 1) {
                startPos = currPos + 1;
                break;
            }
            if(currPos - startPos + 1 == amountNeeded) {
                found = true;
                break;
            }
        }
    }

    if (found == true){
        for (currPos = startPos; currPos - startPos + 1 <= amountNeeded; currPos++){
            avail_mem[currPos] = 1; // Mark memory as allocated
        }
        return startPos; // Return start position of allocated memory
    } else {
        return -1; // Return -1 if memory is not available
    }
}

// Clears memory that was being used
void clearMemory(int startPos, int amount) {
    for (int i = startPos; i < startPos + amount; i++){
        avail_mem[i] = 0; // Mark memory as deallocated
    }
}

// Executes priority processes
void runPriority() {
    while(queues[0]->next != NULL) {
        proc_t currProc = pop(queues[0]); // Pop the first process from highest priority queue
        int memoryStart = findFreeMemory(currProc.memory); // Find free memory for the process
        int status;
        pid_t c_pid, pid;
        c_pid = fork(); // Fork a child process
        
        // Child process
        if (c_pid == 0) {
            printf("Executing ./process:\n");	
            execlp("./process", "./process", NULL); // Execute process binary
            perror("execvp failed\n");
        } 
        // Parent process
        else if(c_pid > 0) {
            sleep(currProc.processorTime); // Sleep for the processor time of the process
            kill(c_pid, SIGINT); // Terminate child process
            if((pid = wait(&status)) < 0) {
                perror("wait");
                _exit(1);
            }
        }
        // Updates the current time
        currTime = currProc.processorTime; 
        checkArrival(currTime); // Checks for newly arrived processes

        printf("Arrival Time Finished Priority proc: %d\n", currProc.arrivalTime);
    }
}

// checks if current or above queues have items in it
bool keepRunning(int queueLevel) {
    for(int i = 0; i <= queueLevel; i++) {
        if (queues[i]->next != NULL) {
            return false; // Return false if any queue has items
        }
    }
    // returns true if all queues are empty
    return true; 
}

// Runs the processes from a specific queue level
void runQueueOne(int queueLevel) {
    int status2;
    proc_t currProc = pop(queues[queueLevel]); // Pop the first process from the specified queue
    // If the process was running before, resume it
    if (currProc.c_pid != -1) {
        kill(currProc.c_pid, SIGCONT); // Send continue signal to the process
        do {
            sleep(1);
            currProc.processorTime--; // Decrement processor time
            currTime++; // Increment current time
            checkArrival(currTime); // Check for newly arrived processes
            // Continue until processor time is exhausted or queues above are empty
        } while(currProc.processorTime > 0 && keepRunning(queueLevel));
        
        // If the processor time is not exhausted, suspend the process
        if(currProc.processorTime > 0) {
            kill(currProc.c_pid, SIGTSTP); // Send stop signal to the process
            sleep(1);
            if(queueLevel < 3) {
                push(currProc, queues[queueLevel + 1]); // Move process to next priority queue
            } else {
                push(currProc, queues[3]);
            }
        } 
        // If the processor time is exhausted, terminate the process
        else {
            kill(currProc.c_pid, SIGINT); // Send interrupt signal to the process
            sleep(1);
        }
    } 
   
    else {         
        int status3;
        pid_t c_pid, pid;
        c_pid = fork(); // Forks a child process
        
        // Represents the child process
        if (c_pid == 0) { 
            printf("Executing ./process:\n");	
            execlp("./process", "./process", NULL); // Execute process binary
            perror("execvp failed\n");   
        } 
        // Parent process
        else if(c_pid > 0) {
            currProc.c_pid = c_pid; // Update child process ID
            currProc.pid = pid; // Update process ID
            do {
                sleep(1);
                currProc.processorTime--; // Decrement processor time
                currTime++; // Increment current time
                checkArrival(currTime); // Check for newly arrived processes
                // Continue until processor time is exhausted or queues above are empty
            } while(currProc.processorTime > 0 && keepRunning(queueLevel));

            // If processor time is not exhausted, suspend the process
            if(currProc.processorTime > 0) {
                kill(currProc.c_pid, SIGTSTP); // Send stop signal to the process
                sleep(1);
                if(queueLevel < 3) {
                    push(currProc, queues[queueLevel + 1]); // Move process to next priority queue
                } else {
                    push(currProc, queues[3]);
                }
            } 
            // If processor time is exhausted, terminate the process
            else {
                kill(currProc.c_pid, SIGINT); // Send interrupt signal to the process
            }
        }
    }
}

