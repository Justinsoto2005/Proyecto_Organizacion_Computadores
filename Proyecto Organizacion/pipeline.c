#include "pipeline.h"

uint32_t regs[32] = {0};
uint32_t mem[1024] = {0};
uint32_t pc = 0;

IF_ID if_id = {0};
ID_EX id_ex = {0};
EX_MEM ex_mem = {0};
MEM_WB mem_wb = {0};

void instruction_fetch() {
    if_id.inst = mem[pc / 4];
    if_id.pc_plus_4 = pc + 4;
    pc += 4;
}

void instruction_decode() {
    uint32_t inst = if_id.inst;
    id_ex.opcode = (inst >> 26) & 0x3F;
    id_ex.rs = (inst >> 21) & 0x1F;
    id_ex.rt = (inst >> 16) & 0x1F;
    id_ex.rd = (inst >> 11) & 0x1F;
    id_ex.shamt = (inst >> 6) & 0x1F;
    id_ex.funct = inst & 0x3F;
    
    id_ex.read_data1 = regs[id_ex.rs];
    id_ex.read_data2 = regs[id_ex.rt];
    
    // Extensión de signo para inmediatos
    int16_t imm = inst & 0xFFFF;
    id_ex.sign_ext_imm = (uint32_t)imm; 
    id_ex.pc_plus_4 = if_id.pc_plus_4;
}

void alu_execute() {
    ex_mem.opcode = id_ex.opcode;
    ex_mem.write_data = id_ex.read_data2;
    ex_mem.reg_write = false; ex_mem.mem_read = false; ex_mem.mem_write = false; ex_mem.mem_to_reg = false;

    if (id_ex.opcode == 0) { // Tipo R
        ex_mem.dest_reg = id_ex.rd;
        ex_mem.reg_write = true;
        switch (id_ex.funct) {
            case 0x20: ex_mem.alu_out = id_ex.read_data1 + id_ex.read_data2; break; // add
            case 0x22: ex_mem.alu_out = id_ex.read_data1 - id_ex.read_data2; break; // sub
            case 0x24: ex_mem.alu_out = id_ex.read_data1 & id_ex.read_data2; break; // and
            case 0x25: ex_mem.alu_out = id_ex.read_data1 | id_ex.read_data2; break; // or
            case 0x26: ex_mem.alu_out = ~(id_ex.read_data1 | id_ex.read_data2); break; // nor
            case 0x27: ex_mem.alu_out = id_ex.read_data1 ^ id_ex.read_data2; break; // xor
            case 0x08: pc = id_ex.read_data1; ex_mem.reg_write = false; break; // jr
        }
    } else { // Tipo I o J
        ex_mem.dest_reg = id_ex.rt;
        switch (id_ex.opcode) {
            case 0x08: // addi
                ex_mem.alu_out = id_ex.read_data1 + id_ex.sign_ext_imm;
                ex_mem.reg_write = true; break;
            case 0x23: // lw
                ex_mem.alu_out = id_ex.read_data1 + id_ex.sign_ext_imm;
                ex_mem.mem_read = true; ex_mem.reg_write = true; ex_mem.mem_to_reg = true; break;
            case 0x2B: // sw
                ex_mem.alu_out = id_ex.read_data1 + id_ex.sign_ext_imm;
                ex_mem.mem_write = true; break;
            case 0x04: // beq
                if (id_ex.read_data1 == id_ex.read_data2) pc = id_ex.pc_plus_4 + (id_ex.sign_ext_imm << 2); break;
            case 0x05: // bne
                if (id_ex.read_data1 != id_ex.read_data2) pc = id_ex.pc_plus_4 + (id_ex.sign_ext_imm << 2); break;
            case 0x02: // j
                pc = (id_ex.pc_plus_4 & 0xF0000000) | ((if_id.inst & 0x3FFFFFF) << 2); break;
        }
    }
}

void memory_access() {
    mem_wb.reg_write = ex_mem.reg_write;
    mem_wb.mem_to_reg = ex_mem.mem_to_reg;
    mem_wb.dest_reg = ex_mem.dest_reg;
    mem_wb.alu_out = ex_mem.alu_out;
    
    if (ex_mem.mem_write) mem[ex_mem.alu_out / 4] = ex_mem.write_data;
    if (ex_mem.mem_read) mem_wb.read_data = mem[ex_mem.alu_out / 4];
}

void write_back() {
    if (mem_wb.reg_write && mem_wb.dest_reg != 0) {
        if (mem_wb.mem_to_reg) {
            regs[mem_wb.dest_reg] = mem_wb.read_data;
        } else {
            regs[mem_wb.dest_reg] = mem_wb.alu_out;
        }
    }
}