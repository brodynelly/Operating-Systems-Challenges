#include <stdio.h>
#include <stdlib.h>

#include "dyn_array.h"
#include "process_scheduling.h"

#define FCFS "FCFS"
#define P "P"
#define RR "RR"
#define SJF "SJF"
// FUNCTION DEFINITIONS

// Add and comment your analysis code in this function.
// THIS IS NOT FINISHED.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Assume these types and functions are defined elsewhere:
// dyn_array_t, ScheduleResult_t, load_process_control_blocks, dyn_array_destroy,
// first_come_first_serve, round_robin, shortest_job_first

int main(int argc, char **argv)
{
    // Check number of arguments
    if (argc < 3 || argc > 4) {
        printf("Usage: %s <pcb file> <algo> [quantum]\n", argv[0]);
        return 1;
    }

    // declare vars 
    char *pcbFile = argv[1];
    char *algo = argv[2];
    size_t quantum = 0;

    // Get quantum if provided
    if (argc == 4)
        quantum = (size_t)atoi(argv[3]);

    if (argc == 4 && strcmp(algo, "RR") != 0) {
        printf("Quantum only works with RR\n");
        return 1;
    }
    if (strcmp(algo, "RR") == 0 && argc < 4) {
        printf("RR needs a quantum\n");
        return 1;
    }

    // load input file
    dyn_array_t *processes = load_process_control_blocks(pcbFile);
    if (!processes) {
        printf("Bad input file\n");
        return 1;
    }

    // allocate results 
    ScheduleResult_t *results = malloc(sizeof(ScheduleResult_t));
    if (!results) {
        printf("Memory error\n");
        dyn_array_destroy(processes);
        return 1;
    }
    // initlize result variables 
    results->average_waiting_time = 0;
    results->average_turnaround_time = 0;
    results->total_run_time = 0;

    // run correct algo 
    if (strcmp(algo, "FCFS") == 0) {
        first_come_first_serve(processes, results);
    }
    else if (strcmp(algo, "RR") == 0) {
        round_robin(processes, results, quantum);
    }
    else if (strcmp(algo, "SJF") == 0) {
        shortest_job_first(processes, results);
    }
    else {
        printf("Unknown algo! Use FCFS, RR, or SJF\n");
        free(results);
        dyn_array_destroy(processes);
        return 1;
    }

    // print functions 
    printf("\nResults for %s:\n", algo);
    printf("Average waiting time: %f\n", results->average_waiting_time);
    printf("Average turnaround time: %f\n", results->average_turnaround_time);
    printf("Total run time: %lu\n", results->total_run_time);

    // clean up code 
    free(results);
    dyn_array_destroy(processes);
    return 0;
}