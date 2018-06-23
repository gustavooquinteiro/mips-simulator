#ifndef _SIM_H_
#define _SIM_H_

// Declaração de constantes para os comandos MIPS e outras constantes recorrentes ao longo do código
#define V0 2
#define RA 31
#define ZERO 0
#define TWO 2
#define HEX_ONE 0x1
#define ALL_32_BITS_1 0xFFFFFFFF
#define FIRST_24_BITS_1 0xFFFFFF00
#define LAST_24_BITS_1 0xFFFFFF
#define LAST_5_BITS_1 0x1F
#define FIRST_16_BITS_1 0xFFFF
#define LAST_16_BITS_1 0xFFFF0000
#define FIFTH_BIT_1 0x10
#define EIGHTH_BIT_1 0x80
#define TENTH_SIXTH_BIT_1 0x8000
#define LAST_5_BITS_0_IN_HALFWORD 0xFFE0
#define FIRST_5_BITS_0_IN_WORD 0x03FFFFFF
#define SIXTH_BIT_1_IN_WORD 0x2000000
#define FIRST_4_BITS_1 0xF0000000
#define BITS_NUMBER 32
#define BYTE 8
#define HALFWORD 16 
#define FOUR 0x00000004
#define EXIT_VALUE 0x0A
#define FUNCTION_BITS 26
#define RD_BITS 11
#define RS_BITS 21
#define RT_BITS 16
#define IMMEDIATE_BITS 16
#define SHAMT_BITS 6 
#define OFFSET_BITS 6
#define DEFAULT_MASK 0x1F	
#define BLTZ 0x0
#define SLL 0x0
#define BGEZ 0x1
#define J 0x2
#define SRL 0x2
#define JAL 0x3
#define SRA 0x3
#define BEQ 0x4
#define SLLV 0x4
#define BNE 0x5
#define BLEZ 0x6
#define SRLV 0x6
#define BGTZ 0x7
#define SRAV 0x7
#define ADDI 0x8
#define JR 0x8 
#define ADDIU 0x9
#define JALR 0x9
#define SLTI 0xA
#define SLTIU 0xB
#define ANDI 0xC
#define SYSCALL 0xC
#define ORI 0xD
#define XORI 0xE
#define LUI 0xF
#define BLTZAL 0x10
#define MFHI 0x10
#define BGEZAL 0x11
#define MTHI 0x11
#define MFLO 0x12
#define MTLO 0x13
#define MULT 0x18
#define MULTU 0x19
#define DIV 0x1A
#define DIVU 0x1B
#define ADD 0x20 
#define LB 0x20
#define ADDU 0x21 
#define LH 0x21
#define SUB 0x22
#define LW 0x23
#define SUBU 0x23
#define AND 0x24
#define LBU 0x24
#define LHU 0x25
#define OR 0x25
#define XOR 0x26
#define NOR 0x27 
#define SB 0x28
#define SLT 0x2A
#define SH 0x29
#define SLTU 0x2B
#define SW 0x2B

// Função para realizar o sign-extend em 8 bits
int32_t signExtend_8Bits(uint32_t  immediate);

// Função para realizar o sign-extend, e se necessário com alguma quantidade de shift, em 16 bits
int32_t signExtend_16Bits(uint32_t immediate, uint32_t shift);

// Função para realizar o sign-extend, e o shift left de 2 bits, em 26 bits
int32_t signExtend_26Bits_shiftLeft_2(uint32_t address);

// Função para realizar o complemento de 2 no operando
int32_t twosComplement(uint32_t operand);

// Função para realizar o zero-extend 
int32_t zeroExtend(uint32_t immediate);

// Função para realizar o sign-extend em 5 bits
int32_t signExtend_5Bits(uint32_t shamt);

// Função que retorna do endereço de memória fornecido somente os LSB bits solicitados
uint32_t readFromMemory_Xbits(uint32_t offset, uint32_t bits);

// Função para extrair do registrador os LSB bits solicitados 
uint32_t extractBitsFromReg(uint32_t bits, uint32_t reg);

// Função de link para instruções do tipo de jump-and-link, armazenando o PC+4 no registrador, geralmente em RA 
void link(uint32_t reg);

// Função para atribuir à PC a próxima instrução 
void nextInstruction();

// Função para atualizar os registradores HI e LO 
void updateHIandLO (int64_t result);

// Função para executar as funções de carregar valores da memória
void loadInstructions(uint32_t function, uint32_t rs, uint32_t rt, int32_t immediate);

// Função para executar as instruções de guardar valores na memória
void storeInstructions(uint32_t function, uint32_t rs, uint32_t rt, int32_t immediate);

// Função para executar as instruções de branch 
void branchInstructions(uint32_t function, uint32_t rs, uint32_t rt, int32_t offset);

// Função para executar as instruções de shift 
void shiftInstructions (uint32_t func, uint32_t rt, uint32_t rd, uint32_t shamt, uint32_t rs);

// ALU para os registradores do tipo R
void RTypeALU(uint32_t func, uint32_t rs, uint32_t rt, uint32_t rd, uint32_t shamt);

// ALU para os registradores do tipo I
void ITypeALU(uint32_t opcode, uint32_t rs, uint32_t rt, uint32_t immediate);

// ALU para os registradores do tipo J, calculando o address, extendendo o sinal com shift-left 2
void JTypeALU(uint32_t func, uint32_t address);

/* Função que recebe o código hex do PC e separa os espaços de opcode
 * e, de acordo o tipo do registrador, o rs, o rt,  o rd, shamt, address
 * chamando a ALU correspondente para executar a instrução */
void CPU (uint32_t instruction);

// Chama a função para fragmentar o código hex contido no PC do estado atual 
void process_instruction();
#endif
