#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "dyn_array.h"
#include "process_scheduling.h"


// private function
void virtual_cpu(ProcessControlBlock_t *process_control_block) 
{
    // decrement the burst time of the pcb
    --process_control_block->remaining_burst_time;
}
// function to compare the burst times of 2 different processes
int compare_burst_times(const void*x, const void*y){
    // if process1 burst time is > process2 burst time
    if (((ProcessControlBlock_t*)x)->remaining_burst_time > ((ProcessControlBlock_t*)y)->remaining_burst_time){
        return -1; 
    // else if process1 burst time is < process2 burst time
    } else if (((ProcessControlBlock_t*)x)->remaining_burst_time < ((ProcessControlBlock_t*)y)->remaining_burst_time){
        return 1; 
    }
    return 0; // else if process1 burst time = process2 burst time
}

/*
===============================================================================================================
Definition from txt bkK: the process that request the CPU first is allocated the cpu first. 
===============================================================================================================
            -- When a process enters into the the ready queue, its PCB is linked onto the tail of the queue, 
            -- When the vCPU is free, it is allocated to the process at the head of the queue 
            -- the running process is then removed from the queue                      
*/

bool first_come_first_serve(dyn_array_t *ready_queue, ScheduleResult_t *result) 
{
    // You need to remove this when you begin implementation.
    // check for validity of params 
    if ( !ready_queue || !result ) return false; 

    // initialize vars 
    uint32_t size = dyn_array_size(ready_queue); // calculate the amount of PCB in the ready queue 
    uint32_t waiting_time = 0; // waiting time on ready queue 
    uint32_t turnaround_time = 0; // completion time of Process Control Blocks 
    // remember to calculate total time to run all Proc. Cont. Blocks 
    uint32_t curr_process_waiting = 0; // curr process waiting time 
    uint32_t running_time = 0; // amount of time to run 

    // create an empty pcb 
    ProcessControlBlock_t pcb_buffer; 

    // iterate through each PCB in ready Queue 
    for ( uint32_t count = 0; count < size; count++ ){
        dyn_array_extract_back(ready_queue, (void*)&pcb_buffer); 

        // update waiting time for total time CPU been busy 
        curr_process_waiting = running_time - pcb_buffer.arrival; 
        waiting_time += curr_process_waiting;

        // update vars from burst time,
        // calculate how long the turnaround time is based on the arrival time and total running time 
        running_time += pcb_buffer.remaining_burst_time; 
        turnaround_time += running_time; 
        turnaround_time -= pcb_buffer.arrival; 
        waiting_time -= pcb_buffer.arrival; 

        // decrement burst time 
        while ( pcb_buffer.remaining_burst_time != 0 ){
            virtual_cpu((void*)&pcb_buffer); 
        }
        
    }

    // calculate values 
    result->average_waiting_time = (float)waiting_time / size; 
    result->average_turnaround_time = (float)turnaround_time/ size; 
    result->total_run_time = running_time; 
    return true;
}
bool shortest_job_first(dyn_array_t *ready_queue, ScheduleResult_t *result) 
{
    if (ready_queue == NULL|| result == NULL){ // param check 
        return false; 
    }
    // init variables
    result->average_waiting_time = 0;
    result->average_turnaround_time = 0;
    result->total_run_time = 0;
    uint32_t size = dyn_array_size(ready_queue);
    uint32_t clock = 0; // clock tick time
    uint32_t latency = 0; // total latency 
    uint32_t turnaround = 0; // turnaround time 
    bool extracted = false; // check if pcb is correctly extracted
    int sum = 0; // arrival time summation 

    ProcessControlBlock_t*pcb = malloc(sizeof(ProcessControlBlock_t)); // malloc process control blocks
    pcb = (ProcessControlBlock_t*)(dyn_array_export(ready_queue));
    dyn_array_t*da = dyn_array_create(0, size, NULL); // init dynamic array

    for (uint32_t i = 0; i < size; i++){ 
        for (uint32_t j = 0; j < size; j++){
            if (pcb[j].arrival <= clock){ // if the process has arrived
                if (pcb[j].started == false){ // if the process hasn't been scheduled
                    extracted = dyn_array_push_back(da, &pcb[j]); // add the process to the back of the queue
                    if (extracted == false){ // if process push failed
                        return false;
                    }
                    pcb[j].started = true; // change scheduled status of process to true  
                    sum = sum + pcb[j].arrival; // add the arrival time to the arrival sum
                }
            }
        }
        if (dyn_array_size(da) != 0){ // if the queue is not empty
            dyn_array_sort(da, compare_burst_times); // sort the array based on burst times
            while (dyn_array_size(da) > 0){ // loop through queue while its not empty
                ProcessControlBlock_t temp_pcb; // init temp pcb var
                extracted = dyn_array_extract_back(da, &temp_pcb); // extract last process in queue
                if (extracted == false){ // if extraction failed
                    return false;
                }
                temp_pcb.started = true; // change status
                latency = latency + clock; // update latency time
                while (temp_pcb.remaining_burst_time > 0){ // while scheduling algo is not done
                    virtual_cpu(&temp_pcb); // add process to virtual cpu
                    clock++; // increment clock ticks
                }
                temp_pcb.started = false; // update status
                turnaround = turnaround + clock; // update turnaround time
            }
        }
    }
    // update results 
    result->average_turnaround_time = (float)(turnaround - sum) / size; 
    result->average_waiting_time = (float)(latency - sum) / size; 
    result->total_run_time = clock; 
    return true;
}
bool round_robin(dyn_array_t *ready_queue, ScheduleResult_t *result, size_t quantum)
{
    // check for validity of params
    if (!ready_queue || !result || quantum == 0)
        return false;

    // initialize result fields
    result->total_run_time = 0;
    result->average_waiting_time = 0;
    result->average_turnaround_time = 0;

    // initialize variables
    bool extracted = false;                     
    uint32_t size = dyn_array_size(ready_queue);  // total number of processes

    // update each PCB's arrival situation:
    // store the original arrival time in priority and update arrival to the current runtime (0 at start)
    for (uint32_t i = 0; i < size; i++) {
        ProcessControlBlock_t *temp_pcb = (ProcessControlBlock_t*)dyn_array_at(ready_queue, i);
        // store original arrival time as priority
        temp_pcb->priority = temp_pcb->arrival; 
        // update the arrival time to the current total runtime ("date the arrival")
        temp_pcb->arrival = result->total_run_time;
    }

    // counter to ensure a process runs for at most quantum time
    uint32_t counter = quantum;

    // process the ready queue until all PCBs have been executed
    while (dyn_array_size(ready_queue) > 0) {
        ProcessControlBlock_t pcb;

        // extract the next process from the ready queue
        extracted = dyn_array_extract_back(ready_queue, &pcb);
        if (!extracted)
            return false;

        // if the process hasn't arrived yet, simulate idle time until it does
        if (pcb.arrival > result->total_run_time) {
            result->total_run_time = pcb.arrival;
        }

        // mark the process as started and run it on the virtual CPU until completion or quantum expires
        pcb.started = true;
        while (pcb.remaining_burst_time != 0 && counter != 0) {
            virtual_cpu(&pcb);
            result->total_run_time++;              // update total runtime for each tick of execution
            counter--;                             // decrement quantum counter
            // accumulate waiting time from processes still in the ready queue
            result->average_waiting_time += dyn_array_size(ready_queue);
        }

        // if the process has finished executing, update its turnaround time;
        // otherwise, reinsert it at the front of the ready queue for further processing
        if (pcb.remaining_burst_time == 0) {
            pcb.started = false;
            result->average_turnaround_time += result->total_run_time;
        } else {
            dyn_array_push_front(ready_queue, &pcb);
        }

        // reset the quantum counter for the next process execution
        counter = quantum;
    }

    // compute average waiting and turnaround times
    result->average_waiting_time = (float)result->average_waiting_time / size;
    result->average_turnaround_time = (float)result->average_turnaround_time / size;

    return true;
}

dyn_array_t *load_process_control_blocks(const char *input_file) 
{
    // error check params and input file
    if(!input_file || !strcmp(input_file, "") || !strcmp(input_file, "\n")) return NULL;

    // error check for .bin extention being passed // return null if false 
    char *temp; 
    temp = strrchr(input_file, '.');
    if( temp == NULL || strcmp(temp, ".bin")) return NULL;

    // open the file and handel error
    // O_RDONLY opens file in read only, binary mode 
    // returns null if file is empty or opening returned -1 (error opening)
    int fp = open(input_file, O_RDONLY); 
    if(fp <= 0) return NULL; 

    // the first byte stores the amount of PCBs in file 
    // initialize buffer (buff) for start of file 
    // store read in signed size_t for error handeling 
    uint32_t buff; /* buffer for handeling size of burst */
    ssize_t buffer_read = read(fp, &buff, sizeof(uint32_t));
    if(buffer_read < 1) return NULL; // error check 

    // malloc a buffer array of pcbs
    // iterate through the file storing PCB data into the array of buffer pcbs
    ProcessControlBlock_t *pcb_buffer = malloc(sizeof(ProcessControlBlock_t) * buff);
    for(uint32_t i = 0; i < buff; i++) {
        buffer_read = read(fp, &pcb_buffer[i].remaining_burst_time, sizeof(uint32_t));
        buffer_read = read(fp, &pcb_buffer[i].priority, sizeof(uint32_t));
        buffer_read = read(fp, &pcb_buffer[i].arrival, sizeof(uint32_t));
    }
    // create a dynamic array of pcb from the array of buffer pcbs 
    // import pcb_buffers to create dyn_array_t buffer 
    dyn_array_t *dyn_pcb_arr= dyn_array_import((void *)pcb_buffer, sizeof(buff), sizeof(ProcessControlBlock_t), NULL);

    // close file 
    // free process control buffer 
    // return created dyn_pcb_arr
    close(fp);
    free(pcb_buffer);
    return dyn_pcb_arr;
}
bool shortest_remaining_time_first(dyn_array_t *ready_queue, ScheduleResult_t *result) 

 {
    // check for validity of parameters
    if (!ready_queue || !result)
        return false;

    // sort the ready queue by remaining burst time (SJF scheduling)
    dyn_array_sort(ready_queue, compare_burst_times);

    // initialize result fields
    result->total_run_time = 0;
    result->average_turnaround_time = 0;
    result->average_waiting_time = 0;

    // initialize local variables
    bool extracted = false;                         // flag to indicate successful PCB extraction
    uint32_t size = dyn_array_size(ready_queue);      // total number of processes
    uint32_t clock = 0;                             // current time (wall clock)
    uint32_t total_wait_time = 0;                     // cumulative waiting time (latency)

    // loop through each process in the ready queue
    for (uint32_t i = 0; i < size; i++) {
        ProcessControlBlock_t pcb;

        // extract the next PCB from the ready queue
        extracted = dyn_array_extract_back(ready_queue, &pcb);
        if (!extracted)
            return false;

        // accumulate waiting time before running this process
        total_wait_time += result->total_run_time;

        // mark process as started and run it until completion (non-preemptive SJF)
        pcb.started = true;
        while (pcb.remaining_burst_time != 0) {
            virtual_cpu(&pcb);
            clock++;                    // increment local clock
            result->total_run_time++;   // update total run time
        }
        pcb.started = false;            // mark process as completed

        // update cumulative turnaround time (time from start until process completion)
        result->average_turnaround_time += clock;
    }

    // calculate average waiting and turnaround times
    result->average_waiting_time = (float)total_wait_time / size;
    result->average_turnaround_time = (float)result->average_turnaround_time / size;

    return true;
}