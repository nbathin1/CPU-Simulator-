//
//  main.c
//  Pipeline
//
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cpu.h"

int binary_flag;

void run_cpu_fun(){

    CPU *cpu = CPU_init();
    CPU_run(cpu);
    CPU_stop(cpu);
}

int main(int argc, const char * argv[]) {
    if (argc<=1) {
        fprintf(stderr, "Error : missing required args\n");
        return -1;
    }
    char* filename = (char*)argv[1];
    char line[100];
    int num_lines = 0;
    FILE *fp1 = fopen(filename, "r");
    for(int i=0;i<MEMORY_SIZE;i++){
        strcpy(memory[i],"\0");
    }
    if (fp1 == NULL) {
        printf("Error: could not open file.\n");
        return 1;
    }

    while (fgets(line, sizeof(line), fp1) != NULL && num_lines < MEMORY_SIZE) {
        char *token = strtok(line, " \n");
        while (token != NULL) {
            strcpy(memory[num_lines], token);
            token = strtok(NULL, " \n");
            num_lines++;
        }
    }
fclose(fp1);

    run_cpu_fun();    
    return 0;
}
