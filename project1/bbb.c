#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "cpu.h"

#define REG_COUNT 128
#define MEMORY_SIZE 1024 

#define add 0
#define sub 1
#define mul 2
#define div 3
#define set 4
#define ld 5
#define ret 6


Register* create_registers(int size);

int stall_flag=0;
int stalled_cycles = 0;
int pc = 0;
int instructions_simulated = 0;

void fetch_instruction(CPU* cpu);
void decode_instruction(CPU* cpu);
void issue_instruction(CPU* cpu);
void read_registers(CPU* cpu);
void execute_add(CPU* cpu);
void execute_mul(CPU* cpu);
void execute_div(CPU* cpu);
void branch(CPU* cpu);
void mem1(CPU* cpu);
void mem2(CPU* cpu);
void writeback(CPU* cpu);

Instruction fetch_instr=(Instruction){-1,-1,-1,-1,0,0};
Instruction decode_instr=(Instruction){-1,-1,-1,-1,0,0};
Instruction ia_instr=(Instruction){-1,-1,-1,-1,0,0};
Instruction rr_instr=(Instruction){-1,-1,-1,-1,0,0};
Instruction add_instr=(Instruction){-1,-1,-1,-1,0,0};
Instruction mul_instr=(Instruction){-1,-1,-1,-1,0,0};
Instruction div_instr=(Instruction){-1,-1,-1,-1,0,0};
Instruction br_instr=(Instruction){-1,-1,-1,-1,0,0};
Instruction mem1_instr=(Instruction){-1,-1,-1,-1,0,0};
Instruction mem2_instr=(Instruction){-1,-1,-1,-1,0,0};
Instruction wb_instr=(Instruction){-1,-1,-1,-1,0,0};

int memory_map(int n){
    FILE *fp;
    char word[10];
    int word_count = 0;
    int w;
    fp = fopen("memory_map.txt", "r");
    if (fp == NULL) {
        printf("Error: unable to open file \n");
        return 1;
    }
    while (fscanf(fp, "%s", word) == 1) {
        word_count++;
        if (word_count == n+1) {
            w = atoi(word);
            break;
        }
    }
    fclose(fp);
    return w;
}



int remove_(char a[10]) {
    int num = 0;
    if (a[0] != '\0') {
        num = atoi(a+1);
    }
    return num;
}


CPU* CPU_init() {
CPU* cpu = malloc(sizeof(*cpu));
if (!cpu) {
return NULL;
}
return cpu;
}

void CPU_stop(CPU* cpu) {
free(cpu->regs);
free(cpu);
}

void print_registers(CPU* cpu) {
printf("================================\n");
printf("--------------------------------\n");
for (int reg=0; reg<REG_COUNT; reg++) {
printf("REG[%2d] | Value=%d \n", reg, cpu->regs[reg].value);
printf("--------------------------------\n");
}
printf("================================\n\n");
}

int CPU_run(CPU* cpu) {

cpu->regs = create_registers(REG_COUNT);
int total_cycles = 1;
float ipc = 0.0;
int k=0;

while (k==0) {

    printf("================================\n");
    printf("Clock cycle #: %d\n",total_cycles);
    printf("--------------------------------\n");

    writeback(cpu);
    mem2(cpu);
    mem1(cpu);
    branch(cpu);
    execute_div(cpu);
    execute_mul(cpu);
    execute_add(cpu);
    read_registers(cpu);
    issue_instruction(cpu);
    decode_instruction(cpu);
    fetch_instruction(cpu);
    total_cycles += 1;
    if(wb_instr.opcode==ret){
        k=1;
    }
    printf("=============== STATE OF ARCHITECTURAL REGISTER FILE ==========\n");
    for(int i=0;i<REG_COUNT;i++){
        printf("|   REG[ %d]   |   Value=%d     |   Status=valid   |\n",i,cpu->regs[i].value);
    }
}

printf("Stalled cycles due to structural hazard: %d\n", stalled_cycles);
printf("Total execution cycles: %d\n", total_cycles-1);
printf("Total instructions simulated: %d\n", instructions_simulated);
ipc = (float)instructions_simulated / total_cycles;
printf("IPC: %.2f\n", ipc);
printf("================================\n");
print_registers(cpu);
return 0;
}

Register* create_registers(int size) {
Register* regs = malloc(sizeof(*regs) * size);
if (!regs) {
return NULL;
}
for (int i=0; i<size; i++) {
regs[i].value = 0;
regs[i].is_writing = false;
}
return regs;
}

// Fetches the next instruction from memory and returns it
void fetch_instruction(CPU* cpu) {
    if(stall_flag==1){ 
    fetch_instr=(Instruction){-1,-1,-1,-1,0,0};
    strcpy(fetch_p[0],"\0");
    stalled_cycles++;
    return;
    }
    strcpy(fetch_p[0],memory[pc]);
    pc=pc+1;
    if(strcmp(memory[pc],"\0")==0){
        fetch_instr=(Instruction){-1,-1,-1,-1,0,0};
    strcpy(fetch_p[0],"\0");
        return;
    }
    instructions_simulated++;
    if(strcmp(memory[pc],"add")==0){fetch_instr.opcode=0;}
    if(strcmp(memory[pc],"sub")==0){fetch_instr.opcode=1;}
    if(strcmp(memory[pc],"mul")==0){fetch_instr.opcode=2;}
    if(strcmp(memory[pc],"div")==0){fetch_instr.opcode=3;}
    
    if(strcmp(memory[pc],"set")==0){
    fetch_instr.opcode=4;
    fetch_instr.dest=remove_(memory[pc+1]);
    if (strncmp(memory[pc+2], "R", 1) == 0){fetch_instr.reg1=remove_(memory[pc+2]); fetch_instr.op1=-1; }
    if (strncmp(memory[pc+2], "#", 1) == 0){fetch_instr.op1=remove_(memory[pc+2]); fetch_instr.reg1=-1; }
    fetch_instr.op2=0; fetch_instr.reg2=-1;
    printf("IF      : %s %s %s %s \n",fetch_p[0],memory[pc],memory[pc+1],memory[pc+2]);
    strcpy(fetch_p[1],memory[pc]);
    strcpy(fetch_p[2],memory[pc+1]);
    strcpy(fetch_p[3],memory[pc+2]);
    strcpy(fetch_p[4]," ");
    pc=pc+3;
    return;
    }

    if(strcmp(memory[pc],"ld")==0){
    fetch_instr.opcode=5;
    fetch_instr.dest=remove_(memory[pc+1]);
    if (strncmp(memory[pc+2], "R", 1) == 0){fetch_instr.reg1=remove_(memory[pc+2]); fetch_instr.op1=-1; }
    if (strncmp(memory[pc+2], "#", 1) == 0){fetch_instr.op1=remove_(memory[pc+2]); fetch_instr.reg1=-1; }
    fetch_instr.op2=0; fetch_instr.reg2=-1;
    printf("IF      :%s %s %s %s\n",fetch_p[0],memory[pc],memory[pc+1],memory[pc+2]);
    strcpy(fetch_p[1],memory[pc]);
    strcpy(fetch_p[2],memory[pc+1]);
    strcpy(fetch_p[3],memory[pc+2]);
    strcpy(fetch_p[4]," ");
    pc=pc+3;
    return;
    }

    if(strcmp(memory[pc],"ret")==0){fetch_instr.opcode=6;
    fetch_instr.dest=-1;
    fetch_instr.reg1=-1;
    fetch_instr.reg2=-1;
    fetch_instr.op1=0;
    fetch_instr.op2=0;
    printf("IF : %s %s\n",fetch_p[0],memory[pc]);
    strcpy(fetch_p[1],memory[pc]);
    strcpy(fetch_p[2],memory[pc+1]);
    strcpy(fetch_p[3],memory[pc+2]);
    strcpy(fetch_p[4],memory[pc+3]);
    
    pc=pc+1;
    return;
    }

    fetch_instr.dest=remove_(memory[pc+1]);
    if (strncmp(memory[pc+2], "R", 1) == 0){fetch_instr.reg1=remove_(memory[pc+2]); fetch_instr.op1=-1; }
    if (strncmp(memory[pc+2], "#", 1) == 0){fetch_instr.op1=remove_(memory[pc+2]); fetch_instr.reg1=-1; }
    if (strncmp(memory[pc+3], "R", 1) == 0){fetch_instr.reg2=remove_(memory[pc+3]);fetch_instr.op2=-1;  }
    if (strncmp(memory[pc+3], "#", 1) == 0){fetch_instr.op2=remove_(memory[pc+3]); fetch_instr.reg2=-1; }
    
if(strlen(memory[pc]) != 0){
    strcpy(fetch_p[1],memory[pc]);
    strcpy(fetch_p[2],memory[pc+1]);
    strcpy(fetch_p[3],memory[pc+2]);
    strcpy(fetch_p[4],memory[pc+3]);
    printf("IF      : %s %s %s %s %s\n",fetch_p[0],fetch_p[1],fetch_p[2],fetch_p[3],fetch_p[4]);
}
else{
    strcpy(fetch_p[0],"\0");
}

    pc=pc+4;
}


// Decodes an instruction and returns the decoded Instruction struct
void decode_instruction(CPU* cpu) {
    decode_instr=fetch_instr;
    if(strlen(fetch_p[0]) != 0){
    strcpy(decode_p[0],fetch_p[0]);strcpy(decode_p[1],fetch_p[1]);strcpy(decode_p[2],fetch_p[2]);strcpy(decode_p[3],fetch_p[3]);strcpy(decode_p[4],fetch_p[4]);
    printf("ID      : %s %s %s %s %s\n",decode_p[0],decode_p[1],decode_p[2],decode_p[3],decode_p[4]);
    }
    else{
    strcpy(decode_p[0],"\0");
    strcpy(decode_p[1],"\0");
    strcpy(decode_p[2],"\0");
    strcpy(decode_p[3],"\0");
}

}

// Issues an instruction by setting the destination register to the CPU's register file
void issue_instruction(CPU* cpu) {
    ia_instr=decode_instr;
    cpu->regs[decode_instr.dest].is_writing = 0;
     if(strlen(decode_p[0]) != 0){
    strcpy(ia_p[0],decode_p[0]);strcpy(ia_p[1],decode_p[1]);strcpy(ia_p[2],decode_p[2]);strcpy(ia_p[3],decode_p[3]);strcpy(ia_p[4],decode_p[4]);
    printf("IA      : %s %s %s %s %s\n",ia_p[0],ia_p[1],ia_p[2],ia_p[3],ia_p[4]);
    }
    else{
    strcpy(ia_p[0],"\0");
    strcpy(ia_p[1],"\0");
    strcpy(ia_p[2],"\0");
    strcpy(ia_p[3],"\0");
}
}

// Reads the source registers for an instruction from the CPU's register file
void read_registers(CPU* cpu) {
    rr_instr=ia_instr;
    cpu->regs[rr_instr.dest].is_writing=1;
    if(strlen(ia_p[0]) != 0){
    strcpy(rr_p[0],ia_p[0]);strcpy(rr_p[1],ia_p[1]);strcpy(rr_p[2],ia_p[2]);strcpy(rr_p[3],ia_p[3]);strcpy(rr_p[4],ia_p[4]);
    printf("RR      : %s %s %s %s %s\n",rr_p[0],rr_p[1],rr_p[2],rr_p[3],rr_p[4]);
    }
    else{
    strcpy(rr_p[0],"\0");
    strcpy(rr_p[1],"\0");
    strcpy(rr_p[2],"\0");
    strcpy(rr_p[3],"\0");
}
    if(rr_instr.reg1 != -1){
        rr_instr.op1 = cpu->regs[rr_instr.reg1].value;        
    }
    if(rr_instr.reg2 != -1){
        rr_instr.op2 = cpu->regs[rr_instr.reg2].value;
    }
}


void execute_add(CPU* cpu) {
   add_instr=rr_instr;

    if(strlen(rr_p[0]) != 0){
    strcpy(add_p[0],rr_p[0]);strcpy(add_p[1],rr_p[1]);strcpy(add_p[2],rr_p[2]);strcpy(add_p[3],rr_p[3]);strcpy(add_p[4],rr_p[4]);
    printf("ADD    : %s %s %s %s %s\n",add_p[0],add_p[1],add_p[2],add_p[3],add_p[3]);
    }
    else{
    strcpy(add_p[0],"\0");
    strcpy(add_p[1],"\0");
    strcpy(add_p[2],"\0");
    strcpy(add_p[3],"\0");
}

    if(add_instr.opcode==add){
        add_instr.op1 = add_instr.op1 + add_instr.op2;
    }
    if(add_instr.opcode==sub){
        add_instr.op1 = add_instr.op1 - add_instr.op2;
    }
}


void execute_mul(CPU* cpu) {
    mul_instr = add_instr;
    if(strlen(add_p[0]) != 0){
    strcpy(mul_p[0],add_p[0]);strcpy(mul_p[1],add_p[1]);strcpy(mul_p[2],add_p[2]);strcpy(mul_p[3],add_p[3]);strcpy(mul_p[4],add_p[4]);
    printf("MUL    : %s %s %s %s %s\n",mul_p[0],mul_p[1],mul_p[2],mul_p[3],mul_p[4]);
    }
    else{
    strcpy(mul_p[0],"\0");
    strcpy(mul_p[1],"\0");
    strcpy(mul_p[2],"\0");
    strcpy(mul_p[3],"\0");
}
    if(mul_instr.opcode==mul){
        mul_instr.op1 = mul_instr.op1 * mul_instr.op2;
    }
}


void execute_div(CPU* cpu) {
   div_instr=mul_instr;
        if(strlen(mul_p[0]) != 0){
    strcpy(div_p[0],mul_p[0]);strcpy(div_p[1],mul_p[1]);strcpy(div_p[2],mul_p[2]);strcpy(div_p[3],mul_p[3]);strcpy(div_p[4],mul_p[4]);
    printf("DIV    : %s %s %s %s %s\n",div_p[0],div_p[1],div_p[2],div_p[3],div_p[4]);
    }
    else{
    strcpy(div_p[0],"\0");
    strcpy(div_p[1],"\0");
    strcpy(div_p[2],"\0");
    strcpy(div_p[3],"\0");
}

    if(div_instr.opcode==div){
        div_instr.op1 = div_instr.op1 / div_instr.op2;
    }
}


void branch(CPU *cpu){
        if(strlen(div_p[0]) != 0){
    strcpy(br_p[0],div_p[0]);strcpy(br_p[1],div_p[1]);strcpy(br_p[2],div_p[2]);strcpy(br_p[3],div_p[3]);strcpy(br_p[4],div_p[4]);
    printf("BR      : %s %s %s %s %s\n",br_p[0],br_p[1],br_p[2],br_p[3],br_p[4]);
    }
    else{
    strcpy(br_p[0],"\0");
    strcpy(br_p[1],"\0");
    strcpy(br_p[2],"\0");
    strcpy(br_p[3],"\0");
}

    br_instr=div_instr;

}

void mem1(CPU *cpu){
        if(strlen(br_p[0]) != 0){
    strcpy(mem1_p[0],br_p[0]);strcpy(mem1_p[1],br_p[1]);strcpy(mem1_p[2],br_p[2]);strcpy(mem1_p[3],br_p[3]);strcpy(mem1_p[4],br_p[4]);
    printf("Mem1     : %s %s %s %s %s\n",mem1_p[0],mem1_p[1],mem1_p[2],mem1_p[3],mem1_p[4]);
    }
    else{
    strcpy(mem1_p[0],"\0");
    strcpy(mem1_p[1],"\0");
    strcpy(mem1_p[2],"\0");
    strcpy(mem1_p[3],"\0");
}

    mem1_instr=br_instr;
    if(mem1_instr.opcode==ld){
        if(mem1_instr.reg1==-1){
            mem1_instr.op1=mem1_instr.op1/4;
        }
    }
}

void mem2(CPU *cpu){
    if(strlen(mem1_p[0]) != 0){

    strcpy(mem2_p[0],mem1_p[0]);strcpy(mem2_p[1],mem1_p[1]);strcpy(mem2_p[2],mem1_p[2]);strcpy(mem2_p[3],mem1_p[3]);strcpy(mem2_p[4],mem1_p[4]);
    printf("Mem2     : %s %s %s %s %s\n",mem2_p[0],mem2_p[1],mem2_p[2],mem2_p[3],mem2_p[4]);
    }
    else{
    strcpy(mem2_p[0],"\0");
    strcpy(mem2_p[1],"\0");
    strcpy(mem2_p[2],"\0");
    strcpy(mem2_p[3],"\0");
}

    mem2_instr=mem1_instr;
    stall_flag=0;
    if(mem2_instr.opcode==ld){
        stall_flag=1;
        if(mem2_instr.reg1==-1){
            mem2_instr.op1=memory_map(mem2_instr.op1);
        }
    }
}

void writeback(CPU* cpu) {
    wb_instr=mem2_instr;
   if(strlen(mem2_p[0]) != 0){
    strcpy(wb_p[0],mem2_p[0]);strcpy(wb_p[1],mem2_p[1]);strcpy(wb_p[2],mem2_p[2]);strcpy(wb_p[3],mem2_p[3]);strcpy(wb_p[4],mem2_p[4]);
    printf("WB      : %s %s %s %s %s\n",wb_p[0],wb_p[1],wb_p[2],wb_p[3],wb_p[4]);
    }
    else{
    strcpy(wb_p[0],"\0");
    strcpy(wb_p[1],"\0");
    strcpy(wb_p[2],"\0");
    strcpy(wb_p[3],"\0");
}
    if(wb_instr.dest==-1){
        return;
    }
    cpu->regs[wb_instr.dest].value=wb_instr.op1;
    cpu->regs[wb_instr.dest].is_writing=0;
    
}