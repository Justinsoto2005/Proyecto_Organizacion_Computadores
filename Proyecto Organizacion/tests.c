#include <assert.h>
#include <stdio.h>
#include "pipeline.h"

void test_add() {
    // Preparar entorno (add $t0, $t1, $t2) -> op=0, rs=9, rt=10, rd=8, funct=32
    pc = 0;
    regs[9] = 10; 
    regs[10] = 5;
    mem[0] = 0x012A4020; // add $t0, $t1, $t2

    instruction_fetch();
    instruction_decode();
    alu_execute();
    memory_access();
    write_back();

    assert(regs[8] == 15);
    printf("Test ADD pasado.\n");
}

void test_addi() {
    // Preparar entorno (addi $t0, $t1, 15) -> op=8, rs=9, rt=8, imm=15
    pc = 0;
    regs[9] = 10;
    mem[0] = 0x2128000F; 

    instruction_fetch();
    instruction_decode();
    alu_execute();
    memory_access();
    write_back();

    assert(regs[8] == 25);
    printf("Test ADDI pasado.\n");
}

void run_all_tests() {
    test_add();
    test_addi();
    // Aquí agregas los asserts de las demás instrucciones
    printf("¡Todos los unit tests pasaron!\n");
}