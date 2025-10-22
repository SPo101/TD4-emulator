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

	unsigned char *RAM;
	unsigned char *work_register1;
	unsigned char *work_register2;
	unsigned char *load_register;

	unsigned char cnt_cf;
	unsigned char arithmetic_logic_act;//0-arithmetic 1-logic
	unsigned char move_bits;//0 for mostly all reg, 1 for x, 2 for y,  3 for xy, pc etc
} registers;

void opcode_decode(unsigned char *, registers *);
unsigned char not_(unsigned char, int = 0);

//remove later
unsigned char command = '&';

int main(int argc, char *argv[]){

	unsigned char inst = 0x8e;
	registers TD4m;
	TD4m.A = 'a';
	TD4m.B = 'b';
	TD4m.PC = 'p';
	TD4m.XY = 'x';
	TD4m.CF = 'c';
	TD4m.ZF = 'z';
	TD4m.input = 'i';
	TD4m.output = 'o';
	TD4m.zero =  '0';

	opcode_decode(&inst, &TD4m);

	cout << "summand = " << *TD4m.work_register1 << endl;
	cout << " res in = " << *TD4m.load_register << endl;


	//when we direct registers to arithmetic unit or logic unit
	//if()
	
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


	if(is_command)
		goto data_is_command;


	SUM1 |= (tobool(D5) << 5);
	SUM1 |= (tobool(D7|D4) << 4);

	LOAD |= (tobool(D7|D6) << 7);
	LOAD |= (tobool(D7|not_(D6)) << 6);
	LOAD |= (tobool(not_(D7 & not_(D6))) << 5);
	LOAD |= (tobool(not_(D6 & D7 & (D4 | not_(TD4m->CF)))) << 4);


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
data_is_command:
	TD4m->work_register1 = &command;
	TD4m->work_register2 = &command;

	
	LOAD |= (tobool(not_(D3) & D2 & D1 & D0) << 7);
	LOAD |= (tobool(D3 & D2 & not_(D1)) << 6);
	LOAD |= (tobool(D3 & (not_(D2) & D0 | D2 & D1)) << 5);
	LOAD |= (tobool( (D1 & ( (D3 & not_(D2) & not_(D0)) | \
		(not_(D3) & D2 & D0))) | (D3 & D2 & ((not_(D1) & D0) | (D1 & not_(D0))))) << 4);

	//define where to store a result from ALU	
	//A-0000 B-0001 RAM-0010 XY-0011 X-0100 Y-0101 PC-0110 zero-0111 one-1000 out-1001

	TD4m->move_bits = 0;
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
			TD4m->move_bits = 8;
			break;	
	}

/*
	//define 1st summand for ALU
	switch(){
		break;	
	}
	//define 2nd summand for ALU
	switch(){
		break;	
	}
*/
	return;
}

//togglebit
unsigned char not_(unsigned char num, int pos){
	return num ^ (1<<pos);
}

