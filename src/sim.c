#include <stdio.h>
#include "shell.h"
#include "sim.h"

void link(uint32_t reg){
	NEXT_STATE.REGS[reg] = CURRENT_STATE.PC + FOUR;
}

void nextInstruction(){
	NEXT_STATE.PC += FOUR; 
}

void updateHIandLO (int64_t result){
	NEXT_STATE.HI = (result >> BITS_NUMBER) & ALL_32_BITS_1;
	NEXT_STATE.LO = result & ALL_32_BITS_1;
}

uint32_t readFromMemory_Xbits(uint32_t offset, uint32_t bits){
	uint32_t shift = BITS_NUMBER - bits;
	return (mem_read_32(offset) << shift) >> shift;
}

int32_t signExtend_5Bits(uint32_t shamt){
	int32_t offset = (shamt & LAST_5_BITS_1);
	if (offset & FIFTH_BIT_1)
		offset |= LAST_5_BITS_0_IN_HALFWORD;
	return offset;
}

int32_t signExtend_8Bits(uint32_t  immediate){
	int32_t offset = (immediate & LAST_24_BITS_1);
	if (offset & EIGHTH_BIT_1)
		offset |= FIRST_24_BITS_1; 
	return offset;
}

int32_t signExtend_16Bits(uint32_t immediate, uint32_t shift){
	int32_t offset = (immediate & FIRST_16_BITS_1) << shift;
	if(offset & (TENTH_SIXTH_BIT_1 << shift)) 
		offset |= LAST_16_BITS_1;
	return offset;
}

int32_t signExtend_26Bits_shiftLeft_2(uint32_t address){
	int32_t offset = (address & FIRST_5_BITS_0_IN_WORD) << TWO;
	if(offset & (SIXTH_BIT_1_IN_WORD << TWO)) 
		offset |= FIRST_4_BITS_1;
	return offset;	
}

int32_t zeroExtend(uint32_t immediate){
	return immediate | ZERO;
}

int32_t twosComplement(uint32_t operand){
	return (~operand) + HEX_ONE;
}

uint32_t extractBitsFromReg(uint32_t bits, uint32_t reg){
	uint32_t shift = BITS_NUMBER - bits; 
	return (CURRENT_STATE.REGS[reg] << shift) >> shift;
}

void loadInstructions(uint32_t function, uint32_t rs, uint32_t rt, int32_t immediate){
	uint32_t address = immediate + CURRENT_STATE.REGS[rs]; 
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

void storeInstructions(uint32_t function, uint32_t rs, uint32_t rt, int32_t immediate){
	uint32_t address = immediate + CURRENT_STATE.REGS[rs]; 
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

void shiftInstructions (uint32_t func, uint32_t rt, uint32_t rd, uint32_t shamt, uint32_t rs){
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
			shiftInstructions(func, rt, rd, shamt, rs); 
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
			(rd == ZERO)? link(RA): link(rd);
			NEXT_STATE.PC = CURRENT_STATE.REGS[rs];
		break;
	}
}

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
			branchInstructions(opcode, rs, rt, signExtend_16Bits(immediate, TWO));
		break;	
	}
}

void JTypeALU(uint32_t func, uint32_t address){		
	if (func == JAL)
		link(RA);
	NEXT_STATE.PC = signExtend_26Bits_shiftLeft_2(address);  
}

void CPU (uint32_t instruction){
	uint32_t opcode, rs, rt, rd, address, immediate, shamt, function; 
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

void process_instruction(){
    /* execute one instruction here. You should use CURRENT_STATE and modify
     * values in NEXT_STATE. You can call mem_read_32() and mem_write_32() to
     * access memory. */
    CPU(mem_read_32(CURRENT_STATE.PC));
}
