#ifndef PIPELINE_H
#define PIPELINE_H
#include <stdint.h>
#include <stdbool.h>

#define MEM_SIZE 1024 // tamaño del arreglo mem[] en palabras de 32 bits

extern uint32_t regs[32];
extern uint32_t mem[MEM_SIZE];
extern uint32_t pc;

// ---------------------------------------------------------------------
// Manejo de errores explícito (opcodes/funct no reconocidos, direcciones
// de memoria fuera de rango o no alineadas, overflow en la ALU).
// Se documenta el comportamiento adoptado: cuando ocurre un error, la
// etapa afectada NO modifica el estado arquitectónico (no escribe en
// memoria ni marca reg_write) y se registra el motivo en sim_error_msg
// para que las pruebas y el modo de depuración puedan inspeccionarlo.
// ---------------------------------------------------------------------
extern bool sim_error;
extern char sim_error_msg[128];
void clear_error();
void raise_error(const char *msg);

// Registros intermedios del Pipeline
typedef struct { uint32_t inst; uint32_t pc_plus_4; } IF_ID;

typedef struct {
    uint32_t pc_plus_4; uint32_t read_data1; uint32_t read_data2;
    uint32_t sign_ext_imm; uint32_t rs; uint32_t rt; uint32_t rd;
    uint32_t addr26; // campo address de 26 bits (solo se usa en tipo J)
    uint8_t opcode; uint8_t funct; uint8_t shamt;
    // Señales de control generadas en Decode (control_unit), tal como
    // define el Architecture Freeze: "no en ALU".
    bool reg_write; bool mem_read; bool mem_write; bool mem_to_reg;
    bool is_branch_eq; bool is_branch_ne; bool is_jump; bool is_jr;
    bool alu_src_imm; // 1 = el segundo operando de la ALU es el inmediato
    bool unknown_op;  // opcode/funct no reconocido -> error explícito
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

// ------------------- Etapas (interfaz pública) -------------------
void instruction_fetch();
void instruction_decode();
void alu_execute();
void memory_access();
void write_back();

// ------------------- Subfunciones por módulo -------------------
// IF
uint32_t fetch_instruction(uint32_t pc_actual);
uint32_t compute_pc_plus4(uint32_t pc_actual);

// ID
void decode_fields(uint32_t inst, uint8_t *opcode, uint8_t *rs, uint8_t *rt,
                    uint8_t *rd, uint8_t *shamt, uint8_t *funct, uint16_t *imm16);
uint32_t sign_extend(uint16_t imm16);
void read_reg_file(uint8_t rs, uint8_t rt, uint32_t *data1, uint32_t *data2);
void control_unit(uint8_t opcode, uint8_t funct, ID_EX *out);

// EX / ALU
uint8_t alu_control(uint8_t opcode, uint8_t funct, bool *op_reconocida);
uint32_t alu_operate(uint8_t alu_op, uint32_t a, uint32_t b, bool *zero, bool *overflow);
uint32_t compute_branch_target(uint32_t pc_plus4, uint32_t sign_ext_imm);

// MEM
bool check_alignment(uint32_t addr);
uint32_t mem_read_word(uint32_t addr, bool *ok);
void mem_write_word(uint32_t addr, uint32_t data, bool *ok);

// WB
uint32_t select_writeback_value(const MEM_WB *stage);
void write_reg_file(uint32_t dest_reg, uint32_t value);

// Modo paso a paso (extra "Innovador" del Architecture Freeze): cuando
// está activo, cada etapa imprime en consola la instrucción/valores que
// procesa en ese ciclo.
extern bool step_mode;

#endif
