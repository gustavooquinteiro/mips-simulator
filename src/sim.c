#include <stdio.h>
#include "shell.h"
// Declaração de constantes para os comandos MIPS e outras constantes recorrentes ao longo do código
#define V0 2
#define RA 31
#define ZERO 0
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
// Declaração de variáveis globais
uint32_t opcode, rs, rt, rd, address, immediate, shamt, function; 
int32_t offset;
// Função de link para instruções do tipo de jump-and-link, armazenando o PC+4 no registrador, geralmente em RA 
void link(uint32_t reg){
	NEXT_STATE.REGS[reg] = CURRENT_STATE.PC + FOUR;
}
// Função para atribuir à PC a próxima instrução 
void nextInstruction(){
	NEXT_STATE.PC += FOUR; 
}
// Função para atualizar os registradores HI e LO 
void updateHIandLO (int64_t result){
	NEXT_STATE.HI = (result >> 32) & 0xFFFFFFFF;
	NEXT_STATE.LO = result & 0xFFFFFFFF;
}
// Função que retorna do endereço de memória fornecido somente os LSB bits solicitados
uint32_t readFromMemory_Xbits(uint32_t offset, uint32_t bits){
	uint32_t shift = 32 - bits;
	return (mem_read_32(offset) << shift) >> shift;
}
// Função para realizar o sign-extend em 5 bits
int32_t signExtend_5Bits(uint32_t shamt){
	offset = (shamt & 0x1F);
	if (offset & 0x10)
		offset |= 0xFFE0;
	return offset;
}
// Função para realizar o sign-extend em 8 bits
int32_t signExtend_8Bits(uint32_t  immediate){
	offset = (immediate & 0xFFFFFF);
	if (offset & (0x80))
		offset |= 0xFFFFFF00; 
	return offset;
}
// Função para realizar o sign-extend, e se necessário com alguma quantidade de shift, em 16 bits
int32_t signExtend_16Bits(uint32_t immediate, uint32_t shift){
	offset = (immediate & 0xFFFF) << shift;
	if(offset & (0x8000 << shift)) 
		offset |= 0xFFFF0000;
	return offset;
}
// Função para realizar o sign-extend, e o shift left de 2 bits, em 26 bits
int32_t signExtend_26Bits_shiftLeft_2(uint32_t address){
	offset = (address & 0x03FFFFFF) << 2;
	if(offset & (0x2000000 << 2)) 
		offset |= 0xF0000000;
	return offset;	
}
// Função para realizar o zero-extend 
int32_t zeroExtend(uint32_t immediate){
	return immediate | ZERO;
}
// Função para realizar o complemento de 2 no operando
int32_t twosComplement(uint32_t operand){
	return (~operand) + 0x1;
}
// Função para extrair do registrador os LSB bits solicitados 
uint32_t extractBitsFromReg(uint32_t bits, uint32_t reg){
	uint32_t shift = 32 - bits; 
	return (CURRENT_STATE.REGS[reg] << shift) >> shift;
}
// Função para executar as funções de carregar valores da memória
void loadInstructions(uint32_t function, uint32_t rs, uint32_t rt, int32_t immediate){
	address = immediate + CURRENT_STATE.REGS[rs]; 
	switch (function){	
		case LB:
			NEXT_STATE.REGS[rt] = signExtend_8Bits(readFromMemory_Xbits(address, BYTE));
		break;
		case LH:		
			NEXT_STATE.REGS[rt] = signExtend_16Bits(readFromMemory_Xbits(address, HALFWORD), ZERO);
		break;
		case LW:
			NEXT_STATE.REGS[rt] = mem_read_32(address);
		break;
		case LBU:
			NEXT_STATE.REGS[rt] = zeroExtend(readFromMemory_Xbits(address, BYTE));
		break;
		case LHU:
			NEXT_STATE.REGS[rt] = zeroExtend(readFromMemory_Xbits(address, HALFWORD));
		break;
	}
}
// Função para executar as instruções de guardar valores na memória
void storeInstructions(uint32_t function, uint32_t rs, uint32_t rt, int32_t immediate){
	address = immediate + CURRENT_STATE.REGS[rs]; 
	switch(function){
		case SB:			
			mem_write_32(address, extractBitsFromReg(BYTE, rt)); 			
		break; 
		case SH:
			mem_write_32(address, extractBitsFromReg(HALFWORD, rt));
		break;
		case SW:
			mem_write_32(address, CURRENT_STATE.REGS[rt]); 
		break;
	}
}
// Função para executar as instruções de branch 
void branchInstructions(uint32_t function, uint32_t rs, uint32_t rt, int32_t offset){
	switch(function){
		case BEQ:
			if ((int32_t)CURRENT_STATE.REGS[rs] == (int32_t)CURRENT_STATE.REGS[rt])
				NEXT_STATE.PC += offset;
		break;
		case BNE:
			if ((int32_t)CURRENT_STATE.REGS[rs] != (int32_t)CURRENT_STATE.REGS[rt])
				NEXT_STATE.PC += offset;
		break;		
		case BLEZ:
			if ((int32_t)CURRENT_STATE.REGS[rs] <= ZERO)
				NEXT_STATE.PC += offset;
		break;				
		case BGTZ:
			if ((int32_t)CURRENT_STATE.REGS[rs] > ZERO)
				NEXT_STATE.PC += offset; 
		break;
		default:
			switch(rt){
				case BLTZ:
				case BLTZAL:
					if ((int32_t)CURRENT_STATE.REGS[rs] < ZERO){
						if (rt == BLTZAL)
							link(RA);
						NEXT_STATE.PC += offset;
					}
				break;
				case BGEZ:
				case BGEZAL:
					if ((int32_t)CURRENT_STATE.REGS[rs] >= ZERO){
						if (rt == BGEZAL)
							link(RA);
						NEXT_STATE.PC += offset;
					}
				break;
			}
		break;
	}
}
// Função para executar as instruções de shift 
void shiftInstructions (uint32_t func, uint32_t rt, uint32_t rd, uint32_t shamt){
	switch(func){
		case SLL:
			NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rt] << shamt; 
		break;
		case SRL:
			NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rt] >> shamt;
		break;
		case SRA:
			NEXT_STATE.REGS[rd] = ((int32_t) CURRENT_STATE.REGS[rt]) >> shamt;  
		break;
		case SRAV:
			NEXT_STATE.REGS[rd] = ((int32_t) CURRENT_STATE.REGS[rt]) >> signExtend_5Bits(CURRENT_STATE.REGS[rs]);
		break;
		case SRLV:
			NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rt] >> CURRENT_STATE.REGS[rs];  
		break;
		case SLLV:
			NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rt] << signExtend_5Bits(CURRENT_STATE.REGS[rs] & DEFAULT_MASK); 
		break;
	}
}	
// ALU para os registradores do tipo R
void RTypeALU(uint32_t func, uint32_t rs, uint32_t rt, uint32_t rd, uint32_t shamt){
	int64_t result;
	uint64_t answer; 
	switch(func){
		case SRL:
		case SLL:
		case SRA:
		case SRLV:
		case SRAV:
		case SLLV: 
			shiftInstructions(func, rt, rd, shamt); 
		break;	
		case JR:
			NEXT_STATE.PC = CURRENT_STATE.REGS[rs];
		break;
		case SYSCALL:
			if (CURRENT_STATE.REGS[V0] == EXIT_VALUE)
				RUN_BIT = FALSE; 
		break;
		case MFHI:
			NEXT_STATE.REGS[rd] = CURRENT_STATE.HI;
		break;
		case MTHI:
			NEXT_STATE.HI = CURRENT_STATE.REGS[rs]; 
		break;
		case MFLO:
			NEXT_STATE.REGS[rd] = CURRENT_STATE.LO;
		break;
		case MTLO:
			NEXT_STATE.LO = CURRENT_STATE.REGS[rs]; 
		break; 
		case MULT:
			result = (int64_t)(twosComplement(CURRENT_STATE.REGS[rs]) * twosComplement(CURRENT_STATE.REGS[rt])); 
			updateHIandLO(result);
		break;
		case MULTU:
			answer = (uint64_t)CURRENT_STATE.REGS[rs] * CURRENT_STATE.REGS[rt]; 
			updateHIandLO(answer);
		break;
		case DIV:
			NEXT_STATE.HI = twosComplement(CURRENT_STATE.REGS[rs]) % twosComplement(CURRENT_STATE.REGS[rt]);
			NEXT_STATE.LO = twosComplement(CURRENT_STATE.REGS[rs]) / twosComplement(CURRENT_STATE.REGS[rt]); 
		break;
		case DIVU:
			NEXT_STATE.HI = CURRENT_STATE.REGS[rs] % CURRENT_STATE.REGS[rt];
			NEXT_STATE.LO = CURRENT_STATE.REGS[rs] / CURRENT_STATE.REGS[rt]; 
		break;
		case ADD:
		case ADDU:
			NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rs] + CURRENT_STATE.REGS[rt]; 
		break;
		case SUB:
		case SUBU:
			NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rs] - CURRENT_STATE.REGS[rt];
		break;
		case AND:
			NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rs] & CURRENT_STATE.REGS[rt];
		break;
		case OR: 
			NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rs] | CURRENT_STATE.REGS[rt];
		break;
		case XOR:
			NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rs] ^ CURRENT_STATE.REGS[rt];
		break;
		case NOR: 
			NEXT_STATE.REGS[rd] =  (~ CURRENT_STATE.REGS[rs]) & (~ CURRENT_STATE.REGS[rt]);
		break;
		case SLT:
			NEXT_STATE.REGS[rd] = (int32_t)CURRENT_STATE.REGS[rs] < (int32_t)CURRENT_STATE.REGS[rt];
		break;
		case SLTU:
			NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rs] < CURRENT_STATE.REGS[rt];
		break;
		case JALR:
			(rd == 0)? link(RA): link(rd);
			NEXT_STATE.PC = CURRENT_STATE.REGS[rs];
		break;
	}
}
// ALU para os registradores do tipo I
void ITypeALU(uint32_t opcode, uint32_t rs, uint32_t rt, uint32_t immediate){
	switch(opcode){
		case ADDI:
		case ADDIU:	 
			NEXT_STATE.REGS[rt] = CURRENT_STATE.REGS[rs] + signExtend_16Bits(immediate, ZERO); 
		break;
		case SLTI:	
			NEXT_STATE.REGS[rt] = (int32_t)CURRENT_STATE.REGS[rs] < signExtend_16Bits(immediate, ZERO); 
		break;
		case SLTIU:		
			NEXT_STATE.REGS[rt] = CURRENT_STATE.REGS[rs] < signExtend_16Bits(immediate, ZERO); 
		break;
		case ANDI:
			NEXT_STATE.REGS[rt] = CURRENT_STATE.REGS[rs] & zeroExtend(immediate);
		break;
		case ORI:
			NEXT_STATE.REGS[rt] = CURRENT_STATE.REGS[rs] | zeroExtend(immediate);
		break;
		case XORI:
			NEXT_STATE.REGS[rt] = CURRENT_STATE.REGS[rs] ^ zeroExtend(immediate);
		break;
		case LUI:
			NEXT_STATE.REGS[rt] = immediate  << IMMEDIATE_BITS;
		break;
		case LW:
		case LH:
		case LB:
		case LHU:
		case LBU:
			loadInstructions(opcode, rs, rt, signExtend_16Bits(immediate, ZERO));
		break;
		case SW:
		case SH:
		case SB:
			storeInstructions(opcode, rs, rt, signExtend_16Bits(immediate, ZERO));
		break;
		default:
			branchInstructions(opcode, rs, rt, signExtend_16Bits(immediate, 2));
		break;	
	}
}
// ALU para os registradores do tipo J, calculando o address, extendendo o sinal com shift-left 2
void JTypeALU(uint32_t func, uint32_t address){		
	if (func == JAL)
		link(RA);
	NEXT_STATE.PC = signExtend_26Bits_shiftLeft_2(address);  
}
/* Função que recebe o código hex do PC e separa os espaços de opcode
 * e, de acordo o tipo do registrador, o rs, o rt,  o rd, shamt, address
 * chamando a ALU correspondente para executar a instrução */
void CPU (uint32_t instruction){
	opcode = instruction >> FUNCTION_BITS;
	switch (opcode){
		// Registradores tipo R
		case ZERO: 		
			function = (instruction << FUNCTION_BITS) >> FUNCTION_BITS;
			rd = (instruction >> RD_BITS) & DEFAULT_MASK;		
			rs = (instruction >> RS_BITS) & DEFAULT_MASK;
			rt = (instruction >> RT_BITS) & DEFAULT_MASK;
			shamt = (instruction >> SHAMT_BITS) & DEFAULT_MASK; 
			RTypeALU(function, rs, rt, rd, shamt);	
			if (function != JR && function != JALR)
				nextInstruction();
		break;
		// Registradores tipo J
		case J:
		case JAL:
			address = (instruction << OFFSET_BITS) >> OFFSET_BITS; 
			JTypeALU(opcode, address);
		break;
		// Registradores tipo I
		default: 
			immediate = (instruction << IMMEDIATE_BITS) >> IMMEDIATE_BITS;
			rs = (instruction >> RS_BITS) & DEFAULT_MASK;
			rt = (instruction >> RT_BITS) & DEFAULT_MASK;
			ITypeALU(opcode, rs, rt, immediate);
			nextInstruction();
		break;
	}
}
// Chama a função para fragmentar o código hex contido no PC do estado atual 
void process_instruction(){
    /* execute one instruction here. You should use CURRENT_STATE and modify
     * values in NEXT_STATE. You can call mem_read_32() and mem_write_32() to
     * access memory. */
    CPU(mem_read_32(CURRENT_STATE.PC));
}
