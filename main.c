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


/*
ADD A,Im - 0000_xxxx - 0x00 - V
ADD B,Im - 0101_xxxx - 0x50 - V

MOV A,Im - 0011_xxxx - 0x30 - V
MOV B,Im - 0111_xxxx - 0x70 - V

MOV A,B  - 0001_0000 - 0x10 - V
MOV B,A  - 0100_0000 - 0x40 - V

JMP Im   - 1111_xxxx - 0xf0 - V
JNC Im   - 1110_xxxx - 0xe0 - V

IN A     - 0010_0000 - 0x20 - V
IN B     - 0110_0000 - 0x60 - V

OUT B     - 1001_0000 - 0x90 - V
OUT Im    - 1011_xxxx - 0xb0 - V
*/

typedef struct {
	unsigned char A;
	unsigned char B;
	unsigned char PC;
	unsigned char CF;
	unsigned char input;
	unsigned char output;
} registers;


void read_opcode(unsigned char *);
void print_console(registers *, unsigned char *);
	


int main(){
	
	registers TD4;
	
	TD4.A = 0x00;
	TD4.B = 0x00;
	TD4.PC = 0x00;
	TD4.CF = 0x00;
	TD4.input = 0x00;
	TD4.output = 0x00;
	
	unsigned char cnt_cf = 0x00;
	

	unsigned char *mem = calloc(MEM_SIZE, 1);
	unsigned char instruction;
	unsigned char data;

	char *input = NULL;
	char buf[2];
	buf[0] = 0x00;
	buf[1] = '\0';
	size_t len = 0;
	ssize_t read = 0;

	read_opcode(mem);


	for(;;){
			
		instruction = *(mem+TD4.PC) & 0xf0;
		data = *(mem+TD4.PC) & 0x0f;

		print_console(&TD4, mem);

		if((instruction == 0x20) || (instruction == 0x60)){
			printf("\033[%dm\033[2K\r%2d | "BYTE_TO_BINARY_PATTERN \
					"\t\033[32mEnter input: ", \
					(MEM_SIZE-1 == TD4.PC) ? 31 : 0, \
					MEM_SIZE-1, BYTE_TO_BINARY(*(mem+MEM_SIZE-1)));

			read = getline(&input, &len, stdin);
			if(input[read-1] == '\n'){
				input[read-1] = '\0';
				read--;
			}
			printf("\033[%dA\r", 1);
	
			memcpy(&buf, input, 1);
			sscanf(buf, "%x", &TD4.input);
		}
		
		printf("\033[%dA\r", MEM_SIZE);
		
		


		switch( instruction ){
		case 0x00:
			TD4.A += data;
			if( TD4.A >= 0x10){
				TD4.A &= 0x0f;
				TD4.CF = 0x01;
				cnt_cf = 0x02;
			}
			break;

		case 0x50:
			TD4.B += data;
			if( TD4.B >= 0x10){
				TD4.B &= 0x0f;
				TD4.CF = 0x01;
				cnt_cf = 0x02;
			}
			break;

		case 0x30:
			TD4.A = data; 
			break;
		
		case 0x70:
			TD4.B = data; 
			break;

		case 0x10:
			TD4.A = TD4.B;	
			break;

		case 0x40:
			TD4.B = TD4.A;	
			break;

		case 0xf0:
			TD4.PC = data;
			TD4.PC --;
			break;

		case 0xe0:
			if(!TD4.CF){
				TD4.PC = data;
				TD4.PC --;
			}
			//TD4.CF = 0x00;
			break;

		case 0x20:
			TD4.A = TD4.input;
			break;

		case 0x60:
			TD4.B = TD4.input;
			break;

		case 0x90:
			TD4.output = TD4.B;
			break;

		case 0xb0:
			TD4.output = data;
			break;

		default:
			break;
		}


		if(cnt_cf < 0)
			cnt_cf = 0;
		if(!cnt_cf)
			TD4.CF = 0x00;

		cnt_cf--;

		sleep(1);
		TD4.PC++;	
		TD4.PC %= MEM_SIZE;

			
	}

	free(input);
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
		switch(i){
			case 0:
			printf("\033[%dm%2d | "BYTE_TO_BINARY_PATTERN \
				"\t\033[0m[ A] = "BYTE_TO_BINARY_PATTERN \
				"\t[ B] = "BYTE_TO_BINARY_PATTERN \
				"\n", \
				(i == TD4->PC) ? 31 : 0, i, BYTE_TO_BINARY(*(mem+i)), \
				BYTE_TO_BINARY(TD4->A), BYTE_TO_BINARY(TD4->B));
				break;

			case 1:
			printf("\033[%dm%2d | "BYTE_TO_BINARY_PATTERN \
				"\t\033[0m[CF] = "BYTE_TO_BINARY_PATTERN \
				"\t[PC] = "BYTE_TO_BINARY_PATTERN \
				"\n", \
				(i == TD4->PC) ? 31 : 0, i, BYTE_TO_BINARY(*(mem+i)), \
				BYTE_TO_BINARY(TD4->CF), BYTE_TO_BINARY(TD4->PC));
				break;

			case 3:
			printf("\033[%dm%2d | "BYTE_TO_BINARY_PATTERN \
				"\t\033[0m[out] = "BYTE_TO_BINARY_PATTERN \
				"\n", \
				(i == TD4->PC) ? 31 : 0, i, BYTE_TO_BINARY(*(mem+i)), \
				BYTE_TO_BINARY(TD4->output));
				break;

			case MEM_SIZE-1:
				printf("\033[%dm\033[2K\r%2d | "BYTE_TO_BINARY_PATTERN \
					"\t\033[0mEnter input: ", \
					(i == TD4->PC) ? 31 : 0, i, BYTE_TO_BINARY(*(mem+i)));
				break;

			default:
				printf("\033[%dm%2d | "BYTE_TO_BINARY_PATTERN"\n", \

					(i == TD4->PC) ? 31 : 0, i, BYTE_TO_BINARY(*(mem+i)));
				break;
		}
	}
	fflush(stdout);
}


