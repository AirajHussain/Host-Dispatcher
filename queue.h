#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

// Defines the overall process structure
typedef struct proc {
    int arrivalTime;
    int priority;
    int processorTime;
    int memory;
    int printers;
    int scanners;
    int modems;
    int cds;
    //child process ID
    pid_t c_pid; 
    //process ID
    pid_t pid;   
    int status;
} proc_t;

// Defines the queue structure
typedef struct queue {
    proc_t process;
    struct queue *next;
} queue_t;

//Represents the function to add a process to the end of the queue
void push(proc_t process, queue_t *head) {
    //finds the last node in the queue
    queue_t *curr = head;
    while (curr->next != NULL) {
        curr = curr->next; 
    }

    // Allocates the memory for the new node
    curr->next = (queue_t *)malloc(sizeof(queue_t)); 
    
    // Assigns the process to the new node
    curr->next->process = process;
    curr->next->next = NULL;
}

// Function to remove and return the first process from the queue
proc_t pop(queue_t *head) {
    queue_t *curr = head;
    
    // Checks if the queue is empty
    if (curr->next == NULL) {
        printf("Queue is empty\n");
        // Returning a default process if the queue is empty
        proc_t emptyProc = {0}; // Initialize all fields to 0
        return emptyProc;
    }

    //removes the first node from the queue
    queue_t *remove = curr->next;
    proc_t retProc = remove->process;
    curr->next = remove->next;

    //frees memory allocated for the removed node
    free(remove);

    //returns the removed process
    return retProc;
}

