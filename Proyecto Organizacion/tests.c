#include <assert.h>
#include <stdio.h>
#include "pipeline.h"

// Helpers de codificación de instrucciones MIPS
#define R_TYPE(rs, rt, rd, funct) ((uint32_t)(((rs) & 0x1F) << 21 | ((rt) & 0x1F) << 16 | \
                                   ((rd) & 0x1F) << 11 | ((funct) & 0x3F)))
#define I_TYPE(op, rs, rt, imm)   ((uint32_t)(((op) & 0x3F) << 26 | ((rs) & 0x1F) << 21 | \
                                   ((rt) & 0x1F) << 16 | ((imm) & 0xFFFF)))
#define J_TYPE(op, addr)          ((uint32_t)(((op) & 0x3F) << 26 | ((addr) & 0x3FFFFFF)))

static void reset_cpu() {
    pc = 0;
    for (int i = 0; i < 32; i++) regs[i] = 0;
    for (int i = 0; i < MEM_SIZE; i++) mem[i] = 0;
    clear_error();
}

static void run_instruction() {
    instruction_fetch();
    instruction_decode();
    alu_execute();
    memory_access();
    write_back();
}

// ======================================================================
// Unit tests por etapa (uno por módulo, como pide el diagrama del sistema)
// ======================================================================
static void test_IF() {
    reset_cpu();
    mem[0] = 0xABCD1234;
    instruction_fetch();
    assert(if_id.inst == 0xABCD1234);
    assert(if_id.pc_plus_4 == 4);
    assert(pc == 4);
    printf("Test IF pasado.\n");
}

static void test_ID() {
    reset_cpu();
    if_id.inst = R_TYPE(9, 10, 8, 0x20); // add $t0,$t1,$t2
    if_id.pc_plus_4 = 4;
    regs[9] = 3; regs[10] = 4;
    instruction_decode();
    assert(id_ex.opcode == 0);
    assert(id_ex.rs == 9 && id_ex.rt == 10 && id_ex.rd == 8);
    assert(id_ex.read_data1 == 3 && id_ex.read_data2 == 4);
    assert(id_ex.reg_write == true && id_ex.mem_read == false);
    assert(!sim_error);
    printf("Test ID pasado.\n");
}

static void test_ALU() {
    reset_cpu();
    id_ex.opcode = 0; id_ex.funct = 0x20; // add
    id_ex.read_data1 = 7; id_ex.read_data2 = 8; id_ex.rd = 5;
    id_ex.reg_write = true;
    alu_execute();
    assert(ex_mem.alu_out == 15);
    assert(ex_mem.reg_write == true);
    assert(ex_mem.dest_reg == 5);
    printf("Test ALU pasado.\n");
}

static void test_MEM() {
    reset_cpu();
    mem[10] = 777;
    ex_mem.mem_read = true; ex_mem.alu_out = 40; // 40/4 = 10
    memory_access();
    assert(mem_wb.read_data == 777);
    assert(!sim_error);
    printf("Test MEM pasado.\n");
}

static void test_WB() {
    reset_cpu();
    mem_wb.reg_write = true; mem_wb.mem_to_reg = false;
    mem_wb.alu_out = 99; mem_wb.dest_reg = 4;
    write_back();
    assert(regs[4] == 99);
    printf("Test WB pasado.\n");
}

// ======================================================================
// Una prueba por cada una de las 13 instrucciones del ISA
// ======================================================================
static void test_add() {
    reset_cpu();
    regs[9] = 10; regs[10] = 5;
    mem[0] = R_TYPE(9, 10, 8, 0x20);
    run_instruction();
    assert(regs[8] == 15 && !sim_error);
    printf("Test ADD pasado.\n");
}

static void test_sub() {
    reset_cpu();
    regs[9] = 10; regs[10] = 5;
    mem[0] = R_TYPE(9, 10, 8, 0x22);
    run_instruction();
    assert(regs[8] == 5 && !sim_error);
    printf("Test SUB pasado.\n");
}

static void test_and() {
    reset_cpu();
    regs[9] = 0xFF; regs[10] = 0x0F;
    mem[0] = R_TYPE(9, 10, 8, 0x24);
    run_instruction();
    assert(regs[8] == 0x0F && !sim_error);
    printf("Test AND pasado.\n");
}

static void test_or() {
    reset_cpu();
    regs[9] = 0xF0; regs[10] = 0x0F;
    mem[0] = R_TYPE(9, 10, 8, 0x25);
    run_instruction();
    assert(regs[8] == 0xFF && !sim_error);
    printf("Test OR pasado.\n");
}

static void test_nor() {
    reset_cpu();
    regs[9] = 0; regs[10] = 0;
    mem[0] = R_TYPE(9, 10, 8, 0x26);
    run_instruction();
    assert(regs[8] == 0xFFFFFFFF && !sim_error);
    printf("Test NOR pasado.\n");
}

static void test_xor() {
    reset_cpu();
    regs[9] = 0xFF; regs[10] = 0x0F;
    mem[0] = R_TYPE(9, 10, 8, 0x27);
    run_instruction();
    assert(regs[8] == 0xF0 && !sim_error);
    printf("Test XOR pasado.\n");
}

static void test_jr() {
    reset_cpu();
    regs[9] = 40;
    mem[0] = R_TYPE(9, 0, 0, 0x08);
    run_instruction();
    assert(pc == 40 && !sim_error);
    printf("Test JR pasado.\n");
}

static void test_addi() {
    reset_cpu();
    regs[9] = 10;
    mem[0] = I_TYPE(0x08, 9, 8, 15);
    run_instruction();
    assert(regs[8] == 25 && !sim_error);
    printf("Test ADDI pasado.\n");
}

static void test_lw() {
    reset_cpu();
    mem[10] = 1234; // dirección 40
    regs[9] = 0;
    mem[0] = I_TYPE(0x23, 9, 8, 40);
    run_instruction();
    assert(regs[8] == 1234 && !sim_error);
    printf("Test LW pasado.\n");
}

static void test_sw() {
    reset_cpu();
    regs[9] = 0; regs[10] = 999;
    mem[0] = I_TYPE(0x2B, 9, 10, 80); // guarda regs[10] en dirección 80
    run_instruction();
    assert(mem[80 / 4] == 999 && !sim_error);
    printf("Test SW pasado.\n");
}

static void test_beq() {
    reset_cpu();
    regs[9] = 5; regs[10] = 5;
    mem[0] = I_TYPE(0x04, 9, 10, 3); // salta si son iguales: target = 4 + (3<<2) = 16
    run_instruction();
    assert(pc == 16 && !sim_error);
    printf("Test BEQ pasado.\n");
}

static void test_bne() {
    reset_cpu();
    regs[9] = 5; regs[10] = 6;
    mem[0] = I_TYPE(0x05, 9, 10, 2); // salta si son distintos: target = 4 + (2<<2) = 12
    run_instruction();
    assert(pc == 12 && !sim_error);
    printf("Test BNE pasado.\n");
}

static void test_j() {
    reset_cpu();
    mem[0] = J_TYPE(0x02, 10); // target = (pc_plus4 & 0xF0000000) | (10<<2) = 40
    run_instruction();
    assert(pc == 40 && !sim_error);
    printf("Test J pasado.\n");
}

// ======================================================================
// Manejo de entradas no esperadas
// ======================================================================
static void test_opcode_desconocido() {
    reset_cpu();
    mem[0] = I_TYPE(0x3F, 0, 0, 0); // opcode inexistente en el ISA
    run_instruction();
    assert(sim_error == true);
    printf("Test opcode desconocido -> error detectado correctamente.\n");
}

static void test_direccion_no_alineada() {
    reset_cpu();
    regs[9] = 0;
    mem[0] = I_TYPE(0x23, 9, 8, 1); // lw dirección 1 (no alineada a 4)
    run_instruction();
    assert(sim_error == true);
    assert(regs[8] == 0); // no debe escribirse por el error
    printf("Test direccion no alineada -> error detectado correctamente.\n");
}

static void test_direccion_fuera_de_rango() {
    reset_cpu();
    regs[9] = 0;
    mem[0] = I_TYPE(0x23, 9, 8, 5000); // dirección 5000 > MEM_SIZE*4 (4096)
    run_instruction();
    assert(sim_error == true);
    assert(regs[8] == 0);
    printf("Test direccion fuera de rango -> error detectado correctamente.\n");
}

static void test_escritura_registro_cero() {
    reset_cpu();
    regs[9] = 1; regs[10] = 1;
    mem[0] = R_TYPE(9, 10, 0, 0x20); // add $zero,$t1,$t2 -> debe ignorarse
    run_instruction();
    assert(regs[0] == 0 && !sim_error);
    printf("Test escritura en $zero ignorada correctamente.\n");
}

// ======================================================================
// Test de sistema: programa completo, valida registros y memoria finales
// ======================================================================
static void test_sistema() {
    reset_cpu();
    // addi $t1,$zero,5
    mem[0] = I_TYPE(0x08, 0, 9, 5);
    // addi $t2,$zero,7
    mem[1] = I_TYPE(0x08, 0, 10, 7);
    // add  $t0,$t1,$t2
    mem[2] = R_TYPE(9, 10, 8, 0x20);
    // sw   $t0, 800($zero)
    mem[3] = I_TYPE(0x2B, 0, 8, 800);
    // lw   $t3, 800($zero)
    mem[4] = I_TYPE(0x23, 0, 11, 800);

    for (int ciclo = 0; ciclo < 5; ciclo++) {
        run_instruction();
    }

    assert(regs[9] == 5);
    assert(regs[10] == 7);
    assert(regs[8] == 12);
    assert(mem[800 / 4] == 12);
    assert(regs[11] == 12);
    assert(!sim_error);
    printf("Test de sistema pasado.\n");
}

void run_all_tests() {
    test_IF(); test_ID(); test_ALU(); test_MEM(); test_WB();

    test_add(); test_sub(); test_and(); test_or(); test_nor(); test_xor(); test_jr();
    test_addi(); test_lw(); test_sw(); test_beq(); test_bne(); test_j();

    test_opcode_desconocido();
    test_direccion_no_alineada();
    test_direccion_fuera_de_rango();
    test_escritura_registro_cero();

    test_sistema();

    printf("¡Todos los unit tests pasaron!\n");
}
