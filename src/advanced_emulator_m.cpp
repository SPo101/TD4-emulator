#include <iostream>

using namespace std;
#define BYTE_TO_BINARY_PATTERN8 "%c%c%c%c%c%c%c%c"

#define BYTE_TO_BINARY8(byte)  \
  ((byte) & 0x80 ? '1' : '0'), \
  ((byte) & 0x40 ? '1' : '0'), \
  ((byte) & 0x20 ? '1' : '0'), \
  ((byte) & 0x10 ? '1' : '0'), \
  ((byte) & 0x08 ? '1' : '0'), \
  ((byte) & 0x04 ? '1' : '0'), \
  ((byte) & 0x02 ? '1' : '0'), \
  ((byte) & 0x01 ? '1' : '0') 

#define tobool(num) \
  ((num) & 0x01 ? 1 : 0)


  

typedef struct {
	unsigned char A;
	unsigned char B;
	unsigned char PC;
	unsigned char XY;
	unsigned char CF;
	unsigned char ZF;
	unsigned char input;
	unsigned char output;
	unsigned char zero;
	unsigned char one;

	unsigned char *RAM;
	unsigned char *work_register1;
	unsigned char *work_register2;
	unsigned char *load_register;

	unsigned char cnt_cf;
	unsigned char cnt_zf;
	unsigned char arithmetic_logic_act;//0-arithmetic 1-logic
	unsigned char move_bits;//0 for mostly all reg, 1 for x, 2 for y/pcl,  3 for xy, pc etc
} registers;

void opcode_decode(unsigned char *, registers *);
unsigned char not_(unsigned char, int = 0);
unsigned char neg(unsigned char);

void arithmetic_unit(unsigned char *instruction, registers *TD4m){

}

void logic_unit(unsigned char *instruction, registers *TD4m){
	switch(*instruction){
		case 0x81: //A = -A ( A = neg(A) + 1)
		case 0x82: //A = !A ( A = neg(A) + 0)
			*TD4m->load_register = neg(*TD4m->work_register1) + *TD4m->work_register2;
			break;
		case 0x83: //A or B
			*TD4m->load_register = *TD4m->work_register1 | *TD4m->work_register2;
			break;
		case 0x84: //A and B
			*TD4m->load_register = *TD4m->work_register1 & *TD4m->work_register2;
			break;
		case 0x85: //A xor B
			*TD4m->load_register = *TD4m->work_register1 ^ *TD4m->work_register2;
			break;
	}


}

int main(int argc, char *argv[]){

	unsigned char inst = 0x81;
	registers TD4m;
	TD4m.A = 'A';
	TD4m.B = 'B';
	TD4m.PC = 'P';
	TD4m.XY = 0xff;
	TD4m.CF = 'C';
	TD4m.ZF = 'Z';
	TD4m.input = 'I';
	TD4m.output = 'O';
	TD4m.zero =  0x00;
	TD4m.one =  0x01;

	TD4m.RAM = (unsigned char*) malloc(124);

	opcode_decode(&inst, &TD4m);

	printf("summand1 = %c (%hhx) \n", *TD4m.work_register1, *TD4m.work_register1);
	if(TD4m.work_register2 != NULL)
		printf("summand2 = %c (%hhx) \n", *TD4m.work_register2, *TD4m.work_register2);
	if(TD4m.load_register != NULL)
		printf("res = %c (%hhx) \n", *TD4m.load_register, *TD4m.load_register);
	printf("mode = %c (%hhx) \n", TD4m.move_bits, TD4m.move_bits);


	/*
	need handler
		*TD4m.work_register2, *TD4m.load_register
	can be NULL
	*/

	//when we direct registers to arithmetic unit or logic unit
	if(((0x01 <= (inst&0x0f)) & ((inst&0x0f) <= 0x05)) & ((inst&0xf0) == 0x80))
		logic_unit(&inst, &TD4m);
	else
		arithmetic_unit(&inst, &TD4m);	

	/*
	if(*TD4m->load_register >= 0x10){
		*TD4m->load_register &= 0x0f;
		TD4m->CF = 0x01;
		TD4m->cnt_cf = 0x02;
	}

	if(*TD4m->load_register == 0x00){
		TD4m->ZF = 0x01;
		TD4m->cnt_zf = 0x02;
	}
	*/
}

void opcode_decode(unsigned char *instruction, registers *TD4m){ 
	unsigned char D0 = (*instruction & 0x01)>>0; 
	unsigned char D1 = (*instruction & 0x02)>>1;
	unsigned char D2 = (*instruction & 0x04)>>2;
	unsigned char D3 = (*instruction & 0x08)>>3; 

	unsigned char D4 = (*instruction & 0x10)>>4; 
	unsigned char D5 = (*instruction & 0x20)>>5;
	unsigned char D6 = (*instruction & 0x40)>>6;
	unsigned char D7 = (*instruction & 0x80)>>7; 

	unsigned char SUM1 = 0x00;
	unsigned char SUM2 = 0x00;
	unsigned char LOAD = 0x00;

	/*is data command?
	in cases 8,a,c,d
	*/
	unsigned char is_command = (D7 & not_(D6) & not_(D4)) | \
								(D7 & D6 & not_(D5));


	TD4m->move_bits = 0;

	if(is_command)
		goto data_is_command;


	SUM1 |= (tobool(D5) << 5);
	SUM1 |= (tobool(D7|D4) << 4);

	LOAD |= (tobool(D7|D6									) << 7);
	LOAD |= (tobool(D7|not_(D6)								) << 6);
	LOAD |= (tobool(not_(D7 & not_(D6))						) << 5);
	LOAD |= (tobool(not_(D6 & D7 & (D4 | not_(TD4m->CF)))	) << 4);


	//block when last 4 bits present data
	switch(SUM1){ //define 1st summand for ALU 
		case 0x00: //A register
			TD4m->work_register1 = &TD4m->A;
			break;
		case 0x10: //B register
			TD4m->work_register1 = &TD4m->B;
			break;
		case 0x20: //IN
			TD4m->work_register1 = &TD4m->input;
			break;
		case 0x30: //ZERO
			TD4m->work_register1 = &TD4m->zero;
			break;
	}

	TD4m->work_register2 = NULL;

	switch(LOAD){//define where to store a result from ALU
		case 0x70: //A register
			TD4m->load_register = &TD4m->A;
			break;
		case 0xb0: //B register
			TD4m->load_register = &TD4m->B;
			break;
		case 0xd0: //OUT
			TD4m->load_register = &TD4m->output;
			break;
		case 0xe0: //PC
			TD4m->load_register = &TD4m->PC;
			break;
		case 0xf0: //for JNC when C
			TD4m->load_register = NULL;
			break;
	}
	return;

	//block when last 4 bits present command
	//A-0000 B-0001 RAM-0010 XY-0011 X-0100 Y-0101 PC-0110 zero-0111 one-1000 out-1001
data_is_command:
	SUM1 |= (tobool(D3 & ((not_(D2) & not_(D0)) | (D2 & D1))) << 5);
	SUM1 |= (tobool(D3 & D1 & ( (not_(D2) * D0) | D2 )		) << 4);

	SUM2 |= (tobool((not_(D3) & not_(D2) & not_(D1) & D0) | (D3 & D2 & D1 & not_(D0))							) << 7);
	SUM2 |= (tobool((not_(D3)&D1&((not_(D2)&not_(D0))|(D2&D0))) | (D3&not_(D2)) | (D3&D2&(not_(D1)|(D1&D0)))	) << 6);
	SUM2 |= (tobool(SUM2>>6																						) << 5);
	SUM2 |= (tobool(not_(SUM2>>7)																				) << 4);
	
	LOAD |= (tobool(not_(D3) & D2 & D1 & D0										) << 7);
	LOAD |= (tobool(D3 & D2 & not_(D1)											) << 6);
	LOAD |= (tobool(D3 & ((not_(D2) & D0) | (D2 & D1))							) << 5);
	LOAD |= (tobool( (D1 & ( (D3 & not_(D2) & not_(D0)) | \
		(not_(D3) & D2 & D0))) | (D3 & D2 & ((not_(D1) & D0) | (D1 & not_(D0))))) << 4);

	//define where to store a result from ALU	
	switch(LOAD){
		case 0x00: //A register
			TD4m->load_register = &TD4m->A;
			break;	
		case 0x10: //B register
			TD4m->load_register = &TD4m->B;
			break;	
		case 0x20: //RAM
			TD4m->load_register = (TD4m->RAM+TD4m->XY);
			break;	
		case 0x90: //OUT
			TD4m->load_register = &TD4m->output;
			break;	
		case 0x30: //XY
			TD4m->load_register = &TD4m->XY;
			TD4m->move_bits = 3;
			break;	
		case 0x40: //X
			TD4m->load_register = &TD4m->XY;
			TD4m->move_bits = 1;
			break;	
		case 0x50: //Y
			TD4m->load_register = &TD4m->XY;
			TD4m->move_bits = 2;
			break;	
		case 0x60: //PC
			TD4m->load_register = &TD4m->PC;
			TD4m->move_bits = 3;
			break;	
	}


	//define 1st summand for ALU
	switch(SUM1){
		case 0x00: //A register
			TD4m->work_register1 = &TD4m->A;
			break;	
		case 0x10: //B register
			TD4m->work_register1 = &TD4m->B;
			break;	
		case 0x20: //RAM
			TD4m->work_register1 = (TD4m->RAM+TD4m->XY);
			break;	
		case 0x30: //XY
			TD4m->work_register1 = &TD4m->XY;
			break;	
	}


	//define 2nd summand for ALU
	switch(SUM2){
		case 0x10: //B register
			TD4m->work_register2 = &TD4m->B;
			break;	
		case 0x70: //ZERO
			TD4m->work_register2 = &TD4m->zero;
			break;	
		case 0x80: //ONE
			TD4m->work_register2 = &TD4m->one;
			break;	
	}

	/*
	printf(BYTE_TO_BINARY_PATTERN8"\n", BYTE_TO_BINARY8(SUM1));
	printf(BYTE_TO_BINARY_PATTERN8"\n", BYTE_TO_BINARY8(SUM2));
	printf(BYTE_TO_BINARY_PATTERN8"\n", BYTE_TO_BINARY8(LOAD));
	*/

	if( ((*instruction)&0xf0) == 0xa0){
		TD4m->work_register1 = &TD4m->input;
		TD4m->work_register2 = &TD4m->zero;
		TD4m->load_register = NULL;

		if(TD4m->ZF){
			TD4m->load_register = &TD4m->PC;
			TD4m->move_bits = 2;
		}

	}
	if( ((*instruction)&0xf0) == 0xc0){
		TD4m->work_register1 = &TD4m->input;
		TD4m->work_register2 = &TD4m->zero;
		TD4m->load_register = &TD4m->XY;
		TD4m->move_bits = 2;
		
	}
	if( ((*instruction)&0xf0) == 0xd0){
		TD4m->work_register1 = &TD4m->input;
		TD4m->work_register2 = &TD4m->zero;
		TD4m->load_register = &TD4m->XY;
		TD4m->move_bits = 1;
		
	}
	return;
}

//togglebit
unsigned char not_(unsigned char num, int pos){
	return num ^ (1<<pos);
}

unsigned char neg(unsigned char num){
	num &= 0x0f;
	num = not_(num, 3);
	num = not_(num, 2);
	num = not_(num, 1);
	num = not_(num, 0);
	return num;
}

