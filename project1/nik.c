#include <stdio.h>
#include <string.h>

// Example instruction set
#define ADD 1
#define SUB 2
#define MUL 3

// Instruction format
typedef struct {
    unsigned int op ; 
    unsigned int dst ; 
    unsigned int src1 ; 
    unsigned int src2 ; 
}Instruction;

int reg[256];

unsigned char mem[65536];

void fetch(Instruction* inst, int* pc) {
    inst->op = mem[*pc];
    inst->dst = mem[*pc + 1];
    inst->src1 = mem[*pc + 2];
    inst->src2 = mem[*pc + 3];
    *pc += 4;
}


void decode(Instruction inst, int* src1_data, int* src2_data) {
    *src1_data = inst.src1;
    *src2_data = inst.src2;
}

// Execute stage
void execute(Instruction inst, int src1_data, int src2_data) {
    int result;
    switch (inst.op) {
        case ADD:
            result = src1_data + src2_data;
            break;
        case SUB:
            result = src1_data - src2_data;
            break;
        case MUL:
            result = src1_data * src2_data;
            break;
        default:
            printf("Invalid opcode %d\n", inst.op);
            return;
    }
    reg[inst.dst] = result;
}

int main() {
    // Initialize memory and registers
    memset(mem, 0, sizeof(mem));
    memset(reg, 0, sizeof(reg));

    // Example program
    mem[0] = ADD; // opcode for ADD instruction
    mem[1] = 1; // destination register
    mem[2] = 2; // source register 1
    mem[3] = 3; // source register 2

    // Initialize program counter
    int pc = 0;

    // Run pipeline
    int clockcycle =0;
    while (pc < 4) {
        Instruction inst;
        int src1_data, src2_data;
        
        fetch(&inst, &pc);
        decode(inst, &src1_data, &src2_data);
        execute(inst, src1_data, src2_data);
        clockcycle++;
    }
    printf("%d",clockcycle);
    // Print registers
    for (int i = 0; i < 256; i++) {
        printf("reg[%d] = %d\n", i, reg[i]);
    }

    return 0;
}
