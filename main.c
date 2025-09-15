#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MEM_SIZE 16 // 16 bytes;

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
  ((byte) & 0x80 ? '1' : '0'), \
  ((byte) & 0x40 ? '1' : '0'), \
  ((byte) & 0x20 ? '1' : '0'), \
  ((byte) & 0x10 ? '1' : '0'), \
  ((byte) & 0x08 ? '1' : '0'), \
  ((byte) & 0x04 ? '1' : '0'), \
  ((byte) & 0x02 ? '1' : '0'), \
  ((byte) & 0x01 ? '1' : '0') 



typedef struct {
	unsigned char A;
	unsigned char B;
	unsigned char PC;
	unsigned char CF;
} registers;


void read_opcode(unsigned char *);
void print_console(registers *, unsigned char *);
	


int main(){
	
	registers TD4;
	
	TD4.A = 0x00;
	TD4.B = 0x00;
	TD4.PC = 0x00;
	TD4.CF = 0x00;

	unsigned char *mem = calloc(MEM_SIZE, 1);


	read_opcode(mem);



	for(;;){
			
		print_console(&TD4, mem);


/*
ADD A,Im - 0000_xxxx - 0x00 - V
ADD B,Im - 0101_xxxx - 0x50 - X

MOV A,Im - 0011_xxxx - 0x30 - X
MOV B,Im - 0111_xxxx - 0x70 - X

MOV A,B  - 0001_0000 - 0x10 - X
MOV B,A  - 0100_0000 - 0x40 - X

JMP Im   - 1111_xxxx - 0xf0 - V
JNC Im   - 1110_xxxx - 0xe0 - X

IN A     - 0010_0000 - 0x20 - X
IN B     - 0110_0000 - 0x60 - X

OUT B     - 1001_0000 - 0x90 - X
OUT Im    - 1011_xxxx - 0xb0 - X
*/

		switch( *(mem+TD4.PC) & 0xf0 ){
		case 0x00:
			TD4.A += *(mem+TD4.PC) & 0x0f;
			if( TD4.A == 0x10){
				TD4.A = 0x00;
				TD4.CF = 0x01;
			}

			break;
		case 0xf0:
			TD4.PC = *(mem+TD4.PC) & 0x0f;
			TD4.PC --;
			break;
		default:
			break;
		}


		

		sleep(2);
		TD4.PC++;	
		TD4.PC %= MEM_SIZE;
			
	}
}


void read_opcode(unsigned char *mem){

	size_t size = 0;
	char *str = NULL;
	char str2[2];

	getline(&str, &size, stdin);
	
	for(int i=0; i<MEM_SIZE; i++){
		memcpy(&str2, str+i*2, 2);
		sscanf(str2, "%x", mem+i);
	}
	free(str);
}


void print_console(registers *TD4, unsigned char *mem){

	printf("\n");
	for(int i=0; i<MEM_SIZE; i++){
		if(!i)
			printf("\033[%dm%2d | "BYTE_TO_BINARY_PATTERN \
				"\t\033[0m[A] = "BYTE_TO_BINARY_PATTERN \
				"\t[B] = "BYTE_TO_BINARY_PATTERN \
				"\n", \
				(i == TD4->PC) ? 31 : 0, i, BYTE_TO_BINARY(*(mem+i)), \
				BYTE_TO_BINARY(TD4->A), BYTE_TO_BINARY(TD4->B));

		else if(i==1)
			printf("\033[%dm%2d | "BYTE_TO_BINARY_PATTERN \
				"\t\033[0m[CF] = "BYTE_TO_BINARY_PATTERN \
				"\t[PC] = "BYTE_TO_BINARY_PATTERN \
				"\n", \
				(i == TD4->PC) ? 31 : 0, i, BYTE_TO_BINARY(*(mem+i)), \
				BYTE_TO_BINARY(TD4->CF), BYTE_TO_BINARY(TD4->PC));
		else
			printf("\033[%dm%2d | "BYTE_TO_BINARY_PATTERN"\n", \
				(i == TD4->PC) ? 31 : 0, i, BYTE_TO_BINARY(*(mem+i)));
		fflush(stdout);
	}
	printf("\033[18A");
	printf("\n");

}
