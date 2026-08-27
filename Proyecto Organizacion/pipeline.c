#include <stdio.h>
#include "pipeline.h"

uint32_t regs[32] = {0};
uint32_t mem[MEM_SIZE] = {0};
uint32_t pc = 0;

IF_ID if_id = {0};
ID_EX id_ex = {0};
EX_MEM ex_mem = {0};
MEM_WB mem_wb = {0};

bool sim_error = false;
char sim_error_msg[128] = "";
bool step_mode = false;

// Códigos internos de operación de la ALU (usados por alu_control/alu_operate)
#define ALUOP_ADD 0
#define ALUOP_SUB 1
#define ALUOP_AND 2
#define ALUOP_OR  3
#define ALUOP_NOR 4
#define ALUOP_XOR 5

void clear_error() {
    sim_error = false;
    sim_error_msg[0] = '\0';
}

void raise_error(const char *msg) {
    sim_error = true;
    snprintf(sim_error_msg, sizeof(sim_error_msg), "%s", msg);
    fprintf(stderr, "[ERROR] %s\n", msg);
}

// ======================================================================
// IF - Instruction Fetch
// ======================================================================
uint32_t fetch_instruction(uint32_t pc_actual) {
    uint32_t idx = pc_actual / 4;
    if (idx >= MEM_SIZE) {
        raise_error("IF: PC fuera de rango de memoria");
        return 0;
    }
    return mem[idx];
}

uint32_t compute_pc_plus4(uint32_t pc_actual) {
    return pc_actual + 4;
}

void instruction_fetch() {
    if (step_mode) printf("[IF ] pc=%u\n", pc);
    if_id.inst = fetch_instruction(pc);
    if_id.pc_plus_4 = compute_pc_plus4(pc);
    pc = if_id.pc_plus_4;
}

// ======================================================================
// ID - Instruction Decode
// ======================================================================
void decode_fields(uint32_t inst, uint8_t *opcode, uint8_t *rs, uint8_t *rt,
                    uint8_t *rd, uint8_t *shamt, uint8_t *funct, uint16_t *imm16) {
    *opcode = (inst >> 26) & 0x3F;
    *rs     = (inst >> 21) & 0x1F;
    *rt     = (inst >> 16) & 0x1F;
    *rd     = (inst >> 11) & 0x1F;
    *shamt  = (inst >> 6)  & 0x1F;
    *funct  = inst & 0x3F;
    *imm16  = inst & 0xFFFF;
}

uint32_t sign_extend(uint16_t imm16) {
    int16_t signed_imm = (int16_t)imm16;
    return (uint32_t)(int32_t)signed_imm;
}

void read_reg_file(uint8_t rs, uint8_t rt, uint32_t *data1, uint32_t *data2) {
    *data1 = regs[rs];
    *data2 = regs[rt];
}

// Genera TODAS las señales de control en Decode (no en la ALU), tal como
// quedó decidido en el Architecture Freeze.
void control_unit(uint8_t opcode, uint8_t funct, ID_EX *out) {
    out->reg_write = false; out->mem_read = false; out->mem_write = false; out->mem_to_reg = false;
    out->is_branch_eq = false; out->is_branch_ne = false; out->is_jump = false; out->is_jr = false;
    out->alu_src_imm = false; out->unknown_op = false;

    if (opcode == 0x00) { // Tipo R
        switch (funct) {
            case 0x20: case 0x22: case 0x24: case 0x25: case 0x26: case 0x27: // add,sub,and,or,nor,xor
                out->reg_write = true;
                break;
            case 0x08: // jr
                out->is_jr = true;
                break;
            default:
                out->unknown_op = true;
        }
        return;
    }

    switch (opcode) {
        case 0x08: // addi
            out->reg_write = true; out->alu_src_imm = true; break;
        case 0x23: // lw
            out->reg_write = true; out->mem_read = true; out->mem_to_reg = true; out->alu_src_imm = true; break;
        case 0x2B: // sw
            out->mem_write = true; out->alu_src_imm = true; break;
        case 0x04: // beq
            out->is_branch_eq = true; break;
        case 0x05: // bne
            out->is_branch_ne = true; break;
        case 0x02: // j
            out->is_jump = true; break;
        default:
            out->unknown_op = true;
    }
}

void instruction_decode() {
    uint32_t inst = if_id.inst;
    uint8_t opcode, rs, rt, rd, shamt, funct;
    uint16_t imm16;

    decode_fields(inst, &opcode, &rs, &rt, &rd, &shamt, &funct, &imm16);

    id_ex.opcode = opcode; id_ex.rs = rs; id_ex.rt = rt; id_ex.rd = rd; id_ex.shamt = shamt;
    id_ex.funct = funct;
    id_ex.sign_ext_imm = sign_extend(imm16);
    id_ex.addr26 = inst & 0x3FFFFFF;
    id_ex.pc_plus_4 = if_id.pc_plus_4;

    read_reg_file(rs, rt, &id_ex.read_data1, &id_ex.read_data2);

    control_unit(opcode, funct, &id_ex);

    if (step_mode)
        printf("[ID ] opcode=0x%02X rs=%u rt=%u rd=%u imm=%d\n", opcode, rs, rt, rd, (int)id_ex.sign_ext_imm);

    if (id_ex.unknown_op) {
        raise_error("ID: opcode/funct no reconocido");
    }
}

// ======================================================================
// EX - ALU / Execute
// ======================================================================
uint8_t alu_control(uint8_t opcode, uint8_t funct, bool *op_reconocida) {
    *op_reconocida = true;
    if (opcode == 0x00) {
        switch (funct) {
            case 0x20: return ALUOP_ADD;
            case 0x22: return ALUOP_SUB;
            case 0x24: return ALUOP_AND;
            case 0x25: return ALUOP_OR;
            case 0x26: return ALUOP_NOR;
            case 0x27: return ALUOP_XOR;
            default:
                *op_reconocida = false;
                return ALUOP_ADD;
        }
    }
    switch (opcode) {
        case 0x08: case 0x23: case 0x2B: // addi, lw, sw -> siempre suman
            return ALUOP_ADD;
        default:
            *op_reconocida = false;
            return ALUOP_ADD;
    }
}

uint32_t alu_operate(uint8_t alu_op, uint32_t a, uint32_t b, bool *zero, bool *overflow) {
    uint32_t result = 0;
    *overflow = false;

    switch (alu_op) {
        case ALUOP_ADD: {
            int32_t sa = (int32_t)a, sb = (int32_t)b;
            int64_t full = (int64_t)sa + (int64_t)sb;
            result = (uint32_t)(sa + sb);
            *overflow = (full != (int64_t)(int32_t)full);
            break;
        }
        case ALUOP_SUB: {
            int32_t sa = (int32_t)a, sb = (int32_t)b;
            int64_t full = (int64_t)sa - (int64_t)sb;
            result = (uint32_t)(sa - sb);
            *overflow = (full != (int64_t)(int32_t)full);
            break;
        }
        case ALUOP_AND: result = a & b; break;
        case ALUOP_OR:  result = a | b; break;
        case ALUOP_NOR: result = ~(a | b); break;
        case ALUOP_XOR: result = a ^ b; break;
        default: break;
    }

    *zero = (result == 0);
    return result;
}

uint32_t compute_branch_target(uint32_t pc_plus4, uint32_t sign_ext_imm) {
    return pc_plus4 + (sign_ext_imm << 2);
}

void alu_execute() {
    ex_mem.opcode = id_ex.opcode;
    ex_mem.reg_write = id_ex.reg_write;
    ex_mem.mem_read = id_ex.mem_read;
    ex_mem.mem_write = id_ex.mem_write;
    ex_mem.mem_to_reg = id_ex.mem_to_reg;
    ex_mem.dest_reg = (id_ex.opcode == 0x00) ? id_ex.rd : id_ex.rt;
    ex_mem.write_data = id_ex.read_data2;
    ex_mem.alu_out = 0;

    if (step_mode)
        printf("[EX ] opcode=0x%02X funct=0x%02X d1=%u d2=%u\n",
               id_ex.opcode, id_ex.funct, id_ex.read_data1, id_ex.read_data2);

    if (id_ex.unknown_op) {
        raise_error("EX: instrucción con opcode/funct no reconocido, no se ejecuta");
        return;
    }

    if (id_ex.is_jr) {
        pc = id_ex.read_data1;
        return;
    }

    if (id_ex.is_jump) {
        pc = (id_ex.pc_plus_4 & 0xF0000000) | (id_ex.addr26 << 2);
        return;
    }

    if (id_ex.is_branch_eq || id_ex.is_branch_ne) {
        bool zero = false, overflow = false;
        alu_operate(ALUOP_SUB, id_ex.read_data1, id_ex.read_data2, &zero, &overflow);
        bool tomar_salto = (id_ex.is_branch_eq && zero) || (id_ex.is_branch_ne && !zero);
        if (tomar_salto) {
            pc = compute_branch_target(id_ex.pc_plus_4, id_ex.sign_ext_imm);
        }
        return;
    }

    // Resto de instrucciones: tipo R aritmético/lógico, o addi/lw/sw
    bool op_reconocida;
    uint8_t aluop = alu_control(id_ex.opcode, id_ex.funct, &op_reconocida);
    if (!op_reconocida) {
        raise_error("EX: combinación opcode/funct sin operación ALU definida");
        return;
    }

    uint32_t operand_b = id_ex.alu_src_imm ? id_ex.sign_ext_imm : id_ex.read_data2;
    bool zero = false, overflow = false;
    ex_mem.alu_out = alu_operate(aluop, id_ex.read_data1, operand_b, &zero, &overflow);

    if (overflow) {
        // Comportamiento documentado: se reporta el overflow pero la
        // simulación continúa con el resultado truncado a 32 bits.
        raise_error("EX: overflow detectado en la ALU (add/sub); resultado truncado a 32 bits");
    }
}

// ======================================================================
// MEM - Memory
// ======================================================================
bool check_alignment(uint32_t addr) {
    return (addr % 4) == 0;
}

uint32_t mem_read_word(uint32_t addr, bool *ok) {
    *ok = check_alignment(addr) && (addr / 4) < MEM_SIZE;
    if (!*ok) return 0;
    return mem[addr / 4];
}

void mem_write_word(uint32_t addr, uint32_t data, bool *ok) {
    *ok = check_alignment(addr) && (addr / 4) < MEM_SIZE;
    if (!*ok) return;
    mem[addr / 4] = data;
}

void memory_access() {
    mem_wb.reg_write = ex_mem.reg_write;
    mem_wb.mem_to_reg = ex_mem.mem_to_reg;
    mem_wb.dest_reg = ex_mem.dest_reg;
    mem_wb.alu_out = ex_mem.alu_out;
    mem_wb.read_data = 0;

    if (step_mode)
        printf("[MEM] addr=%u mem_read=%d mem_write=%d\n", ex_mem.alu_out, ex_mem.mem_read, ex_mem.mem_write);

    if (ex_mem.mem_write) {
        bool ok;
        mem_write_word(ex_mem.alu_out, ex_mem.write_data, &ok);
        if (!ok) {
            raise_error("MEM: dirección fuera de rango o no alineada en sw");
            mem_wb.reg_write = false;
        }
    }
    if (ex_mem.mem_read) {
        bool ok;
        mem_wb.read_data = mem_read_word(ex_mem.alu_out, &ok);
        if (!ok) {
            raise_error("MEM: dirección fuera de rango o no alineada en lw");
            mem_wb.reg_write = false;
        }
    }
}

// ======================================================================
// WB - Write Back
// ======================================================================
uint32_t select_writeback_value(const MEM_WB *stage) {
    return stage->mem_to_reg ? stage->read_data : stage->alu_out;
}

void write_reg_file(uint32_t dest_reg, uint32_t value) {
    if (dest_reg != 0 && dest_reg < 32) {
        regs[dest_reg] = value;
    }
}

void write_back() {
    if (step_mode)
        printf("[WB ] reg_write=%d dest=%u\n", mem_wb.reg_write, mem_wb.dest_reg);

    if (!mem_wb.reg_write) return;

    uint32_t value = select_writeback_value(&mem_wb);
    write_reg_file(mem_wb.dest_reg, value);
}
