/*
 * student.c
 * Multithreaded OS Simulation for CS 2200, Project 6
 * Summer 2015
 *
 * This file contains the CPU scheduler for the simulation.
 * Name:Kaije Huang
 * GTID:903076121
 */

#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "os-sim.h"


/*
 * current[] is an array of pointers to the currently running processes.
 * There is one array element corresponding to each CPU in the simulation.
 *
 * current[] should be updated by schedule() each time a process is scheduled
 * on a CPU.  Since the current[] array is accessed by multiple threads, you
 * will need to use a mutex to protect it.  current_mutex has been provided
 * for your use.
 */
static pcb_t **current;
static pthread_mutex_t current_mutex;

// head of the readyQueue
static pcb_t *head;
//mutex for the readyQueue
static pthread_mutex_t ready_mutex;

//idle condition
// static int in_idle;
static pthread_cond_t not_empty;

//for different scheduler type
static int scheduler_type;
static int time_slice;
static int cpu_count;

//get the size of the readyQueue && traverse through the ready queue and print the nodes
// int size() {
//     int i = 0;
//     pcb_t *temp;
//     if(head == NULL) return 0;
//     for(temp = head; temp->next != NULL; temp = temp->next) {
//         //print the node
//         printf("%s / ", temp->name);
//         i++;
//     }

//     printf("\n");
    
//     return i;
// }

//enqueue
void enqueue(pcb_t* newPcb) {
    pcb_t* curr;
    newPcb->next = NULL;

    if(head == NULL) {
        head = newPcb;
    } else {
        //loop through the queue to get the tail
        for(curr = head; curr->next != NULL; curr = curr->next);
        curr->next = newPcb;
    }
    pthread_cond_signal(&not_empty);
}

//dequeue
pcb_t* dequeue() {
    pcb_t* curr;
    
    if(head == NULL) {
        return NULL;
    } else {
        curr = head;
        head = head->next;
    }
    return curr;
}

//dequeue from tail
void enqueueFromHead(pcb_t* newPcb) {
    if(head == NULL) {
        head = newPcb;
    } else{
        newPcb->next = head;
        head = newPcb;  
    } 
    pthread_cond_signal(&not_empty);

}




/*
 * schedule() is your CPU scheduler.  It should perform the following tasks:
 *
 *   1. Select and remove a runnable process from your ready queue which 
 *	you will have to implement with a linked list or something of the sort.
 *
 *   2. Set the process state to RUNNING
 *
 *   3. Call context_switch(), to tell the simulator which process to execute
 *      next on the CPU.  If no process is runnable, call context_switch()
 *      with a pointer to NULL to select the idle process.
 *	The current array (see above) is how you access the currently running
 *	process indexed by the cpu id. See above for full description.
 *	context_switch() is prototyped in os-sim.h. Look there for more information 
 *	about it and its parameters.
 */
static void schedule(unsigned int cpu_id)
{
    /* FIX ME */

    pthread_mutex_lock(&ready_mutex);
    pcb_t* newPcb = dequeue();
    pthread_mutex_unlock(&ready_mutex);




    if(newPcb== NULL) {
        context_switch(cpu_id, NULL, time_slice); 
    } else {
        //Not sure if it is a need to lock the ready queue here
        pthread_mutex_lock(&ready_mutex);
        newPcb->state = PROCESS_RUNNING;
        pthread_mutex_unlock(&ready_mutex);


        pthread_mutex_lock(&current_mutex);
        //assign the new process to a cpu
        current[cpu_id] = newPcb;
        pthread_mutex_unlock(&current_mutex);

        context_switch(cpu_id, newPcb, time_slice); 
    } 
}


/*
 * idle() is your idle process.  It is called by the simulator when the idle
 * process is scheduled.
 *
 * This function should block until a process is added to your ready queue.
 * It should then call schedule() to select the process to run on the CPU.
 */
extern void idle(unsigned int cpu_id)
{
    /* FIX ME */
    pthread_mutex_lock(&ready_mutex);
    // in_idle++;

    while(head == NULL){
        pthread_cond_wait(&not_empty, &ready_mutex);
    }

    // in_idle--;
    pthread_mutex_unlock(&ready_mutex);
    schedule(cpu_id);

    /*
     * REMOVE THE LINE BELOW AFTER IMPLEMENTING IDLE()
     *
     * idle() must block when the ready queue is empty, or else the CPU threads
     * will spin in a loop.  Until a ready queue is implemented, we'll put the
     * thread to sleep to keep it from consuming 100% of the CPU time.  Once
     * you implement a proper idle() function using a condition variable,
     * remove the call to mt_safe_usleep() below.
     */

}


/*
 * preempt() is the handler called by the simulator when a process is
 * preempted due to its timeslice expiring.
 *
 * This function should place the currently running process back in the
 * ready queue, and call schedule() to select a new runnable process.
 */
extern void preempt(unsigned int cpu_id)
{
    /* FIX ME */
    pcb_t *running_pcb;
    running_pcb = current[cpu_id];
    enqueue(running_pcb);
    schedule(cpu_id);
}


/*
 * yield() is the handler called by the simulator when a process yields the
 * CPU to perform an I/O request.
 *
 * It should mark the process as WAITING, then call schedule() to select
 * a new process for the CPU.
 */
extern void yield(unsigned int cpu_id)
{
    /* FIX ME */
    pthread_mutex_lock(&current_mutex);
    current[cpu_id]->state = PROCESS_WAITING;
    pthread_mutex_unlock(&current_mutex);
    schedule(cpu_id);   
}


/*
 * terminate() is the handler called by the simulator when a process completes.
 * It should mark the process as terminated, then call schedule() to select
 * a new process for the CPU.
 */
extern void terminate(unsigned int cpu_id)
{
    /* FIX ME */
    pthread_mutex_lock(&current_mutex);
    current[cpu_id]->state = PROCESS_TERMINATED;
    pthread_mutex_unlock(&current_mutex);
    schedule(cpu_id);
}


/*
 * wake_up() is the handler called by the simulator when a process's I/O
 * request completes.  It should perform the following tasks:
 *
 *   1. Mark the process as READY, and insert it into the ready queue.
 *
 *   2. If the scheduling algorithm is static priority, wake_up() may need
 *      to preempt the CPU with the lowest priority process to allow it to
 *      execute the process which just woke up.  However, if any CPU is
 *      currently running idle, or all of the CPUs are running processes
 *      with a higher priority than the one which just woke up, wake_up()
 *      should not preempt any CPUs.
 *	To preempt a process, use force_preempt(). Look in os-sim.h for 
 * 	its prototype and the parameters it takes in.
 */
extern void wake_up(pcb_t *process)
{
    // /* FIX ME */    
    int i,lowestPriority,lowestId;
    int findIdle = 0;
    process->state = PROCESS_READY;


    /* loop for processes to evict if static priority mode */
    if(scheduler_type == 3) {
        pthread_mutex_lock(&current_mutex);
        lowestId = -1;

        lowestPriority = process->static_priority;
        /* loop through all CPUs */
        for(i=0; i<cpu_count; i++) {
                /* if find IDLE, stop looking */
                if(current[i] == NULL) {
                    findIdle = 1;
                    break;
                }
                /* grab the lowest priority available */
                if((current[i]->static_priority) < lowestPriority) {
                    lowestPriority = current[i]->static_priority;
                    lowestId = i;
                }
        }
        pthread_mutex_unlock(&current_mutex);
        /* evict the process if found and lower priority than our process */
        if(findIdle == 0 && lowestId != -1) {
            force_preempt(lowestId); 
            // pthread_mutex_lock(&ready_mutex);
            // enqueueFromHead(process);
            // pthread_mutex_unlock(&ready_mutex);
            // return;
        }
    }


    pthread_mutex_lock(&ready_mutex);
    enqueue(process);
    pthread_mutex_unlock(&ready_mutex);
}


/*
 * main() simply parses command line arguments, then calls start_simulator().
 * You will need to modify it to support the -r and -p command-line parameters.
 */
int main(int argc, char *argv[])
{

    cpu_count = 0;

    /* Parse command-line arguments */
    if (argc < 2 || argc > 4)
    {
        fprintf(stderr, "CS 2200 Project 4 -- Multithreaded OS Simulator\n"
            "Usage: ./os-sim <# CPUs> [ -r <time slice> | -p ]\n"
            "    Default : FIFO Scheduler\n"
            "         -r : Round-Robin Scheduler\n"
            "         -p : Static Priority Scheduler\n\n");
        return -1;
    }
    //get the number of CPU
    cpu_count = atoi(argv[1]);

    /* FIX ME - Add support for -r and -p parameters*/
    //initialize scheduler_type to 0
    if(cpu_count == 0) {
        exit(1);
    }


    scheduler_type = 0;


    if(argc == 2){
        //FIFO
        scheduler_type = 1;
        time_slice = -1;
    } else {
        if(argv[2][1] == 'r'){
            //Robin
            scheduler_type = 2;
            printf("oh %s\n", argv[3]);
            time_slice = atoi(argv[3]);//h8 * pow(10,8);

        }

        if(argv[2][1] == 'p') {
            scheduler_type = 3;
            // printf("PPPPPPP!");

        }
    }

    if(scheduler_type == 0) {
        printf("wrong input!");
        return -1;
    }

    printf("cpu : %d sc : %d\n", cpu_count, scheduler_type);
     
    /* Allocate the current[] array and its mutex */
    current = malloc(sizeof(pcb_t*) * cpu_count);
    assert(current != NULL);
    pthread_mutex_init(&current_mutex, NULL);

    //Setup read queue
    head = NULL;
    //Setup mutext for ready queue
    pthread_mutex_init(&ready_mutex, NULL);

    //Setup condition variable
    pthread_cond_init(&not_empty, NULL);

    /* Start the simulator in the library */
    start_simulator(cpu_count);

    return 0;
}


