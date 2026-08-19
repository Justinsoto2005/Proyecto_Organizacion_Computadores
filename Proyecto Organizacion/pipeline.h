#ifndef PIPELINE_H
#define PIPELINE_H
#include <stdint.h>
#include <stdbool.h>

extern uint32_t regs[32];
extern uint32_t mem[1024];
extern uint32_t pc;

// Registros intermedios del Pipeline
typedef struct { uint32_t inst; uint32_t pc_plus_4; } IF_ID;
typedef struct {
    uint32_t pc_plus_4; uint32_t read_data1; uint32_t read_data2;
    uint32_t sign_ext_imm; uint32_t rs; uint32_t rt; uint32_t rd;
    uint8_t opcode; uint8_t funct; uint8_t shamt;
} ID_EX;
typedef struct {
    uint32_t alu_out; uint32_t write_data; uint32_t dest_reg;
    uint8_t opcode; bool reg_write; bool mem_read; bool mem_write; bool mem_to_reg;
} EX_MEM;
typedef struct {
    uint32_t read_data; uint32_t alu_out; uint32_t dest_reg;
    bool reg_write; bool mem_to_reg;
} MEM_WB;

// Variables globales de las etapas
extern IF_ID if_id;
extern ID_EX id_ex;
extern EX_MEM ex_mem;
extern MEM_WB mem_wb;

void instruction_fetch();
void instruction_decode();
void alu_execute();
void memory_access();
void write_back();

#endif